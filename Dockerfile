FROM debian:buster

ARG LLVM_VERSION=12

RUN apt-get update -y && \
    apt-get install -y software-properties-common wget gnupg2 && \
    apt-get install -y g++ cmake git

RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    add-apt-repository "deb http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-$LLVM_VERSION main" && \
    apt-get update -y && \
    apt-get install -y libclang-$LLVM_VERSION-dev

ADD ./ /hl-server

RUN cd /hl-server && \
    mkdir -p build && \
    cd build && \
    rm * -rf && \
    cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_BASE_DIR=/usr/lib/llvm-$LLVM_VERSION .. && \
    cmake --build . && \
    cmake --build . --target install && \
    cd / && \
    rm /hl-server -r

RUN apt-get autoremove -y cmake git g++ software-properties-common wget gnupg2 && \
    apt-get purge -y

# see `clang -print-resource-dir` for get llvm resource dir
RUN echo "cd /new-root/tmp" > /entrypoint.sh && \
    echo "llvm_resource_dir=\$(ls -d /usr/lib/llvm-$LLVM_VERSION/lib/clang/* | tail -n 1)" >> /entrypoint.sh && \
    echo "cp -rf \$llvm_resource_dir /new-root/tmp/llvm_resource_dir" >> /entrypoint.sh && \
    echo "hl-server --root=/new-root --flag=-resource-dir=/tmp/llvm_resource_dir \$@" >> /entrypoint.sh && \
    echo "rm -f /new-root/tmp/llvm_resource_dir" >> /entrypoint.sh

EXPOSE 53827
ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]
