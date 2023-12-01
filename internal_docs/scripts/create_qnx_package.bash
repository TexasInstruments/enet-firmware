#!/bin/bash

TMPDIR=/tmp/ethfw-qnx

mkdir $TMPDIR
mkdir $TMPDIR/ethremotecfg
mkdir $TMPDIR/utils

cp -r ethremotecfg/protocol $TMPDIR/ethremotecfg
cp -r ethremotecfg/client $TMPDIR/ethremotecfg
cp -r utils/ethfw_common $TMPDIR/utils

find $TMPDIR -name concerto.mak -exec rm {} \;

echo "ETHFW QNX Package: $TMPDIR"
