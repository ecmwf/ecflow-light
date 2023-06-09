#
# (C) Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation nor
# does it submit to any jurisdiction.
#

set(TEST_CXXFLAGS
  -Wno-unused-parameter # silences eckit warnings on test framework
)

# ==============================================================================
# API Test

set(TARGET ecflow_light_api_client_test)

set(${TARGET}_srcs
  # SOURCES
  TestAPI.cc
)

ecbuild_add_test(
  TARGET ${TARGET}
  SOURCES
    ${${TARGET}_srcs}
  LIBS
    ecflow_light
    eckit
  CXXFLAGS
    ${TEST_CXXFLAGS}
)

target_clangformat(TARGET ${TARGET} CONDITION HAVE_TESTS)

# ==============================================================================
# UDP Client Test

set(TARGET ecflow_light_udp_client_test)

set(${TARGET}_srcs
  # SOURCES
  TestUDPClient.cc
)

ecbuild_add_test(
  TARGET ${TARGET}
  SOURCES
    ${${TARGET}_srcs}
  LIBS
    ecflow_light
    eckit
  CXXFLAGS
    ${TEST_CXXFLAGS}
)

target_clangformat(TARGET ${TARGET} CONDITION HAVE_TESTS)
