#!/usr/bin/env bash

BASEDIR=`dirname $0`/..

$BASEDIR/bin/setup

source $BASEDIR/env/bin/activate
cd $BASEDIR
export PYTHONPATH=.

exec python $@