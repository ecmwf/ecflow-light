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
# Documentation

macro(ecflow_light_documentation)

  # ============================================================================
  # Doxygen

  find_package(Doxygen REQUIRED)

  set(DOXYGEN_INPUT_DIR ${CMAKE_SOURCE_DIR}/src)
  set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
  set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/xml/index.xml)
  set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in)
  set(DOXYFILE_OUT ${DOXYGEN_OUTPUT_DIR}/Doxyfile)

  # Replace variables inside @@ with the current values
  configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

  file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}) # Just in case Doxygen doesn't create this for us

  # Only regenerate Doxygen when the Doxyfile or public headers change
  add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
    DEPENDS ${ECFLOW_LIGHT_DEV_PUBLIC_HEADERS}
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
    WORKING_DIRECTORY ${DOXYGEN_OUTPUT_DIR}
    MAIN_DEPENDENCY ${DOXYFILE_OUT}
    COMMENT "Generating ecFlow Light documentation with Doxygen")

  # Nice named target so we can run the job easily
  add_custom_target(ecflow_light_doxygen DEPENDS ${DOXYGEN_INDEX_FILE})

  # ============================================================================
  # Sphinx

  find_package(Sphinx REQUIRED)

  set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/sphinx)
  set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx/site)
  set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

  set(SPHINX_SOURCES
    ${SPHINX_SOURCE}/index.rst
    ${SPHINX_SOURCE}/reference.rst)

  # Only regenerate Sphinx when:
  # - Doxygen has rerun
  # - Our doc files have been updated
  # - The Sphinx config has been updated
  add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
    COMMAND
    ${SPHINX_EXECUTABLE}
    -b html
    -Dbreathe_projects.ecflowlight=${DOXYGEN_OUTPUT_DIR}/xml
    ${SPHINX_SOURCE}
    ${SPHINX_BUILD}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS
    ${SPHINX_SOURCES}
    ${DOXYGEN_INDEX_FILE}
    MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
    COMMENT "Generating ecFlow Light documentation with Sphinx")

  # Nice named target so we can run the job easily
  add_custom_target(ecflow_light_sphinx DEPENDS ${SPHINX_INDEX_FILE} ecflow_light_doxygen)

  add_custom_target(docs DEPENDS ecflow_light_sphinx ecflow_light_doxygen)

  # Add an install target to install the docs
  include(GNUInstallDirs)
  install(DIRECTORY ${SPHINX_BUILD}
    DESTINATION "${CMAKE_INSTALL_PREFIX}/docs")

endmacro()
