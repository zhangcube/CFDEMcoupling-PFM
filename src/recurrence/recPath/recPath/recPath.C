/*---------------------------------------------------------------------------*\
    CFDEMcoupling academic - Open Source CFD-DEM coupling

    Contributing authors:
    Thomas Lichtenegger
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
\*---------------------------------------------------------------------------*/

#include "error.H"
#include "recPath.H"
#include "recModel.H"
#include <unistd.h>


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(recPath, 0);

defineRunTimeSelectionTable(recPath, dictionary);


// * * * * * * * * * * * * * private Member Functions  * * * * * * * * * * * * //

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

// Construct from components
recPath::recPath
(
    const dictionary& dict,
    recBase& base
)
:
    base_(base),
    recProperties_(dict),
    verbose_(dict.lookupOrDefault<Switch>("verbose", false)),
    virtualTimeIndexList_( base_.recM().virtualTimeIndexList() )
{
}

// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

recPath::~recPath()
{}

// * * * * * * * * * * * * * public Member Functions  * * * * * * * * * * * * //


// * * * * * * * * * * * * * private Member Functions  * * * * * * * * * * * * //

void recPath::getRecPath()
{
    label numRecIntervals = 0;

    if(Pstream::master())
    {
        computeRecPath();
        numRecIntervals = virtualTimeIndexList_.size();
    }

    Pstream::scatter(numRecIntervals);

    if(!Pstream::master())
    {
        virtualTimeIndexList_.setSize(numRecIntervals);
    }

    Pstream::scatter(virtualTimeIndexList_);

    if(verbose_)
    {
        Info << "\nRecurrence path communicated to all processors.\n" << endl;
    }
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
