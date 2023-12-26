#!/bin/bash

#TMC CONFIDENTIAL
#$TUSLibId$
#Copyright (C) 2022 TOYOTA MOTOR CORPORATION
#All Rights Reserved.

function usage {
    cat <<EOF
Usage: tus delta [OPTIONS] [COMMAND]

Description:
  TUS-SDK delta update command

Options:
  -h         print help message

Commands:
    diff       create differential data
    patch      apply patch
EOF
}

function usage_diff {
    cat <<EOF
Usage: tus delta diff [OPTIONS] SRC_FILE DST_FILE DELTA_FILE

Description:
  Create DELTA_FILE from SRC_FILE and DST_FILE.

Options:
  -h                      print help message
  --algorithm ALGORITHM   specify delta algorithm (default:bsdiff)
EOF
}

function usage_patch {
    cat <<EOF
Usage: tus delta patch [OPTIONS] SRC_FILE PATCHED_FILE DELTA_FILE

Description:
  Apply DELTA_FILE to SRC_FILE, to create PATCHED_FILE.

Options:
  -h                      print help message
  --algorithm ALGORITHM   specify delta algorithm (default:bsdiff)
EOF
}

case $1 in
-h)
    usage
    exit 0
    ;;
diff|patch)
    subcommand="$1"
shift
    ;;
*)
    usage
    exit 1
    ;;
esac
algorithm="bsdiff"

case $1 in
-h)
    usage_$subcommand
    exit 0
    ;;
--algorithm)
    case $2 in
    bsdiff)
        ;;
    ## other algorithm (= sub directory name)
    ## ...
    *)
        echo "$2 : no such algorithm." >&2
        exit 1
        ;;
    esac
    algorithm=$2
    shift 2
    ;;
-*)
    usage_$subcommand
    exit 1
    ;;
esac

if [ $# -ne 3 ]; then
    usage_$subcommand
    exit 1
fi

# echo $0
`dirname $0`/$algorithm/$subcommand.sh $*
