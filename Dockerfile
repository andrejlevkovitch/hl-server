FROM alpine:3.14.2

# install libclang
RUN apk add --no-cache clang-dev

# compile and install hl-server
RUN apk add --no-cache build-base cmake git && \
    cd / && \
    git clone --recurse-submodules https://github.com/andrejlevkovitch/hl-server.git && \
    cd /hl-server && \
    mkdir -p build && \
    cd build && \
    rm * -rf && \
    cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_BASE_DIR=/usr/lib .. && \
    cmake --build . && \
    cmake --build . --target install && \
    cd / && \
    rm -rf /hl-server && \
    apk del build-base cmake git

# add entrypoint
RUN echo "cd /new-root/tmp" > /entrypoint.sh && \
    echo "llvm_resource_dir=\$(clang --print-resource-dir)" >> /entrypoint.sh && \
    echo "cp -rf \$llvm_resource_dir /new-root/tmp/llvm_resource_dir" >> /entrypoint.sh && \
    echo "hl-server --root=/new-root --flag=-resource-dir=/tmp/llvm_resource_dir \$@" >> /entrypoint.sh && \
    echo "rm -rf /new-root/tmp/llvm_resource_dir" >> /entrypoint.sh

EXPOSE 53827
ENTRYPOINT ["/bin/sh", "/entrypoint.sh"]
