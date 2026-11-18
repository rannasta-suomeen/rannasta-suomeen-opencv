use std::env;
use std::path::PathBuf;

fn main() {
    const DIR: &str = "vendor/custom-opencv-lib";
    println!("cargo:rerun-if-changed=wrapper.h");
    println!("cargo:rerun-if-changed={DIR}");

    // Equivalent of `pkg-config --cflags --libs opencv4`. The pkg-config crate
    // prints the cargo:rustc-link-* lines itself, so no hard-coded library list.
    let opencv = pkg_config::Config::new()
        .atleast_version("4.2")
        .probe("opencv4")
        .expect("OpenCV 4 not found via pkg-config (apt install libopencv-dev)");

    // Compile src files
    cc::Build::new()
        .cpp(true)
        .files(["lib.cpp", "resolve_mask.cpp", "flood_fill.cpp"].map(|f| format!("{DIR}/{f}")))
        .includes(&opencv.include_paths)
        .compile("custom-lib");

    // Generate bindings
    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .clang_arg("-xc++")
        .clang_arg(format!("-I{DIR}"))
        .clang_args(
            opencv
                .include_paths
                .iter()
                .map(|p| format!("-I{}", p.display())),
        )
        // Allowlisting (we don't need the whole ass C++ std library)
        .allowlist_function("run_image_tool")
        .allowlist_type("ResolveOptions")
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
