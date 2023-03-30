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
# ECFLOW Light client (CLI) tool -- using C++ API, send single request

set(TARGET ecflow_light_client_cpp)

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
  CONDITION HAVE_BUILD_TOOLS
)

target_clangformat(${TARGET})

# ==============================================================================
# ECFLOW Light client (CLI) tool -- using C++ API, send multiple requests

set(TARGET ecflow_light_client_multiple_cpp)

set(${TARGET}_srcs
  # SOURCES
  ClientMainMultiple.cc
)

ecbuild_add_executable(
  TARGET ${TARGET}
  SOURCES
    ${${TARGET}_srcs}
  LIBS
    ecflow_light
  CONDITION HAVE_BUILD_TOOLS
)

target_clangformat(${TARGET})

# ==============================================================================
# ECFLOW Light client (CLI) tool -- using C API, send single request

set(TARGET ecflow_light_client_c)

set(${TARGET}_srcs
  # SOURCES
  ClientMain.c
)

ecbuild_add_executable(
  TARGET ${TARGET}
  SOURCES
    ${${TARGET}_srcs}
  LIBS
    ecflow_light
  CONDITION HAVE_BUILD_TOOLS
)

target_clangformat(${TARGET})

# ==============================================================================
# ECFLOW Light client (CLI) tool -- using Fortran 90 API, send single request

set(TARGET ecflow_light_client_f90)

set(${TARGET}_srcs
  # SOURCES
  ClientMain.F90
)

ecbuild_add_executable(
  TARGET ${TARGET}
  SOURCES
    ${${TARGET}_srcs}
  LIBS
    ecflow_lightf
    ecflow_light
  CONDITION HAVE_BUILD_TOOLS
)

# ==============================================================================
# ECFLOW Light client (CLI) tool -- using Fortran 90 API, send multiple requests

set(TARGET ecflow_light_client_multiple_f90)

set(${TARGET}_srcs
  # SOURCES
  ClientMainMultiple.F90
)

ecbuild_add_executable(
  TARGET ${TARGET}
  SOURCES
    ${${TARGET}_srcs}
  LIBS
    ecflow_lightf
    ecflow_light
  CONDITION HAVE_BUILD_TOOLS
)
