// default_cpp_flags.hpp
// XXX this file generated automaticly. Do not edit!

#pragma once

// first flag needed for process every file separately
//
// second flag needed for fix problem with finding stddef.h
// see output of command `clang -print-resource-dir`
const char *c_default_flags = R"(
-fPIC
-resource-dir=${LLVM_LIBRARY_DIRS}/clang/${LLVM_PACKAGE_VERSION}
)";
