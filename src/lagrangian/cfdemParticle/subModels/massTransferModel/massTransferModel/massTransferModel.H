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

    Copyright (C) 2015- Thomas Lichtenegger, JKU Linz, Austria

\*---------------------------------------------------------------------------*/

#ifndef massTransferModel_H
#define massTransferModel_H

#include "fvCFD.H"
#include "cfdemCloudEnergy.H"
#include "probeModel.H"
#include "interpolationCellPoint.H"
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class massTransferModel Declaration
\*---------------------------------------------------------------------------*/

class massTransferModel
{

protected:

    // Protected data
        const dictionary& dict_;

        cfdemCloudEnergy& particleCloud_;

        IOdictionary transportProperties_;

        mutable volScalarField D0Field_;

        mutable volScalarField CsField_;

public:

    //- Runtime type information
    TypeName("massTransferModel");

    // Declare runtime constructor selection table

        declareRunTimeSelectionTable
        (
            autoPtr,
            massTransferModel,
            dictionary,
            (
                const dictionary& dict,
                cfdemCloudEnergy& sm
            ),
            (dict,sm)
        );


    // Constructors

        //- Construct from components
        massTransferModel
        (
            const dictionary& dict,
            cfdemCloudEnergy& sm
        );


    // Destructor

        virtual ~massTransferModel();


    // Selector

        static autoPtr<massTransferModel> New
        (
            const dictionary& dict,
            cfdemCloudEnergy& sm,
            word massTransferType
        );


    // Member Functions

        virtual void addMassContribution(volScalarField&) const = 0;

        virtual void addMassCoefficient(volScalarField&) const = 0;

        virtual void calcMassContribution() = 0;

        virtual void postFlow() {}

        const volScalarField& D0Field() const;

        const volScalarField& CsField() const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
