# hl-server

Fast asynchronous server for c/cpp code tokenization, bases on `clang`.

__supported version protocols__: v1.1

See [test hl client](example/simple_hl_client)

See [vim-hl-client](https://github.com/andrejlevkovitch/vim-hl-client)

## Requirements

- `libclang-dev` and `llvm-dev`

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


__NOTE__ if you want to use [libc++](https://libcxx.llvm.org/docs/UsingLibcxx.html)
then you need also set incude directory:

```
-stdlib=libc++
-I${LLVM_INSTALL_PREFIX}/include/c++/v1
```

## Other systems

The server should works with other *nix systems without problems, but it doesn't
tested. For non-unix systems you can try `boost` branch
