/*---------------------------------------------------------------------------*\
    CFDEMcoupling - Open Source CFD-DEM coupling

    CFDEMcoupling is part of the CFDEMproject
    www.cfdem.com
                                Christoph Goniva, christoph.goniva@cfdem.com
                                Copyright 2009-2012 JKU Linz
                                Copyright 2012-     DCS Computing GmbH, Linz
-------------------------------------------------------------------------------
License
    This file is part of CFDEMcoupling.

    CFDEMcoupling is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 3 of the License, or (at your
    option) any later version.

    CFDEMcoupling is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with CFDEMcoupling; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Description
    This code is designed to realize coupled CFD-DEM simulations using LIGGGHTS
    and OpenFOAM(R). Note: this code is not part of OpenFOAM(R) (see DISCLAIMER).
\*---------------------------------------------------------------------------*/

#include "fileName.H"
#include "cfdemCloudIB.H"
#include "voidFractionModel.H"
#include "forceModel.H"
#include "locateModel.H"
#include "dataExchangeModel.H"
#include "IOModel.H"
#include "IOmanip.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

// Construct from components
cfdemCloudIB::cfdemCloudIB
(
    const fvMesh& mesh
)
:
    cfdemCloud(mesh),
    angularVelocities_(NULL),
    pRefCell_(readLabel(mesh.solutionDict().subDict("PISO").lookup("pRefCell"))),
    pRefValue_(readScalar(mesh.solutionDict().subDict("PISO").lookup("pRefValue"))),
    DEMTorques_(NULL),
    haveEvolvedOnce_(false),
    skipLagrangeToEulerMapping_(false)
{

    if(this->couplingProperties().found("skipLagrangeToEulerMapping"))
    {
        Info << "Will skip lagrange-to-Euler mapping..." << endl;
        skipLagrangeToEulerMapping_=true;
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

cfdemCloudIB::~cfdemCloudIB()
{
    dataExchangeM().destroy(angularVelocities_,3);
    dataExchangeM().destroy(DEMTorques_,3);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //
void cfdemCloudIB::getDEMdata()
{
    cfdemCloud::getDEMdata();
    dataExchangeM().getData("omega","vector-atom",angularVelocities_);
}

bool cfdemCloudIB::reAllocArrays()
{
    if(cfdemCloud::reAllocArrays())
    {
        // get arrays of new length
        dataExchangeM().allocateArray(angularVelocities_,0.,3);
        dataExchangeM().allocateArray(DEMTorques_,0.,3);
        return true;
    }
    return false;
}

void cfdemCloudIB::giveDEMdata()
{
    cfdemCloud::giveDEMdata();
    dataExchangeM().giveData("hdtorque","vector-atom",DEMTorques_);
}

inline double ** cfdemCloudIB::DEMTorques() const
{
    return DEMTorques_;
}

bool cfdemCloudIB::evolve(volVectorField& Us)
{
    numberOfParticlesChanged_ = false;
    arraysReallocated_=false;
    bool doCouple=false;

    if (dataExchangeM().doCoupleNow())
    {
        Info << "\n timeStepFraction() = " << dataExchangeM().timeStepFraction() << endl;
        dataExchangeM().couple(0);
        doCouple=true;

//        Info << "skipLagrangeToEulerMapping_: " << skipLagrangeToEulerMapping_
//             << " haveEvolvedOnce_: " << haveEvolvedOnce_ << endl;
        if(!skipLagrangeToEulerMapping_ || !haveEvolvedOnce_)
        {
          if(verbose_) Info << "- getDEMdata()" << endl;
          getDEMdata();
          Info << "nr particles = " << numberOfParticles() << endl;

          // search cellID of particles
          if(verbose_) Info << "- findCell()" << endl;
          locateM().findCell(NULL,positions_,cellIDs_,numberOfParticles());
          if(verbose_) Info << "findCell done." << endl;

          // set void fraction field
          if(verbose_) Info << "- setvoidFraction()" << endl;
          voidFractionM().setvoidFraction(NULL,voidfractions_,particleWeights_,particleVolumes_,particleV_);
          if(verbose_) Info << "setvoidFraction done." << endl;
        }

        // set particles forces
        if(verbose_) Info << "- setForce(forces_)" << endl;
        for(int index = 0;index <  numberOfParticles_; ++index){
            for(int i=0;i<3;i++){
                impForces_[index][i] = 0;
                expForces_[index][i] = 0;
                DEMForces_[index][i] = 0;
            }
        }
        for (int i=0;i<nrForceModels();i++) forceM(i).setForce();
        if(verbose_) Info << "setForce done." << endl;

        // set particle velocity field
        if(verbose_) Info << "- setVelocity(velocities_)" << endl;
        calcForcingTerm(Us); // only needed for continuous forcing

        // write DEM data
        if(verbose_) Info << " -giveDEMdata()" << endl;
        giveDEMdata();

        dataExchangeM().couple(1);

        haveEvolvedOnce_=true;
    }

    if(verbose_) Info << "evolve done." << endl;

    //if(verbose_)    #include "debugInfo.H";

    // do particle IO
    IOM().dumpDEMdata();

    return doCouple;
}

void cfdemCloudIB::calcForcingTerm(volVectorField& Us)
{
    label cell = 0;
    vector uP(0,0,0);
    vector rVec(0,0,0);
    vector velRot(0,0,0);
    vector angVel(0,0,0);
    Us.primitiveFieldRef() = vector::zero;
    for (int index = 0; index < numberOfParticles(); ++index)
    {
        for (int subCell = 0; subCell < voidFractionM().cellsPerParticle()[index][0]; subCell++)
        {
            cell = cellIDs()[index][subCell];

            if (cell >=0)
            {
                // calc particle velocity
                for (int i=0;i<3;i++) rVec[i]=Us.mesh().C()[cell][i]-position(index)[i];
                for (int i=0;i<3;i++) angVel[i]=angularVelocities()[index][i];
                velRot=angVel^rVec;
                for (int i=0;i<3;i++) uP[i] = velocities()[index][i]+velRot[i];
                Us[cell] = (1-voidfractions_[index][subCell])*uP;
            }
        }
    }
    Us.correctBoundaryConditions();
}

void cfdemCloudIB::calcVelocityCorrection
(
    volScalarField& p,
    volVectorField& U,
    volScalarField& phiIB,
    volScalarField& voidfraction
)
{
    label cellI=0;
    vector uParticle(0,0,0);
    vector rVec(0,0,0);
    vector velRot(0,0,0);
    vector angVel(0,0,0);

    for(int index=0; index< numberOfParticles(); index++)
    {
        //if(regionM().inRegion()[index][0])
        //{
            for(int subCell=0;subCell<voidFractionM().cellsPerParticle()[index][0];subCell++)
            {
                //Info << "subCell=" << subCell << endl;
                cellI = cellIDs()[index][subCell];

                if (cellI >= 0)
                {
                    // calc particle velocity
                    for(int i=0;i<3;i++) rVec[i]=U.mesh().C()[cellI][i]-position(index)[i];
                    for(int i=0;i<3;i++) angVel[i]=angularVelocities()[index][i];
                    velRot=angVel^rVec;
                    for(int i=0;i<3;i++) uParticle[i] = velocities()[index][i]+velRot[i];

                    // impose field velocity
                    U[cellI]= (1-voidfractions_[index][subCell])*uParticle+voidfractions_[index][subCell]*U[cellI];
                }
            }
        //}
    }
    U.correctBoundaryConditions();

    // make field divergence free - set reference value in case it is needed
    fvScalarMatrix phiIBEqn
    (
        fvm::laplacian(phiIB) == fvc::div(U) + fvc::ddt(voidfraction)
    );
    if(phiIB.needReference())
    {
        phiIBEqn.setReference(pRefCell_, pRefValue_);
    }

    phiIBEqn.solve();

    U=U-fvc::grad(phiIB);
    U.correctBoundaryConditions();

    // correct the pressure as well
    p=p+phiIB/U.mesh().time().deltaT();  // no need to account for rho since p=(p/rho) in OF
    p.correctBoundaryConditions();

    if (couplingProperties_.found("checkinterface"))
    {
        Info << "checking no-slip on interface..." << endl;
//          #include "checkInterfaceVelocity.H" //TODO: check carefully!
    }

}

vector cfdemCloudIB::angularVelocity(int index) const
{
    return vector(angularVelocities_[index][0],angularVelocities_[index][1],angularVelocities_[index][2]);
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
