/*---------------------------------------------------------------------------*\
    CFDEMcoupling - Open Source CFD-DEM coupling

    CFDEMcoupling is part of the CFDEMproject
    www.cfdem.com
                                Christoph Goniva, christoph.goniva@cfdem.com
                                Copyright 2009-2012 JKU Linz
                                Copyright 2012-2015 DCS Computing GmbH, Linz
                                Copyright 2015-     JKU Linz
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

#include "error.H"

#include "ShirgaonkarIB.H"
#include "addToRunTimeSelectionTable.H"
#include "voidFractionModel.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(ShirgaonkarIB, 0);

addToRunTimeSelectionTable
(
    forceModel,
    ShirgaonkarIB,
    dictionary
);


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

// Construct from components
ShirgaonkarIB::ShirgaonkarIB
(
    const dictionary& dict,
    cfdemCloud& sm
)
:
    forceModel(dict,sm),
    propsDict_(dict.subDict(typeName + "Props")),
    verbose_(propsDict_.found("verbose")),
    twoDimensional_(propsDict_.found("twoDimensional")),
    depth_(1),
    multisphere_(propsDict_.found("multisphere")), //  drag for a multisphere particle
    useTorque_(propsDict_.found("useTorque")),
    velFieldName_(propsDict_.lookup("velFieldName")),
    U_(sm.mesh().lookupObject<volVectorField> (velFieldName_)),
    pressureFieldName_(propsDict_.lookup("pressureFieldName")),
    p_(sm.mesh().lookupObject<volScalarField> (pressureFieldName_)),
    solidVolFractionName_(multisphere_?propsDict_.lookup("solidVolFractionName"):word::null),
    lambda_(multisphere_?sm.mesh().lookupObject<volScalarField>(solidVolFractionName_):volScalarField::null())
{
    //Append the field names to be probed
    particleCloud_.probeM().initialize(typeName, typeName+".logDat");
    particleCloud_.probeM().vectorFields_.append("dragForce"); //first entry must the be the force
    particleCloud_.probeM().writeHeader();

    if (twoDimensional_)
    {
        depth_ = propsDict_.lookup("depth");
        Info << "2-dimensional simulation - make sure DEM side is 2D" << endl;
        Info << "depth of domain is assumed to be :" << depth_ << endl;
    }

    // init force sub model
    setForceSubModels(propsDict_);

    // define switches which can be read from dict
    forceSubM(0).setSwitchesList(SW_TREAT_FORCE_EXPLICIT,true); // activate treatExplicit switch

    // read those switches defined above, if provided in dict
    forceSubM(0).readSwitches();

    particleCloud_.checkCG(false);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

ShirgaonkarIB::~ShirgaonkarIB()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void ShirgaonkarIB::setForce() const
{

    label cellI;
    vector drag;
    vector torque;

    volVectorField h=forceSubM(0).IBDragPerV(U_,p_);

    #include "setupProbeModel.H"

    for(int index=0; index< particleCloud_.numberOfParticles(); index++)
    {
        //if(mask[index][0])
        //{
            drag=vector::zero;
            torque=vector::zero;

            for(int subCell=0;subCell<particleCloud_.voidFractionM().cellsPerParticle()[index][0];subCell++)
            {
                //Info << "subCell=" << subCell << endl;
                cellI = particleCloud_.cellIDs()[index][subCell];

                if (cellI > -1) // particle Found
                {
                    drag += h[cellI]*h.mesh().V()[cellI];
                    if(useTorque_)
                    {
                        vector rc = particleCloud_.mesh().C()[cellI];
                        vector positionCenter = particleCloud_.position(index);
                        torque += (rc - positionCenter)^h[cellI]*h.mesh().V()[cellI];
                    }
                }

            }

            // set force on particle
            if(twoDimensional_) drag /= depth_;

            //Set value fields and write the probe
            if(probeIt_)
            {
                #include "setupProbeModelfields.H"
                vValues.append(drag);           //first entry must the be the force
                particleCloud_.probeM().writeProbe(index, sValues, vValues);
            }

            // write particle based data to global array
            forceSubM(0).partToArray(index,drag,vector::zero);

            if(verbose_) Info << "impForces = " << particleCloud_.impForces()[index][0]
                              << ","            << particleCloud_.impForces()[index][1]
                              << ","            << particleCloud_.impForces()[index][2]
                              << endl;

            if(useTorque_)
            {
                for(int j=0;j<3;j++) particleCloud_.DEMTorques()[index][j] = torque[j]; // Adding to the particle torque;
                if(verbose_) Info << "DEMTorques = " << particleCloud_.DEMTorques()[index][0] << "," << particleCloud_.DEMTorques()[index][1] << "," << particleCloud_.DEMTorques()[index][2] << endl;
            }
        //}
    }

    // Calculate the force if the particle is multisphere template
    if(multisphere_) calcForce();
}

void ShirgaonkarIB::calcForce() const
{
    vector dragMS;

    volVectorField h=forceSubM(0).IBDragPerV(U_,p_);

    dragMS = vector::zero;

    forAll(lambda_,cellI)
    {
        if(lambda_[cellI] > 0) dragMS += h[cellI]*h.mesh().V()[cellI];
        else dragMS = dragMS;
    }

    Pout << "Drag force on particle clump = " << dragMS[0] << ", " << dragMS[1] << ", " << dragMS[2] << endl;
}




// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
