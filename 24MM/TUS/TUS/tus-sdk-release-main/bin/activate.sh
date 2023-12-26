#!/bin/bash

#TMC CONFIDENTIAL
#$TUSLibId$
#Copyright (C) 2022 TOYOTA MOTOR CORPORATION
#All Rights Reserved.

# when executed by source, in Bash $0 is "bash". In other shell ,for example zsh,  $0 is a script file path.
pushd "$(dirname "${BASH_SOURCE:-$0}")" >/dev/null || exit
# update environment variables
TUS_SDK="$(pwd)/../"
export TUS_SDK
TUS_CMD_DIR="$TUS_SDK/bin"
export PATH="$PATH:$TUS_CMD_DIR"
popd >/dev/null || exit
