// default_cpp_flags.hpp
// XXX this file generate automaticly!

#pragma once

const char *defaultFlags[] = {
    "-isystem${LLVM_INSTALL_PREFIX}/include",
    "-isystem${LLVM_INSTALL_PREFIX}/include/c++/v1",
    "-isystem${LLVM_INSTALL_PREFIX}/lib/clang/${LLVM_PACKAGE_VERSION}/include",
    "-resource-dir=${LLVM_INSTALL_PREFIX}/lib/clang/${LLVM_PACKAGE_VERSION}"};
