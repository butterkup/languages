#!/usr/bin/env bash

LOCATION="$HOME/Content/work/languages/bin"

if ! ${__clox_PATH_ALREADY_SETUP:-false}; then
  export PYTHONPATH="$(dirname "$LOCATION"):$PYTHONPATH"
  export PATH="$LOCATION:$PATH"
  __clox_PATH_ALREADY_SETUP=true
fi

alias mlox="make -sC '$LOCATION/../c_lox'"
source "$LOCATION/clox.sh"

unset LOCATION
