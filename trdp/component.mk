#**********************************************************************************************************************/
#
#* @file            component.mk
#*
#* @brief           Component Makefile for the Expressif IDF v3.0
#*
#* @details         To use the TRDP stack with the Expressive Integrated Development Framework,
#*                  this Makefile must be placed inside the component directory. It will be included by
#*                  the main IDF Makefile.
#*
#* @note            Project: TCNOpen TRDP prototype stack
#*
#* @author          Bernd Loehr
#*
#* @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
#*          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#*          Copyright 2018. All rights reserved.
#*
#* $Id$
#
#

COMPONENT_ADD_INCLUDEDIRS := \
src/api \
src/vos/api \
src/vos/esp

COMPONENT_SRCDIRS := \
src/common src/vos/common \
src/vos/esp

CFLAGS += -Wall -DESP32 -I src/vos/esp -I src/api -Wno-unknown-pragmas -Wno-char-subscripts -Wno-format -DL_ENDIAN

