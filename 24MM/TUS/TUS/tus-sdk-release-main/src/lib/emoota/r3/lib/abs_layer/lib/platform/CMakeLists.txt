#/* ****************************************************************************
# *
# *         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
# *
# *  The software and information contained herein is the proprietary and
# *  confidential property of Airbiquity Inc. and shall not be disclosed
# *  in whole or in part. Possession, use, reproduction or transfer of
# *  this software and information is prohibited without the express written
# *  permission of Airbiquity Inc.
# *
# *  Copyright © 2018 Airbiquity Inc.  All rights reserved.
# *
# * ****************************************************************************
# */

# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(${EMOOTA3}platform LANGUAGES C)

add_library(${EMOOTA3}platform src/platformconfig.c)
set_property(TARGET ${EMOOTA3}platform PROPERTY POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(${EMOOTA3}platform
        PUBLIC
            ./include
        )
