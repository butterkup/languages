#!/usr/bin/env bash

MYPATH="$1"

if [ ! -e "$MYPATH" ]; then
  echo I do not know where I am, pass my directory path as first argument.
else
  ROOT="$(realpath "$MYPATH")"
  PYPATH="$(dirname "$ROOT")"
  export PYTHONPATH="$PYPATH:$PYTHONPATH"
  export PATH="$ROOT:$PATH"
fi
