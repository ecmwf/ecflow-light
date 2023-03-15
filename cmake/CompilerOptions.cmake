#
# (C) Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation nor
# does it submit to any jurisdiction.
#

ecbuild_add_option( FEATURE WARNINGS
                    DEFAULT ON
                    DESCRIPTION "Add warnings to compiler" )

ecbuild_add_option( FEATURE COLOURED_OUTPUT
                    DEFAULT ON
                    DESCRIPTION "Make sure that GCC/Clang produce coloured output" )

ecbuild_add_option( FEATURE EXPORT_COMPILE_COMMANDS
                    DEFAULT ON
                    DESCRIPTION "Activate generation of compilation commands DB" )

# ==============================================================================
# Compiler Warnings

if(HAVE_WARNINGS)

  ecbuild_add_cxx_flags( "-Wall" NO_FAIL )
  ecbuild_add_cxx_flags( "-Wextra" NO_FAIL )
  ecbuild_add_cxx_flags( "-Wpedantic" NO_FAIL )

endif()

# ==============================================================================
# Compiler Commands DB

if(HAVE_EXPORT_COMPILE_COMMANDS)

  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

endif()

# ==============================================================================
# Compiler Coloured Output

if(HAVE_COLOURED_OUTPUT AND "${CMAKE_GENERATOR}" STREQUAL "Ninja")

  message(STATUS "Ninja generator detected! Ensuring GNU/Clang produce coloured output...")
  ecbuild_add_cxx_flags(
    "$<$<CXX_COMPILER_ID:GNU>:-fdiagnostics-color=always>"
    "$<$<CXX_COMPILER_ID:Clang>:-fdiagnostics-color>"
    NO_FAIL)

endif()
