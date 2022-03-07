#!/bin/sh
#
# Copyright     This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
#               If a copy of the MPL was not distributed with this file, 
#               You can obtain one at http://mozilla.org/MPL/2.0/.
#               Copyright Alstom Transport SA or its subsidiaries and others, 2010-2022. All rights reserved.
#
# Component     SDTv2 Library
#
# File          makelint.sh
#
# Requirements  NA
#
# Abstract      Lint all SW variants in one go
#

make INTEGRITY_config
make lint

make LINUX_HMI500wrl_config
make lint

make VXWORKS_68K_CSS1_config
make lint

make VXWORKS_HMI411_config
make lint

make VXWORKS_PPC_CSS3_GW-C_config
make lint

make VXWORKS_PPC_CSS3_CCUO_config
make lint

make VXWORKS_PPC_CSS3_DCU2_config
make lint

