# ecflow-light

> [!IMPORTANT]
> This software is **Graduated** and subject to ECMWF's guidelines on [Software Maturity](https://github.com/ecmwf/codex/raw/refs/heads/main/Project%20Maturity).

*ecFlow-light* library enables a workflow task with a lightweight mechanism to send telemetry over UDP to the ecFlow
server. The currently supported requests are:

 - update task meter
 - update task label
 - update task event

## Requirements

Tested compilers include:

- GCC 13.1.0
- Clang 16.0.3
- Intel 2021.4.0
- Apple LLVM 14.0.0 (clang-1400.0.29.202)

Required dependencies:

- CMake --- For use and installation see http://www.cmake.org/
- ecbuild --- Library of CMake macros at ECMWF
- eckit --- Library to support development of tools and applications at ECMWF

## COPYRIGHT AND LICENCE

Copyright 2023- European Centre for Medium-Range Weather Forecasts (ECMWF).

This software is licensed under the terms of the Apache Licence Version 2.0
which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
