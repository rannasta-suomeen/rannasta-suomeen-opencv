# opencv-ffi-rust

> **This project does NOT contain rust bindings to opencv-lib**
> You can find rust bindings from [here](https://github.com/twistedfall/opencv-rust)
> This project only contains the minimal working `build.rs` configuration for binding custom C++ functions that internally might use opencv

## Note before using
* This is **not** maintained by anyone
* It's a C++-ffi, and therefore it's [not even fully supported](https://rust-lang.github.io/rust-bindgen/cpp.html#unsupported-features) by `bindgen`. 
* Forced to work with `opencv-4.6.0`, will break by **any** version change
* Assumes the `opencv` was installed via apt by `libopencv-dev` *(by default looks for the libs from `usr/include/opencv4`)*
* Only tested and confirmed to work on `ubuntu`
* Requires (at least) `clang` and `pkg-config`
* Highly unstable, will most definetely break if you throw any modern C++ syntax at it
* Just be prepared for rust-linker to constantly break

## In case you are desparate for opencv to cross-compile
* `cargo test` will likely fail
* `cargo build` check if the libraries are not working *(`cargo test` doesn't know how to handle those errors)*
* `bash utils/compile.sh` Manually compile `lib.cpp`
* `bash utils/test.sh` Manually run the `test.cpp`

## Contributing

Please, just fork this thing and maybe turn it into something useful. No need to even open issues for whatever this is