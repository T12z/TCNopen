#!/bin/sh
#
# Copyright     This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
#               If a copy of the MPL was not distributed with this file, 
#               You can obtain one at http://mozilla.org/MPL/2.0/.
#               Copyright Alstom Transport SA or its subsidiaries and others, 2010-2022. All rights reserved.
#
# Component     SDTv2 Library
#
# File          make_macmyra.sh
#
# Requirements  NA
#
# Abstract      Builds the HMI Linux variant
#

SDT2VERSION=2.3.0
export PATH=/opt/wrl201/gnu/4.1-wrlinux-2.0/x86-linux2/bin:$PATH
make LINUX_HMI500wrl_config
make
make DEBUG=TRUE

#create DL2
mkdir -p output/linux-x86-hmi500wrl-rel/dl2/lib
cp output/linux-x86-hmi500wrl-rel/libsdt.so.* output/linux-x86-hmi500wrl-rel/dl2/lib
cd output/linux-x86-hmi500wrl-rel/dl2/lib; 
rm *.md5; 
rm *.sha1; 
ln -s libsdt.so.$SDT2VERSION libsdt.so.2;
cd .. 
#use only tar without gzip
tar cf sdtv2.tar *; 
makedlu sdtv2.tar sdtv2 2.3.0.1 DLU_TYPE_LINUX_TAR TPATH /usr/local/sdtv2.tar
