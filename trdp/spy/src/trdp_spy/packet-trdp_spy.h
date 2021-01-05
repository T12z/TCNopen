/******************************************************************************/
/**
 * @file            packet-trdp_spy.h
 *
 * @brief           Interface between Wireshark and the TRDP anaylsis module
 *
 * @details
 *
 * @note            Project: TRDP SPY
 *
 * @author          Florian Weispfenning, Bombardier Transportation
 * @author          Thorsten Schulz, Universität Rostock
 *
 * @copyright       This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * @copyright       Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 * @copyright       Copyright Universität Rostock, 2019 (substantial changes leading to GLib-only version and update to v2.0, Wirshark 3.)
 *
 * $Id$
 *
 */

/**
 * @mainpage TRDP-SPY
 *
 * @section TRDP_intro Introduction
 *
 * @subsection TRDP_purpose Purpose
 * As part of the IP-Train project, two new protocols namely TRDP-PD (Process Data) and
 * TRDP-MD (Message Data) are intended to be supported by the Wireshark tool. The support
 * is envisaged to be made available in the form of a plug-in.
 *
 * The existing GUI of the Wireshark is not modified. The plug-in TRDP-SPY
 * shall be available as a DLL for Windows platform and shared library for TRDP-spy for Linux
 * platform.
 *
 * @subsection TRDP_audience Intended Audience
 * The TRDP-SPY will be used primarily by TRDP Engineers.
 *
 *
 * @section TRDP_design Design Description
 *
 * @subsection TRDP_system System
 * TRDP Wire Protocol Analysis tool (TRDP-SPY) shall provide qualitative and quantitative
 * analysis of TRDP streams, in order to verify system behaviour during qualification tests (level
 * 2 and level 3) and provide help in problem analysis during train integration and debugging.
 *
 * @subsection TRDP_opEnv Operational Environment
 * The plug-in shall be compatible with Windows and Linux implementation of Wireshark.
 * Standard behavior of Wireshark for all other protocols than WP shall not be influenced in
 * any way by the TRDPWP analysis plug-in.
 *
 * The plug-in shall be delivered as a DLL (Windows),
 * shared Library (.so files for Linux) along with the plugin source.
 *
 * @section TRDP_devWin Development Environment for Windows
 *
 * Following specifications are used for development of the TRDP PD and TRDP MD plug-in for Wireshark.
 * @li Operating System: Windows
 * @li Tool : Wireshark V3.0.1
 * @li Programming Language: C
 * @li TRDP Wire Protocol
 *
 * @subsection TRDP_compileWin Steps to compile for Windows
 * Prerequisites:
 * @li @c source from repository
 * @li Follow the README.txt in the root source folder
 * @li and check the online guide https://www.wireshark.org/docs/wsdg_html_chunked/ChSetupWin32.html#ChWin32Build
 *
 * This will generate the @c trdp_spy.dll or @c trdp_spy.so
 *
 * @section TRDP_devLinux Development Environment for Linux
 * Following specifications are used for development of the TRDP PD and TRDP MD plug-in for Wireshark.
 * @li Operating System: GNU/Linux Debian 10
 * @li Tool : Wireshark v2.6.8, v3.0.1, v3.2
 * @li Programming Language: C
 *
 * @subsection TRDP_compLinux Steps to compile and install Wireshark on Linux:
 * Prerequisites:
 * @li @c source from repository
 * @li Follow the README.txt in the root source folder
 *
 *
 * @section TRDPinterface Interface
 *
 * The plug-in shall be delivered as a DLL i.e. trdp_spy.dll for Windows platform and shared library @c trdp_spy.so
 * files for Linux platform. For Application Data decoding the @c TRDP_config.xml file is required that contains the
 * details of the Data-sets corresponding to each frame that is captured or logged by Wireshark.
 *
 * Overall interface of the system can be explained as shown in the figure below:

 * @image html interfaceDiagram.png "Interface Diagram"
 * @image latex interfaceDiagram.png "Interface Diagram" width=17cm
 *
 * @section TRDPusecase Usecase
 *
 * The TRDP-SPY plugin is integrated into Wireshark as described:
 *
 * @image html seqDiagram.png "Live Functionality Sequence Diagram"
 * @image latex seqDiagram.png "Live Functionality Sequence Diagram" width=10cm
 *
 * On startup the plugin is registered in Wireshark, so the corresponding TCP and UDP packets are sent to this plugin.
 * Each fitting packet is analyzed by the @c trdp_dissect .
 *
 */


 /**
 * @addtogroup Wireshark
 * @{
 */

#ifdef DOXYGEN

/** @fn void proto_register_trdp (void)
 *
 * @brief start analysing TRDP packets
 *
 * Register the protocol with Wireshark
 * this format is require because a script is used to build the C function
 *  that calls all the protocol registration.
 */
void proto_register_trdp (void);


/** @fn void proto_reg_handoff_trdp(void)
 *
 * @brief Called, if the analysis of TRDP packets is stopped.
 *
 * If this dissector uses sub-dissector registration add a registration routine.
 *  This exact format is required because a script is used to find these routines
 *  and create the code that calls these routines.
 *
 *  This function is also called by preferences whenever "Apply" is pressed
 *  (see prefs_register_protocol above) so it should accommodate being called
 *  more than once.
 */
void proto_reg_handoff_trdp(void);

/**@fn void dissect_trdp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
 *
 * @brief
 * Code to analyse the actual TRDP packet
 *
 * @param tvb               buffer
 * @param pinfo             info for the packet
 * @param tree              to which the information are added
 *
 * @return nothing
 */
void dissect_trdp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

#endif

/** @} */
