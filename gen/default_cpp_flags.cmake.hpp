// default_cpp_flags.hpp
// XXX this file generate automaticly!

#pragma once

const char *defaultFlags[] = {
    // need for process every file separatly
    "-fPIC",

    // need for fix problem with finding stddef.h
    // see output of command `clang -print-resource-dir`
    "-resource-dir=${LLVM_LIBRARY_DIRS}/clang/${LLVM_PACKAGE_VERSION}",
};
