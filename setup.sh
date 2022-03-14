#!/bin/bash

omnetpath=$(which omnetpp)
parentdir=$(dirname $omnetpath)
parentdir=$(dirname $parentdir)
mapsdir=$parentdir/images/maps
cp ./images/maps/earth.jpg $mapsdir

