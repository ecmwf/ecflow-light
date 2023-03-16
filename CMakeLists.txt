#
# (C) Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation nor
# does it submit to any jurisdiction.
#

cmake_minimum_required( VERSION 3.20 FATAL_ERROR )

find_package( ecbuild 3.7 REQUIRED
  HINTS
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../ecbuild )

# ==============================================================================
# Project Definitions/Languages

project( ecflow_light
  VERSION 0.0.1
  LANGUAGES CXX )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# ==============================================================================
# Project Dependencies

include(cmake/Dependencies.cmake)


# ==============================================================================
# Project Options

include(cmake/CompilerOptions.cmake)


# ==============================================================================
# Project Build

add_subdirectory( src )


# ==============================================================================
# Project Package

ecbuild_pkgconfig(
  NAME ecflow_light
  DESCRIPTION "ECMWF Meteorological toolkit"
  LIBRARIES ecflow_light )

ecbuild_install_project( NAME ${PROJECT_NAME} )


# ==============================================================================
# Project Summary

ecbuild_print_summary()
