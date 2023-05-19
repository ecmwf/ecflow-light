#
# (C) Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation nor
# does it submit to any jurisdiction.
#

# *** DOCUMENTATION *******************************************************************

include(Documentation)

ecbuild_add_option(FEATURE DOCUMENTATION
                   DEFAULT OFF
                   DESCRIPTION "Enable documentation generation")

if (HAVE_DOCUMENTATION)
  ecflow_light_documentation()
endif ()
