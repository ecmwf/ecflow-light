# ecflow-light

ecFlow-light library enables a workflow task with a lightweight mechanism to send telemetry over UDP to the ecFlow
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
