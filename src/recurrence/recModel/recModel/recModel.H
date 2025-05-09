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

#ifndef recModel_H
#define recModel_H

#include "recBase.H"
#include "fvCFD.H"
#include "HashTable.H"
#include "labelPair.H"
#include "OFstream.H"

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class recModel Declaration
\*---------------------------------------------------------------------------*/

class recModel
{
protected:

    // Protected data
    recBase& base_;

    const dictionary& recProperties_;

    IOdictionary controlDict_;

    IOdictionary lastJumpTime_;

    bool verbose_;

    const wordList volScalarFieldNames_;
    const wordList volVectorFieldNames_;
    const wordList surfaceScalarFieldNames_;

    scalar startTime_;
    scalar endTime_;
    scalar timeStep_;

    scalar recStartTime_;
    scalar recEndTime_;
    scalarList recTimeStep_;
    labelList recTimeStep2CFDTimeStep_;
    label totRecSteps_;

    label sequenceStart;
    label sequenceEnd;

    label currDataBase_;
    label currDataBaseNext_;
    label virtualStartIndex;
    label virtualTimeIndex;
    label virtualTimeIndexNext;
    // list of pairs (start value, end value)
    labelPairList virtualTimeIndexList_;
    label virtualTimeIndexListPos_;

    bool writeRecMat_;
    word recMatName_;
    OFstream pathFile_;

    virtual void readFieldSeries() = 0;
    virtual void init();

public:

    friend class recBase;

    //- Runtime type information
    TypeName("recModel");

    // Declare runtime constructor selection table

        declareRunTimeSelectionTable
        (
            autoPtr,
            recModel,
            dictionary,
            (
                const dictionary& dict,
                recBase& base
            ),
            (dict,base)
        );


    // Constructors

        //- Construct from components
        recModel
        (
            const dictionary& dict,
            recBase& base
        );


    // Destructor

        virtual ~recModel();


    // Selector

        static autoPtr<recModel> New
        (
            const dictionary& dict,
            recBase& base
        );


    // Member Functions

    /*virtual void exportVolScalarField(word, volScalarField&) const = 0;
    virtual void exportVolVectorField(word, volVectorField&) const = 0;
    virtual void exportSurfaceScalarField(word, surfaceScalarField&) const = 0;*/

    virtual void exportVolScalarFieldAve(word, volScalarField&, label = 0)
    {}
    virtual void exportVolVectorFieldAve(word, volVectorField&, label = 0)
    {}
    virtual void exportSurfaceScalarFieldAve(word, surfaceScalarField&, label = 0)
    {}

    virtual tmp<volScalarField> exportVolScalarFieldAve(word, label = 0)
    {
        return volScalarField::null();
    }
    virtual tmp<volVectorField> exportVolVectorFieldAve(word, label = 0)
    {
        return volVectorField::null();
    }
    virtual tmp<surfaceScalarField> exportSurfaceScalarFieldAve(word, label = 0)
    {
        return surfaceScalarField::null();
    }

    virtual void exportVolScalarField(word, volScalarField&)  = 0;
    virtual void exportVolVectorField(word, volVectorField&)  = 0;
    virtual void exportSurfaceScalarField(word, surfaceScalarField&)  = 0;

    virtual const volScalarField& exportVolScalarField(word, label)  = 0;
    virtual const volVectorField& exportVolVectorField(word, label)  = 0;
    virtual const surfaceScalarField& exportSurfaceScalarField(word, label)  = 0;

    virtual PtrList<volScalarField>& exportVolScalarFieldList(word)  = 0;
    virtual PtrList<volVectorField>& exportVolVectorFieldList(word)  = 0;
    virtual PtrList<surfaceScalarField>& exportSurfaceScalarFieldList(word)  = 0;

  //  virtual tmp<surfaceScalarField> exportAveragedSurfaceScalarField(word, scalar, label index = -1) = 0;
    virtual void exportAveragedVolVectorField(volVectorField&, word, scalar, label index = -1) const = 0;

    virtual SymmetricSquareMatrix<scalar>& recurrenceMatrix() = 0;

    virtual const HashTable<label,word>& timeIndexList() const = 0;

    labelPairList& virtualTimeIndexList();

    virtual label lowerSeqLim() const = 0;
    virtual label upperSeqLim() const = 0;

    label currDataBase() const
    {
        return currDataBase_;
    }

    label currDataBaseNext() const
    {
        return currDataBaseNext_;
    }

    bool endOfPath() const;

    virtual label numIntervals() const
    {
        return 1;
    }

    virtual label numRecFields() const = 0;
    virtual label numRecFields(label i) const
    {
        return 0;
    }

    virtual label numDataBaseFields() const = 0;

    label totRecSteps() const;

    scalar recStartTime() const
    {
        return recStartTime_;
    }

    scalar recEndTime() const
    {
        return recEndTime_;
    }

    scalar recTimeStep() const
    {
        return recTimeStep_[currDataBase_];
    }

    label recTimeStep2CFDTimeStep() const
    {
        return recTimeStep2CFDTimeStep_[currDataBase_];
    }

    virtual void updateRecFields() = 0;
    virtual label currentTimeIndex() const = 0;

    virtual void writeRecMatrix() const = 0;

    void writeRecPath();


    const wordList volScalarFieldNames()
    {
        return volScalarFieldNames_;
    }

    const wordList volVectorFieldNames()
    {
        return volVectorFieldNames_;
    }

    const wordList surfaceScalarFieldNames()
    {
        return surfaceScalarFieldNames_;
    }

    void writeRecPathLastInterval();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
