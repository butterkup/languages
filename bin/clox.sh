#!/usr/bin/env bash

declare -A CLOX_DEFS
declare -A CLOX_ACTIVE_DEFS

CLOX_DEFS["stack"]=CLOX_STACK_TRACE
CLOX_DEFS["scan"]=CLOX_SCAN_TRACE
CLOX_DEFS["inst"]=CLOX_INST_TRACE
CLOX_DEFS["odel"]=CLOX_ODEL_TRACE

function _clox_valid_macro() {
  [ -z "$1" ] && return 1
  for valid in "${!CLOX_DEFS[@]}"; do
    [ "$valid" == "$1" ] && return 0
  done
  return 2
}

function _clox_export_defs() {
  local MACRO=""
  for def in "${CLOX_ACTIVE_DEFS[@]}"; do
    MACRO="$MACRO $def"
  done
  MACRO="${MACRO## }"
  MACRO="${MACRO%% }"
  export CLOX_MACRO="$MACRO"
}

function _clox_usage_macro() {
  echo "Unexpected macro '$1', valid macros are: ${!CLOX_DEFS[@]}"
}

function _clox_define() {
  for macro; do
    if _clox_valid_macro "$macro"; then
      CLOX_ACTIVE_DEFS["$macro"]="-D${CLOX_DEFS["$macro"]}"
    else
      _clox_usage_macro "$macro"
    fi
  done
  _clox_export_defs
}

function _clox_undefine() {
  for macro; do
    if _clox_valid_macro "$macro"; then
      unset CLOX_ACTIVE_DEFS["$macro"]
    else
      _clox_usage_macro "$macro"
    fi
  done
  _clox_export_defs
}

function _clox_macro_map() {
  echo "Mapping from macro key to macro definition."
  for macro in "${!CLOX_DEFS[@]}"; do
    echo "  '$macro': '${CLOX_DEFS["$macro"]}'"
  done
}

function _clox_defined() {
  echo "Defined: CLOX_MACRO='$CLOX_MACRO'"
  echo "ActiveMacros [${#CLOX_ACTIVE_DEFS[@]}]: ${!CLOX_ACTIVE_DEFS[@]}"
}

function _clox_defs_main() {
  [ $# -eq 0 ] && set -- --defs
  while [ "$#" -gt 0 ]; do
    case "$1" in
    -h | --help)
      echo "Valid options:"
      echo "  -h | --help   Display this help message and exit."
      echo "  -c | --clr    Clear all definitions of CLOX_MACRO and exit."
      echo "  -m | --map    Show all macro key to macro def mapping."
      echo "  -d | --defs   Show all current definition for CLOX_MACRO"
      echo "If no options are passed, -d | --defs is assumed."
      return 0
      ;;
    -m | --map)
      _clox_macro_map
      return 0
      ;;
    -d | --defs)
      _clox_defined
      return 0
      ;;
    -c | --clr | clr)
      export CLOX_MACRO=""
      CLOX_ACTIVE_DEFS=()
      return 0
      ;;
    *)
      echo "Error: $1 Unknown Option: -h to get valid options"
      return 0
      ;;
    esac
    shift 1
  done
}

alias clox_h=_clox_defs_main
alias clox_def=_clox_define
alias clox_udef=_clox_undefine
