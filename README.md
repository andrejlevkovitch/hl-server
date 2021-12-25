# hl-server

Fast asynchronous server for c/cpp and go code tokenization.
Based on `clang` and `go/ast`.

__supported version protocols__: v1.1

See [test hl client](example/simple_hl_client)

See [vim-hl-client](https://github.com/andrejlevkovitch/vim-hl-client)

## Requirements

- `libclang-dev` and `llvm-dev`

- `golang` min `1.13` (optional, see __Compilation__)

- `c++` compiler with support `11` standard

- [nlohmann-json](https://github.com/nlohmann/json) - upload automaticly

- [json-schema-validator](https://github.com/pboettch/json-schema-validator) - upload automaticly


## Compilation

```sh
mkdir build
cd build
cmake ..
cmake --build . -- -j4
```

__NOTE__ if you have several llvm packages you can set version that you want to
use by setting `USE_LLVM_VERSION` variable:

```sh
cmake -DUSE_LLVM_VERSION=11.1 ..
```

Also it can be required if you have several llvm packages with different versions,
but only one `libclang` package. So in this case you can get error about not
founded `Clang_LIBRARY`. You can fix that by setting version of llvm package
same as version of libclang package

__NOTE__ by default go completer is off, so for compile hl-server with go
highlight support you should reconfigure your cmake with:

```sh
cmake -DGO_TOKENIZER=ON ..
```

## Known issues

- Usage [libc++](https://libcxx.llvm.org/docs/UsingLibcxx.html)

For using libc++ you should add next lines to your .color_coded file:

```
-stdlib=libc++
-I${LLVM_INSTALL_PREFIX}/include/c++/v1
```

`LLVM_INSTALL_PREFIX` usually is `/usr/lib/llvm-${LLVM_VERSION}`

- I have error from hl-server like: "can't find stddef.h" or "can't find stdbool.h"

Libclang can't find llvm resource dir, so you should add it manually. You can
add it to .color_coded file as `-resource-dir=${LLVM_RESOURCE_DIR}` or as `--flag=-resource-dir=${LLVM_RESOURCE_DIR}`
argument to hl-server. For getting llvm resource dir you can call next command:

```
clang -print-resource-dir
```


## Bugs

For some reason you can get `ASTReadError` with using flags:

```
-isystem
/some/third/party/library
```

Just change it to:

```
-I/some/third/pary/library
```

## Other systems

The server should works with other *nix systems without problems, but it doesn't
tested. For non-unix systems you can try `boost` branch
