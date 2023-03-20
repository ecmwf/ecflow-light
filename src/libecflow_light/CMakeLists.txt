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

configure_file(ecflow/light/Version.h.in generated/ecflow/light/Version.h @ONLY)

set(TARGET ecflow_light)

set(${TARGET}_public_headers
  # PUBLIC HEADERS
  ecflow/light/LightAPI.h
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ecflow/light/Version.h
)

set(${TARGET}_sources
  # PRIVATE HEADERS
  ecflow/light/ClientAPI.h
  ecflow/light/Conversion.h
  ecflow/light/Exception.h
  # SOURCES
  ecflow/light/ClientAPI.cc
  ecflow/light/LightAPI.cc
  ecflow/light/Version.cc
)

ecbuild_add_library(
  TARGET ${TARGET}
  TYPE STATIC
  SOURCES
    ${${TARGET}_public_headers}
    ${${TARGET}_sources}
  PUBLIC_INCLUDES
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
  PUBLIC_LIBS
    eckit
)

target_clangformat(${TARGET})

set_property(
  TARGET ${TARGET}
  PROPERTY PUBLIC_HEADER ${${TARGET}_public_headers})

install(
  TARGETS ${TARGET}
  PUBLIC_HEADER
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ecflow/light")

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/shared
  DESTINATION "${CMAKE_INSTALL_PREFIX}")
