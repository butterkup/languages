#!/usr/bin/env bash

__tmp_MYPATH="$1"

if [ ! -e "$__tmp_MYPATH" ]; then
  echo I do not know where I am, pass my directory path as first argument.
else
  __tmp_ROOT="$(realpath "$__tmp_MYPATH")"
  __tmp_PYPATH="$(dirname "$__tmp_ROOT")"
  if [ ! -v __clox_PATH_ALREADY_SETUP ]; then
    export PYTHONPATH="$__tmp_PYPATH:$PYTHONPATH"
    export PATH="$__tmp_ROOT:$PATH"
  else
    __clox_PATH_ALREADY_SETUP=true
    exit 1
  fi
fi

unset __P_clox_defs __P_active_clox_defs CLOX_MACRO

declare -x CLOX_MACRO
declare -A __P_clox_defs
declare -A __P_active_clox_defs

__P_clox_defs["stack"]=CLOX_STACK_TRACE
__P_clox_defs["scan"]=CLOX_SCAN_TRACE
__P_clox_defs["inst"]=CLOX_INST_TRACE
__P_clox_defs["odel"]=CLOX_ODEL_TRACE

function _in_array() {
  local value="$1"
  shift 1
  for item; do
    if [ "$value" == "$item" ]; then
      return 0
    fi
  done
  return 1
}

function _valid_clox_macro() {
  [ -z "$1" ] && return 1
  _in_array "$1" ${!__P_clox_defs[@]}
}

function _export_clox_macro() {
  local MACRO=""
  for macro in "${__P_active_clox_defs[@]}"; do
    MACRO="$([ -z "$MACRO" ] && echo "$macro" || echo "$MACRO $macro")"
  done
  export CLOX_MACRO="$MACRO"
}

function _valid_clox_macros() {
  echo "Valid Macros: ${!__P_clox_defs[@]}"
}

function clox_def() {
  for macro; do
    if _valid_clox_macro "$macro"; then
      __P_active_clox_defs["$macro"]="-D${__P_clox_defs[$macro]}"
    else
      echo >&2 "Unknown definition: '$macro'."
      return 2
    fi
  done
  _export_clox_macro
}

function clox_undef() {
  for macro; do
    if _valid_clox_macro "$macro"; then
      unset __P_active_clox_defs["$macro"]
    else
      echo >&2 "Unknown definition: '$macro'."
      return 2
    fi
  done
  _export_clox_macro
}

function clox_mac() {
  case "$1" in
  clr) CLOX_MACRO=""; __P_active_clox_defs=();;
  show | "") echo "CLOX_MACRO='$CLOX_MACRO'" ;;
  \? | help) _valid_clox_macros ;&
  *) echo "Valid Command: 'clr' 'show' '?' 'help' ''" ;;
  esac
}

alias mlox="make -sC '$(realpath "$__tmp_MYPATH")/../c_lox'"

unset __tmp_MYPATH __tmp_ROOT __tmp_PYPATH
