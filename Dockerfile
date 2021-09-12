FROM debian:buster

ARG LLVM_VERSION=12

ADD ./ /hl-server

RUN apt-get update -y && \
    apt-get install -y --no-install-recommends software-properties-common wget gnupg2 g++ make cmake git && \
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    add-apt-repository "deb http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-$LLVM_VERSION main" && \
    apt-get update -y && \
    apt-get install -y --no-install-recommends libclang-$LLVM_VERSION-dev && \
    cd /hl-server && \
    mkdir -p build && \
    cd build && \
    rm * -rf && \
    cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_BASE_DIR=/usr/lib/llvm-$LLVM_VERSION .. && \
    cmake --build . && \
    cmake --build . --target install && \
    cd / && \
    rm /hl-server -r && \
    apt-get autoremove -y software-properties-common wget gnupg2 g++ make cmake git && \
    apt-get purge -y && \
    apt-get clean -y && \
    echo "cd /new-root/tmp" > /entrypoint.sh && \
    echo "llvm_resource_dir=\$(ls -d /usr/lib/llvm-$LLVM_VERSION/lib/clang/* | tail -n 1)" >> /entrypoint.sh && \
    echo "cp -rf \$llvm_resource_dir /new-root/tmp/llvm_resource_dir" >> /entrypoint.sh && \
    echo "hl-server --root=/new-root --flag=-resource-dir=/tmp/llvm_resource_dir \$@" >> /entrypoint.sh && \
    echo "rm -f /new-root/tmp/llvm_resource_dir" >> /entrypoint.sh

EXPOSE 53827
ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]
