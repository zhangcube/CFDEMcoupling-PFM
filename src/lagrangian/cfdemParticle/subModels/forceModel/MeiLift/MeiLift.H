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

    This function is based on the derivation in R. Mei,
    An approximate expression for shear lift force on a spherical  particle at a
    finite Reynolds number,
    Int. J. Multiph. Flow 18 (1992) 145–147

    The data for this functions is based on J.B. Mclaughlin,
    Inertial migration of a small sphere in linear shear flows,
    Journal of Fluid Mechanics. 224 (1991) 261-274.

    The second order terms are based on E. Loth and A. J. Dorgan,
    An equation of motion for particles of finite Reynolds number and size,
    Environ. Fluid Mech. 9 (2009) 187–206
    and can be added to the lift coefficient if desired
    (contribution from RQ)

    - including interpolation of the velocity to the particle position (optional)
    - including output to file for testing/data analysis (optional)


Class
    MeiLift

SourceFiles
    MeiLift.C

\*---------------------------------------------------------------------------*/

#ifndef MeiLift_H
#define MeiLift_H

#include "forceModel.H"
#include "IOList.H"
#include "interpolationCellPoint.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class MeiLift Declaration
\*---------------------------------------------------------------------------*/

class MeiLift
:
    public forceModel
{
private:
    dictionary propsDict_;

    word velFieldName_;

    const volVectorField& U_;

    bool useShearInduced_;

    bool useSpinInduced_;

    bool combineShearSpin_;

public:

    //- Runtime type information
    TypeName("MeiLift");


    // Constructors

        //- Construct from components
        MeiLift
        (
            const dictionary& dict,
            cfdemCloud& sm
        );

    // Destructor

        ~MeiLift();


    // Member Functions
        void setForce() const;

};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
