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
# ECFLOW Light library

set(TARGET ecflow_light)

set(${TARGET}_srcs
  # HEADERS
  ecflow/light/ClientAPI.h
  # SOURCES
  ecflow/light/ClientAPI.cc
)

ecbuild_add_library(
  TARGET ${TARGET}
  TYPE STATIC
  SOURCES ${${TARGET}_srcs}
  PUBLIC_INCLUDES
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PUBLIC_LIBS
    eckit
)
