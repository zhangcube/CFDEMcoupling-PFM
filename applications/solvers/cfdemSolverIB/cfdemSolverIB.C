/*---------------------------------------------------------------------------*\
    CFDEMcoupling - Open Source CFD-DEM coupling

    CFDEMcoupling is part of the CFDEMproject
    www.cfdem.com
                                Christoph Goniva, christoph.goniva@cfdem.com
                                Copyright (C) 1991-2009 OpenCFD Ltd.
                                Copyright (C) 2009-2012 JKU, Linz
                                Copyright (C) 2012-2015 DCS Computing GmbH,Linz
                                Copyright (C) 2015-     JKU, Linz
-------------------------------------------------------------------------------
License
    This file is part of CFDEMcoupling.

    CFDEMcoupling is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CFDEMcoupling is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with CFDEMcoupling.  If not, see <http://www.gnu.org/licenses/>.

Application
    cfdemSolverIB

Description
    Transient solver for incompressible flow.
    The code is an evolution of the solver pisoFoam in OpenFOAM(R) 1.6,
    where additional functionality for CFD-DEM coupling using immersed body
    (fictitious domain) method is added.
Contributions
    Alice Hager
    Daniel Queteschiner
    Thomas Lichtenegger
    Achuth N. Balachandran Nair
\*---------------------------------------------------------------------------*/


#include "fvCFD.H"
#include "singlePhaseTransportModel.H"
#include "turbulentTransportModel.H"
#include "pisoControl.H"

#include "cfdemCloudIB.H"
#include "implicitCouple.H"

#include "averagingModel.H"
#include "regionModel.H"
#include "voidFractionModel.H"

#include "dynamicFvMesh.H" //dyM

#include "cellSet.H"

#include "fvOptions.H"  // added the fvOptions library

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{

    #include "setRootCase.H"
    #include "createTime.H"
    #include "createDynamicFvMesh.H"
    #include "createControl.H"
    #include "createTimeControls.H"
    #include "createFields.H"
    #include "initContinuityErrs.H"
    #include "createFvOptions.H"

    // create cfdemCloud
    #include "readGravitationalAcceleration.H"
    cfdemCloudIB particleCloud(mesh);

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< "\nStarting time loop\n" << endl;

    while (runTime.loop())
    {
        Info<< "Time = " << runTime.timeName() << nl << endl;

        //=== dyM ===================
        interFace = mag(mesh.lookupObject<volScalarField>("voidfractionNext"));
        mesh.update(); //dyM

        #include "readTimeControls.H"
        #include "CourantNo.H"
        #include "setDeltaT.H"

        // do particle stuff
        Info << "- evolve()" << endl;
        particleCloud.evolve(Us);

        // Pressure-velocity PISO corrector
        {
            MRF.correctBoundaryVelocity(U);

            // Momentum predictor

            fvVectorMatrix UEqn
            (
                fvm::ddt(voidfraction,U) + MRF.DDt(U)
              + fvm::div(phi, U)
              + turbulence->divDevReff(U)
             ==
                fvOptions(U)
            );

            UEqn.relax();

            fvOptions.constrain(UEqn);

            if (piso.momentumPredictor())
            {
                solve(UEqn == -fvc::grad(p));
                fvOptions.correct(U);
            }

            // --- PISO loop
            while (piso.correct())
            {
                volScalarField rUA = 1.0/UEqn.A();
                surfaceScalarField rUAf(fvc::interpolate(rUA));

                U = rUA*UEqn.H();

                phi = (fvc::interpolate(U) & mesh.Sf())
                    + rUAf*fvc::ddtCorr(U, phi);

                adjustPhi(phi, U, p);


                while (piso.correctNonOrthogonal())
                {
                    // Pressure corrector

                    fvScalarMatrix pEqn
                    (
                        fvm::laplacian(rUA, p) == fvc::div(phi) + particleCloud.ddtVoidfraction()
                    );

                    pEqn.setReference(pRefCell, pRefValue);

                    pEqn.solve(mesh.solver(p.select(piso.finalInnerIter())));

                    if (piso.finalNonOrthogonalIter())
                    {
                        phi -= pEqn.flux();
                    }
                }

                #include "continuityErrs.H"

                U -= rUA*fvc::grad(p);
                U.correctBoundaryConditions();
            }
        }

        laminarTransport.correct();
        turbulence->correct();

        Info << "particleCloud.calcVelocityCorrection() " << endl;
        volScalarField voidfractionNext=mesh.lookupObject<volScalarField>("voidfractionNext");
        particleCloud.calcVelocityCorrection(p,U,phiIB,voidfractionNext);

        fvOptions.correct(U);

        runTime.write();

        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
