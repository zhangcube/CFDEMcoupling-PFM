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
\*---------------------------------------------------------------------------*/

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

inline Foam::label Foam::lruRecModel::getVolScalarFieldIndex(word fieldname, label index) const
{
    if(index < 0 || index > numRecFields_ - 1)
    {
        FatalError<<"lruRecModel: Index out of bounds for volScalarField with name "
            << fieldname << abort(FatalError);
    }

    label fieldI(-1);

    for(int i=0; i<volScalarFieldNames_.size(); i++)
    {
        if(volScalarFieldNames_[i].match(fieldname))
        {
            fieldI = i;
        }
    }

    if (fieldI == -1)
    {
        FatalError<<"lruRecModel: Could not find volScalarField with name "
            << fieldname << abort(FatalError);
    }

    return fieldI;
}

inline Foam::label Foam::lruRecModel::getVolVectorFieldIndex(word fieldname, label index) const
{
    if(index < 0 || index > numRecFields_ - 1)
    {
        FatalError<<"lruRecModel: Index out of bounds for volVectorField with name "
            << fieldname << abort(FatalError);
    }

    label fieldI(-1);

    for(int i=0; i<volVectorFieldNames_.size(); i++)
    {
        if(volVectorFieldNames_[i].match(fieldname))
        {
            fieldI = i;
        }
    }

    if (fieldI == -1)
    {
        FatalError<<"lruRecModel: Could not find volVectorField with name "
            << fieldname << abort(FatalError);
    }

    return fieldI;
}

inline Foam::label Foam::lruRecModel::getSurfaceScalarFieldIndex(word fieldname, label index) const
{
    if(index < 0 || index > numRecFields_ - 1)
    {
        FatalError<<"lruRecModel: Index out of bounds for surfaceScalarField with name "
            << fieldname << abort(FatalError);
    }

    label fieldI(-1);

    for(int i=0; i<surfaceScalarFieldNames_.size(); i++)
    {
        if(surfaceScalarFieldNames_[i].match(fieldname))
        {
            fieldI = i;
        }
    }

    if (fieldI == -1)
    {
        FatalError<<"lruRecModel: Could not find surfaceScalarField with name "
            << fieldname << abort(FatalError);
    }

    return fieldI;
}
