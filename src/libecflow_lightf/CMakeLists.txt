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
# ECFLOW Light library (Fortran Module)

set(TARGET ecflow_lightf)

set(${TARGET}_sources
  # SOURCES
  ecflow_light.F90
)

ecbuild_add_library(
  TARGET ${TARGET}
  SOURCES
    ${${TARGET}_sources}
  PUBLIC_INCLUDES
    $<BUILD_INTERFACE:${CMAKE_Fortran_MODULE_DIRECTORY}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE_LIBS
    ecflow_light
)

install( DIRECTORY ${CMAKE_Fortran_MODULE_DIRECTORY}/${CMAKE_CFG_INTDIR}
  DESTINATION module/ecflow_light
  COMPONENT modules )
