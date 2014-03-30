# Copyright 2011 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.



cmake_minimum_required(VERSION 2.6)
project(gr-osmosdr CXX C)

set(CMAKE_BUILD_TYPE Debug)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

########################################################################
# Find build dependencies
########################################################################
find_package(Gruel)
find_package(GnuradioCore)
find_package(GnuradioIQBalance)
find_package(UHD)
find_package(GnuradioUHD)
find_package(GnuradioFCD)
find_package(GnuradioFCDPP)
find_package(LibOsmoSDR)
find_package(LibRTLSDR)
find_package(LibMiriSDR)
find_package(LibHackRF)
find_package(LibbladeRF)
#find_package(LibDSD)

if(NOT GRUEL_FOUND)
    message(FATAL_ERROR "Gruel required to build " ${CMAKE_PROJECT_NAME})
endif()

if(NOT GNURADIO_CORE_FOUND)
    message(FATAL_ERROR "GnuRadio Core required to build " ${CMAKE_PROJECT_NAME})
endif()


########################################################################
# Setup boost
########################################################################
MESSAGE(STATUS "Configuring Boost C++ Libraries...")

# Although not required on my system, some users have linking issues without
SET(BOOST_REQUIRED_COMPONENTS
    thread
    system
    program_options
    filesystem
)

if(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")
    list(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
endif(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")

set(Boost_ADDITIONAL_VERSIONS
    "1.35.0" "1.35" "1.36.0" "1.36" "1.37.0" "1.37" "1.38.0" "1.38" "1.39.0" "1.39"
    "1.40.0" "1.40" "1.41.0" "1.41" "1.42.0" "1.42" "1.43.0" "1.43" "1.44.0" "1.44"
    "1.45.0" "1.45" "1.46.0" "1.46" "1.47.0" "1.47" "1.48.0" "1.48" "1.49.0" "1.49"
    "1.50.0" "1.50" "1.51.0" "1.51" "1.52.0" "1.52" "1.53.0" "1.53" "1.54.0" "1.54"
    "1.55.0" "1.55" "1.56.0" "1.56" "1.57.0" "1.57" "1.58.0" "1.58" "1.59.0" "1.59"
    "1.60.0" "1.60" "1.61.0" "1.61" "1.62.0" "1.62" "1.63.0" "1.63" "1.64.0" "1.64"
    "1.65.0" "1.65" "1.66.0" "1.66" "1.67.0" "1.67" "1.68.0" "1.68" "1.69.0" "1.69"
)

find_package(Boost COMPONENTS ${BOOST_REQUIRED_COMPONENTS})

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost required to build " ${CMAKE_PROJECT_NAME})
endif()

ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK)


########################################################################
# Register component
########################################################################
#include(GrComponent)
#GR_REGISTER_COMPONENT("gr-audio" ENABLE_GR_AUDIO
#    Boost_FOUND
#    ENABLE_GR_CORE
#)




SET(GR_PKG_AUDIO_EXAMPLES_DIR .)



########################################################################
# Setup the include and linker paths
########################################################################
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${GRUEL_INCLUDE_DIRS}
    ${GNURADIO_CORE_INCLUDE_DIRS}
    ${LIBOSMOSDR_INCLUDE_DIR}
    ${LIBDSD_INCLUDE_DIR}
    /usr/local/include/dsd/
	./lib/
)

link_directories(
    ${Boost_LIBRARY_DIRS}
    ${GRUEL_LIBRARY_DIRS}
    ${GNURADIO_CORE_LIBRARY_DIRS}
    ${LIBOSMOSDR_LIBRARIES}
    ${LIBDSD_LIBRARIES}
	/usr/local/lib/
	/usr/local/lib/pyton2.7/dist-packages/gnuradio/
)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

add_executable(smartnet smartnet.cc smartnet_crc.cc smartnet_deinterleave.cc logging_receiver_dsd.cc logging_receiver_pocsag.cc talkgroup.cc )
target_link_libraries(smartnet  gnuradio-core gnuradio-audio gnuradio-blocks gnuradio-osmosdr gnuradio-filter gnuradio-digital gnuradio-analog ${GRUEL_LIBRARIES} ${Boost_LIBRARIES} gr-dsd itpp ncurses menu)


INSTALL(TARGETS
    smartnet
    DESTINATION ${GR_PKG_AUDIO_EXAMPLES_DIR}
    COMPONENT "audio_examples"
)
