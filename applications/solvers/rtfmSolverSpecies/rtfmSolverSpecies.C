/*---------------------------------------------------------------------------*\
    CFDEMcoupling academic - Open Source CFD-DEM coupling

    Contributing authors:
    Thomas Lichtenegger, Gerhard Holzinger
    Copyright (C) 2015- Johannes Kepler University, Linz
-------------------------------------------------------------------------------
License
    This file is part of CFDEMcoupling academic.

    CFDEMcoupling academic is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CFDEMcoupling academic is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with CFDEMcoupling academic.  If not, see <http://www.gnu.org/licenses/>.

Application
    cfdemSolverRecurrence

Description
    Solves a transport equation for a passive scalar on a two-phase solution
    Test-bed for a solver based on recurrence statistics

Rules
    Solution data to compute the recurrence statistics from, needs to
        reside in $CASE_ROOT/dataBase
    Time step data in dataBase needs to be evenly spaced in time

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "singlePhaseTransportModel.H"
#include "turbulentTransportModel.H"
#include "fvOptions.H"

#include "cfdemCloudRec.H"
#include "recBase.H"
#include "recModel.H"

#include "cfdemCloud.H"
#include "clockModel.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    #include "postProcess.H"
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"
    #include "createControl.H"
    #include "createFields.H"
    #include "createFvOptions.H"
    scalar relaxCoeff(0.0);

    cfdemCloudRec<cfdemCloud> particleCloud(mesh);
    recBase recurrenceBase(mesh);

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< "\nCalculating particle trajectories based on recurrence statistics\n" << endl;

    label recTimeIndex(0);
    label stepCounter = 0;
    label recTimeStep2CFDTimeStep = recurrenceBase.recM().recTimeStep2CFDTimeStep();

    while (runTime.run())
    {
        runTime++;

        // do stuff (every lagrangian time step)
        particleCloud.clockM().start(1,"Global");

        Info<< "Time = " << runTime.timeName() << nl << endl;

        particleCloud.clockM().start(2,"Flow");
        #include "TEq.H"
        particleCloud.clockM().stop("Flow");

        stepCounter++;

        if (stepCounter == recTimeStep2CFDTimeStep)
        {
            Info<< "Updating fields at run time " << runTime.timeOutputValue()
                << " corresponding to recTimeIndex " << recTimeIndex << ".\n" << endl;
            recurrenceBase.updateRecFields();
            #include "readFields.H"
            recTimeIndex++;
            stepCounter = 0;
            recTimeStep2CFDTimeStep = recurrenceBase.recM().recTimeStep2CFDTimeStep();
        }

        particleCloud.clockM().start(27,"Output");
        runTime.write();
        particleCloud.clockM().stop("Output");

        particleCloud.clockM().stop("Global");

        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
