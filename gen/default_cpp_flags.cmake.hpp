// default_cpp_flags.hpp
// XXX this file generated automaticly. Do not edit!

#pragma once

const char *c_defaultFlags[] = {
    // need for process every file separately
    "-fPIC",

    // need for fix problem with finding stddef.h
    // see output of command `clang -print-resource-dir`
    "-resource-dir=${LLVM_LIBRARY_DIRS}/clang/${LLVM_PACKAGE_VERSION}",
};
