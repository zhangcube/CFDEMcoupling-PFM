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

#include "error.H"
#include "particle.H"
#include "IOModel.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(IOModel, 0);

defineRunTimeSelectionTable(IOModel, dictionary);

// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

int IOModel::dumpDEMdata() const
{
    return -1;
}

bool IOModel::dumpNow() const
{
    //bool dmp(false);
    //if (time_.value()+SMALL > time_.endTime().value()-time_.deltaT().value() || time_.outputTime())
    //    dmp=true;

    return time_.outputTime();
}

fileName IOModel::createTimeDir(const fileName& path) const
{
    fileName timeDirPath(path/time_.timeName());
    mkDir(timeDirPath,0777);
    return timeDirPath;
}

fileName IOModel::createLagrangianDir(const fileName& path) const
{
    fileName lagrangianDirPath(path/"lagrangian");
    mkDir(lagrangianDirPath,0777);
    fileName cfdemCloudDirPath(lagrangianDirPath/"cfdemCloud1");
    mkDir(cfdemCloudDirPath,0777);
    return cfdemCloudDirPath;
}

fileName IOModel::buildFilePath(const word& dirName) const
{
    // create file structure
    fileName path("");
    if(parOutput_)
    {
        path = fileName(particleCloud_.mesh().time().path()/particleCloud_.mesh().time().timeName()/dirName/"particleCloud");
        mkDir(path,0777);
    }
    else
    {
        path = fileName("."/dirName);
        mkDir(path,0777);
        mkDir(path/"constant",0777);
        OFstream stubFile(path/"particles.foam");
    }
    return path;
}

void IOModel::streamDataToPath(const fileName& path, const double* const* array,int nPProc,const word& name,const word& type,const word& className) const
{
    OFstream fileStream(path/name);

    fileStream
        << "/*--------------------------------*- C++ -*----------------------------------*\\" << nl
        << "  =========                 |" << nl
        << "  \\\\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox" << nl
        << "   \\\\    /   O peration     | Website:  https://openfoam.org" << nl
        << "    \\\\  /    A nd           | Version:  " << FOAMversion << nl
        << "     \\\\/     M anipulation  |" << nl
        << "\\*---------------------------------------------------------------------------*/" << nl
        << "FoamFile" << nl
        << "{" << nl
        << "    version     " << fileStream.version() << ";" << nl
        << "    format      " << fileStream.format() << ";" << nl
        << "    class       " << className << ";" << nl
        << "    location    " << 0 << ";" << nl
        << "    object      " << name << ";" << nl
        << "}" << nl
        << "// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //" << nl << nl;

    fileStream << nPProc << "\n";

    if (type == "origProcId")
    {
        if (nPProc > 0) fileStream << "{0}" << "\n";
        else fileStream << "{}" << "\n";
        return;
    }
#if OPENFOAM_VERSION_MAJOR > 4
    else if (barycentricOutput_ && type == "position")
    {
        // Force construction of face-diagonal decomposition before construction
        // of particle which uses parallel transfers.
        (void)particleCloud_.mesh().tetBasePtIs();
    }
#endif

    fileStream << token::BEGIN_LIST << nl;

    const int * const* cellIDs = particleCloud_.cellIDs();
    for (int index = 0; index < particleCloud_.numberOfParticles(); ++index)
    {
        if (cellIDs[index][0] > -1) // particle Found
        {
            if (type == "scalar")
            {
                fileStream << array[index][0] << " \n";
            }
            else if (type == "position")
            {
#if OPENFOAM_VERSION_MAJOR > 4
                if (barycentricOutput_)
                {
                    particle part
                    (
                        particleCloud_.mesh(),
                        vector(array[index][0],array[index][1],array[index][2]),
                        cellIDs[index][0]
                    );

                    part.writePosition(fileStream);
                    fileStream << nl;
                }
                else
#endif
                {
                    fileStream << "( " << array[index][0] << " " << array[index][1] << " " << array[index][2] << " ) " << cellIDs[index][0] << nl;
                }
            }
            else if (type == "vector")
            {
                fileStream << "( "<< array[index][0] << " " << array[index][1] << " " << array[index][2] << " ) " << " \n";
            }
            else if (type == "label")
            {
                fileStream << index << " \n";
            }
        }
    }

    fileStream << token::END_LIST << nl;
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

// Construct from components
IOModel::IOModel
(
    const dictionary& dict,
    cfdemCloud& sm
)
:
    dict_(dict),
    particleCloud_(sm),
    time_(sm.mesh().time()),
    parOutput_(true),
    barycentricOutput_(true)
{
    if (
            particleCloud_.dataExchangeM().type()=="oneWayVTK" ||
            dict_.found("serialOutput")
       )
    {
        parOutput_ = false;
        Warning << "IO model is in serial write mode, only data on proc 0 is written" << endl;
    }

    if (dict_.found("cartesianOutput"))
    {
        barycentricOutput_ = false;
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

IOModel::~IOModel()
{}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
