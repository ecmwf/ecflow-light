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
# ECKIT

ecbuild_find_package(
  NAME eckit
  VERSION  1.18
  REQUIRED )


# ==============================================================================
# cURL

find_package(CURL REQUIRED)


# ==============================================================================
# std::fs

set(STDFSLIB "")
if( (NOT APPLE) AND 
    (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "Intel"))
  set(STDFSLIB "stdc++fs")
endif()
