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

#include "error.H"
#include "ZehnerSchluenderThermCond.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(ZehnerSchluenderThermCond, 0);

addToRunTimeSelectionTable
(
    thermCondModel,
    ZehnerSchluenderThermCond,
    dictionary
);


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

// Construct from components
ZehnerSchluenderThermCond::ZehnerSchluenderThermCond
(
    const dictionary& dict,
    cfdemCloud& sm
)
:
    thermCondModel(dict,sm),
    propsDict_(dict.subDict(typeName + "Props")),
    partKsFieldName_(propsDict_.lookupOrDefault<word>("partKsFieldName","partKs")),
    partKsField_(const_cast<volScalarField&>(sm.mesh().lookupObject<volScalarField> (partKsFieldName_))),
    voidfractionFieldName_(propsDict_.lookupOrDefault<word>("voidfractionFieldName","voidfraction")),
    voidfraction_(sm.mesh().lookupObject<volScalarField> (voidfractionFieldName_)),
    typeKs_(propsDict_.lookupOrDefault<scalarList>("thermalConductivities",scalarList(1,-1.0))),
    partKsRegName_(typeName + "partKs")
{
    if (typeKs_[0] < 0.0)
    {
        FatalError << "ZehnerSchluenderThermCond: provide list of thermal conductivities." << abort(FatalError);
    }

    if (typeKs_.size() > 1)
    {
        particleCloud_.registerParticleProperty<double**>(partKsRegName_,1);
    }
    else
    {
        partKsField_ *= typeKs_[0];
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

ZehnerSchluenderThermCond::~ZehnerSchluenderThermCond()
{
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void ZehnerSchluenderThermCond::calcThermCond()
{
    const volScalarField& kf0Field_ = kf0Field();
    calcPartKsField();
    scalar A = 0.0;
    scalar B = 0.0;
    scalar C = 0.0;
    scalar k = 0.0;
    scalar InvOnemBoA = 0.0;
    scalar voidfraction = 0.0;
    scalar w = 7.26e-3;

    forAll(partKsField_, cellI)
    {
        voidfraction = voidfraction_[cellI];
        if(voidfraction > 1.0 - SMALL || partKsField_[cellI] < SMALL) partKsField_[cellI] = 0.0;
        else
        {
            scalar kf0 = kf0Field_[cellI];
            A = partKsField_[cellI]/kf0;
            B = 1.25 * Foam::pow((1 - voidfraction) / voidfraction, 1.11);
            InvOnemBoA = 1.0/(1.0 - B/A);
            C = (A - 1) * InvOnemBoA * InvOnemBoA * B/A * log(A/B) - (B - 1) * InvOnemBoA - 0.5 * (B + 1);
            C *= 2.0 * InvOnemBoA;
            k = Foam::sqrt(1 - voidfraction) * (w * A + (1 - w) * C) * kf0;
            partKsField_[cellI] = k / (1 - voidfraction);
        }
    }

    partKsField_.correctBoundaryConditions();

    // if a wallQFactor field is present, use it to scale heat transport through a patch
    if (hasWallQFactor_)
    {
        wallQFactor_.correctBoundaryConditions();
        forAll(wallQFactor_.boundaryField(), patchi)
            partKsField_.boundaryFieldRef()[patchi] *= wallQFactor_.boundaryField()[patchi];
    }
}

void ZehnerSchluenderThermCond::calcPartKsField() const
{
    if (typeKs_.size() <= 1) return;

    if (!particleCloud_.getParticleTypes())
    {
        FatalError << "ZehnerSchluenderThermCond needs data for more than one type, but types are not communicated." << abort(FatalError);
    }

    double**& partKs_ = particleCloud_.getParticlePropertyRef<double**>(partKsRegName_);
    label cellI=0;
    label partType = 0;
    for(int index = 0;index < particleCloud_.numberOfParticles(); ++index)
    {
            cellI = particleCloud_.cellIDs()[index][0];
            if(cellI >= 0)
            {
                partType = particleCloud_.particleType(index);
                // LIGGGGHTS counts types 1, 2, ..., C++ array starts at 0
                partKs_[index][0] = typeKs_[partType - 1];
            }
    }

    partKsField_.primitiveFieldRef() = 0.0;
    particleCloud_.averagingM().resetWeightFields();
    particleCloud_.averagingM().setScalarAverage
    (
        partKsField_,
        partKs_,
        particleCloud_.particleWeights(),
        particleCloud_.averagingM().UsWeightField(),
        NULL
    );
}
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
