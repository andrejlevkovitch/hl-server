#!/bin/bash

clang_recource_dir=$(clang -print-resource-dir)

docker run --rm -it -p53827:53827 -v/:/new-root hl-server:latest $@
