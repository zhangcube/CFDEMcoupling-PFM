/*---------------------------------------------------------------------------*\
License

    This is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This code is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with this code.  If not, see <http://www.gnu.org/licenses/>.

    Copyright (C) 2018- Mathias Vångö, JKU Linz, Austria

Application
    cfdemSolverMultiphaseScalar

Description
    CFD-DEM solver for n incompressible fluids which captures the interfaces and
    includes surface-tension and contact-angle effects for each phase. It is based
    on the OpenFOAM(R)-4.x solver multiphaseInterFoam but extended to incorporate
    DEM functionalities from the open-source DEM code LIGGGHTS.

    Turbulence modelling is generic, i.e. laminar, RAS or LES may be selected.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "multiphaseMixtureScalar.H"
#include "turbulentTransportModel.H"
#include "pimpleControl.H"
#include "fvOptions.H"
#include "CorrectPhi.H"

#include "cfdemCloudEnergy.H"
#include "implicitCouple.H"
#include "clockModel.H"
#include "smoothingModel.H"
#include "forceModel.H"
#include "thermCondModel.H"
#include "diffCoeffModel.H"
#include "energyModel.H"
#include "massTransferModel.H"


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
#if OPENFOAM_VERSION_MAJOR >= 6
    FatalError << "cfdemSolverMultiphase requires OpenFOAM 4.x or 5.x to work properly" << exit(FatalError);
#endif

    #include "postProcess.H"
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"
    #include "createControl.H"
    #include "initContinuityErrs.H"
    #include "createFields.H"
    #include "createFvOptions.H"
    #include "correctPhi.H"
    #include "CourantNo.H"

    turbulence->validate();

    // create cfdemCloud
    cfdemCloudEnergy particleCloud(mesh);

    #include "additionalChecks.H"

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< "\nStarting time loop\n" << endl;

    while (runTime.loop())
    {
        #include "CourantNo.H"
        #include "alphaCourantNo.H"

        particleCloud.clockM().start(1,"Global");

        Info<< "Time = " << runTime.timeName() << nl << endl;

        particleCloud.clockM().start(2,"Coupling");
        bool hasEvolved = particleCloud.evolve(voidfraction,Us,U);

        if(hasEvolved)
        {
            particleCloud.smoothingM().smoothen(particleCloud.forceM(0).impParticleForces());
        }

        Info << "update Ksl.internalField()" << endl;
        Ksl = particleCloud.momCoupleM(0).impMomSource();
        Ksl.correctBoundaryConditions();

       //Force Checks
       vector fTotal(0,0,0);
       vector fImpTotal = sum(mesh.V()*Ksl.internalField()*(Us.internalField()-U.internalField())).value();
       reduce(fImpTotal, sumOp<vector>());
       Info << "TotalForceExp: " << fTotal << endl;
       Info << "TotalForceImp: " << fImpTotal << endl;

        #include "solverDebugInfo.H"
        particleCloud.clockM().stop("Coupling");

        particleCloud.clockM().start(26,"Flow");

        if(particleCloud.solveFlow())
        {
            mixture.solve();
            rho = mixture.rho();
            rhoEps = rho * voidfraction;

            #include "EEqn.H"
            #include "CEqn.H"

             // --- Pressure-velocity PIMPLE corrector loop
            while (pimple.loop())
            {
                #include "UEqn.H"

                // --- Pressure corrector loop
                while (pimple.correct())
                {
                    #include "pEqn.H"
                }

                if (pimple.turbCorr())
                {
                    turbulence->correct();
                }
            }
        }
        else
        {
            Info << "skipping flow solution." << endl;
        }

        particleCloud.clockM().start(31,"postFlow");
        particleCloud.postFlow();
        particleCloud.clockM().stop("postFlow");

        runTime.write();

        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;

        particleCloud.clockM().stop("Flow");
        particleCloud.clockM().stop("Global");
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
