# CMakeLists.txt
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
cmake_minimum_required(VERSION 2.8.3)

# Debug print lines (for development)
#add_definitions(-DPRINT_DEBUG)

# Debug Output activate:
set ( CMAKE_VERBOSE_MAKEFILE true )

# Find the QtWidgets library
find_package(Qt5Xml)

# Add the include directories for the Qt 5 Widgets module to
# the compile lines.
include_directories(${Qt5Widgets_INCLUDE_DIRS})

include(WiresharkPlugin)

# Add compiler flags for building executables (-fPIE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

# Plugin name and version info (major minor micro extra)
set_module_info(trdp_spy 1 3 0 1)

#find_package(Qt5Xml)

set(DISSECTOR_SRC
    packet-trdp_spy.cpp
    trdp_env.cpp
    trdpConfigHandler.cpp
)

set(PLUGIN_FILES
	plugin.c
	${DISSECTOR_SRC}
)

set(CLEAN_FILES
	${PLUGIN_FILES}
)

if (WERROR_COMMON_FLAGS)
	set_source_files_properties(
		${CLEAN_FILES}
		PROPERTIES
		COMPILE_FLAGS ${WERROR_COMMON_FLAGS}
	)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

register_dissector_files(plugin.c
	plugin
	${DISSECTOR_SRC}
)

add_plugin_library(trdp_spy)

target_link_libraries(trdp_spy epan Qt5::Xml)

install(TARGETS trdp_spy
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/@CPACK_PACKAGE_NAME@/plugins/${CPACK_PACKAGE_VERSION} NAMELINK_SKIP
	RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/@CPACK_PACKAGE_NAME@/plugins/${CPACK_PACKAGE_VERSION}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/@CPACK_PACKAGE_NAME@/plugins/${CPACK_PACKAGE_VERSION}
)

file(GLOB DISSECTOR_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.h")
CHECKAPI(
	NAME
	  trdp_spy
	SWITCHES
	  -g abort -g termoutput -build
	SOURCES
	  ${DISSECTOR_SRC}
	  ${DISSECTOR_HEADERS}
)
