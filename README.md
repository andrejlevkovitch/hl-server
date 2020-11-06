# hl-server

Fast asynchronous server for buffers tokenizing.
Nowadays it support tokenizing `c` and `cpp` buffers (based on `clang`).
As example here realized `polyndrom` tokenizing for buffers

__supported version protocols__: v1, v1.1

See [test hl client](test/simple_hl_client)

See [vim-hl-client](https://github.com/andrejlevkovitch/vim-hl-client)

## Requirements

- `c++` compiler with support `17` standard

- `boost`

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
-isystem${LLVM_INSTALL_PREFIX}/include/c++/v1
```
