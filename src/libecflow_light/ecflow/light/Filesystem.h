/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_FILESYSTEM_H
#define ECFLOW_LIGHT_FILESYSTEM_H

#if __GNUC__ >= 8 or __clang_major__ >= 7

#include <filesystem>
namespace fs = std::filesystem;

#elif __GNUC__ >= 7

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#else

#error Filesystem support not found -- please use compiler that allows #include <filesystem>

#endif

#endif
