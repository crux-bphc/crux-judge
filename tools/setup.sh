#! /usr/bin/env bash

echo "Setup: Setting up Virtual Env"

BASEDIR=`dirname $0`/..

if [ ! -d "$BASEDIR/env" ]; then
    virtualenv -q $BASEDIR/env --no-site-packages
    echo "Virtualenv created."
fi

if [ ! -f "$BASEDIR/env/updated" -o $BASEDIR/requirements.pip -nt $BASEDIR/env/updated ]; then
    pip3 install -r $BASEDIR/requirements.pip -E $BASEDIR/env
    touch $BASEDIR/env/updated
    echo "Requirements installed."
fi
