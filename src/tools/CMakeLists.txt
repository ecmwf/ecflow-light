#
# (C) Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation nor
# does it submit to any jurisdiction.
#

# ==============================================================================
# ECFLOW Light client (CLI) tool -- using C++ API

set(TARGET ecflow_light_client)

set(${TARGET}_srcs
  # SOURCES
  ClientMain.cc
)

ecbuild_add_executable(
  TARGET ${TARGET}
  SOURCES
    ${${TARGET}_srcs}
  LIBS
    ecflow_light
    eckit
    eckit_option
  CONDITION HAVE_BUILD_TOOLS
)

target_clangformat(TARGET ${TARGET} CONDITION HAVE_BUILD_TOOLS)


# ==============================================================================
# Tiny client (CLI) tool -- using C++ API

set(TARGET tiny_client)

set(${TARGET}_srcs
  # SOURCES
  TinyMain.cc
)

ecbuild_add_executable(
  TARGET ${TARGET}
  SOURCES
    ${${TARGET}_srcs}
  LIBS
    ecflow_light
  CONDITION HAVE_BUILD_TOOLS
)

target_clangformat(TARGET ${TARGET} CONDITION HAVE_BUILD_TOOLS)
