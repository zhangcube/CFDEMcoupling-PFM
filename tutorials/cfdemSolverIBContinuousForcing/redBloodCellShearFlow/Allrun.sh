#!/bin/bash
#------------------------------------------------------------------------------
# Allrun script for redBloodCellShearFlow test case
# run redBloodCellShearFlow
# Achuth N. Balachandran Nair - October 2018
#------------------------------------------------------------------------------

source $CFDEM_PROJECT_DIR/etc/functions.sh

#- define variables
casePath="$(dirname "$(readlink -f ${BASH_SOURCE[0]})")"

# check if mesh was built
if [ -f "$casePath/CFD/constant/polyMesh/points" ]; then
    echo "mesh was built before - using old mesh"
else
    echo "mesh needs to be built"
    cd $casePath/CFD
    blockMesh
fi

if [ -f "$casePath/DEM/post/restart/liggghts.restart" ];  then
    echo "LIGGGHTS init was run before - using existing restart file"
else
    #- run serial DEM
    $casePath/DEMrun.sh
fi

#- run parallel CFD-DEM
bash $casePath/parCFDDEMrun.sh
