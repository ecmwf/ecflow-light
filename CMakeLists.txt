#
# (C) Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation nor
# does it submit to any jurisdiction.
#

cmake_minimum_required( VERSION 3.17.5 FATAL_ERROR )

find_package( ecbuild 3.7 REQUIRED
  HINTS
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../ecbuild )

# ==============================================================================
# Project Definitions/Languages

project( ecflow_light
  LANGUAGES C CXX Fortran )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

ecbuild_declare_project()

ecbuild_add_option( FEATURE BUILD_TOOLS
                    DEFAULT ON
                    DESCRIPTION "Build the command line tools" )

# ==============================================================================
# Project Dependencies

include(cmake/Dependencies.cmake)


# ==============================================================================
# Project Options

include(cmake/CompilerOptions.cmake)

find_package(ClangFormat)


# ==============================================================================
# Project Build

add_subdirectory( src )
add_subdirectory( tests )
add_subdirectory( docs )

# ==============================================================================
# Project Package

ecbuild_pkgconfig(
  NAME ecflow_light
  DESCRIPTION "ECMWF Workflow Management (light)"
  LIBRARIES ecflow_light )

ecbuild_install_project( NAME ${PROJECT_NAME} )


# ==============================================================================
# Project Summary

ecbuild_print_summary()
