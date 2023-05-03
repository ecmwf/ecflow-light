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
configure_file(ecflow/light/Version.cc.in generated/ecflow/light/Version.cc @ONLY)

set(TARGET ecflow_light)

set(${TARGET}_public_headers
  # PUBLIC HEADERS
  ecflow/light/API.h
  ecflow/light/API.hpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ecflow/light/Version.h
)

set(${TARGET}_sources
  # PRIVATE HEADERS
  ecflow/light/ClientAPI.hpp
  ecflow/light/Conversion.hpp
  ecflow/light/Exception.hpp
  ecflow/light/Trace.hpp
  # SOURCES
  ecflow/light/API.cc
  ecflow/light/ClientAPI.cc
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ecflow/light/Version.cc
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
  PRIVATE_LIBS
    eckit
    ${STDFSLIB}
)

target_clangformat(TARGET ${TARGET})

set_property(
  TARGET ${TARGET}
  PROPERTY PUBLIC_HEADER ${${TARGET}_public_headers})

install(
  TARGETS ${TARGET}
  PUBLIC_HEADER
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ecflow/light")
