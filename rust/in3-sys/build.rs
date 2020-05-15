extern crate bindgen;
extern crate core;

use std::{env, fs};
use std::fs::copy;
use std::path::PathBuf;
use std::vec::Vec;

const IN3_DIR: &'static str = "in3";
const BINDINGS_FILE: &'static str = "in3.rs";

/// Build in3 using the cc crate
fn build_in3_cc() {
    fn append_c_files_from_dir(files: &mut Vec<String>, path: String) {
        let paths = fs::read_dir(path).unwrap();
        for p in paths {
            let p = p.unwrap();
            let pathstr = p.path().display().to_string();
            if p.file_type().unwrap().is_dir() {
                append_c_files_from_dir(files, pathstr);
            } else if p.file_name().into_string().unwrap().ends_with(".c") {
                files.push(pathstr);
            }
        }
    }
    let mut files = vec![];
    append_c_files_from_dir(&mut files, "in3/src/api".into());
    append_c_files_from_dir(&mut files, "in3/src/core".into());
    append_c_files_from_dir(&mut files, "in3/src/verifier".into());
    append_c_files_from_dir(&mut files, "in3/src/third-party/crypto".into());
    append_c_files_from_dir(&mut files, "in3/src/third-party/libb64".into());
    // append_c_files_from_dir(&mut files, "in3/src/third-party/libscrypt".into());
    append_c_files_from_dir(&mut files, "in3/src/third-party/multihash".into());
    append_c_files_from_dir(&mut files, "in3/src/third-party/nanopb".into());
    append_c_files_from_dir(&mut files, "in3/src/third-party/tommath".into());

    cc::Build::new()
        .files(files)
        .include(format!("{}/{}", IN3_DIR, "include"))
        .define("ETH_FULL", None)
        .define("IPFS", None)
        .define("ETH_API", None)
        .define("USE_PRECOMPUTED_EC", None)
        .define("ERR_MSG", None)
        .define("EVM_GAS", None)
        // .define("USE_SCRYPT", None)
        .define("IN3_MATH_FAST", None)
        .flag_if_supported("-Wno-unused-function")
        .flag_if_supported("-Wno-unused-parameter")
        .flag_if_supported("-Wno-unknown-pragmas")
        .flag_if_supported("-Wno-sign-compare")
        .flag_if_supported("-Wno-return-type")
        .flag_if_supported("-Wno-implicit-fallthrough")
        .flag_if_supported("-Wno-missing-field-initializers")
        .compile(format!("in3_{}", env_var("TARGET")).as_str());
}

/// Search for header in search paths
fn find_in3_header(header_search_paths: &Vec<PathBuf>, name: &str) -> Option<PathBuf> {
    for search_path in header_search_paths.iter() {
        let potential_file = search_path.join(name);
        if potential_file.is_file() {
            return Some(potential_file);
        }
    }
    None
}

/// Gets environment variable value. Panics if variable is not set.
fn env_var(var: &str) -> String {
    env::var(var).expect(&format!("Environment variable {} is not set", var))
}

fn write_bindgen_bindings(
    header_search_paths: &Vec<PathBuf>,
    update_pregenerated_bindings: bool,
    pregenerated_bindgen_header: PathBuf,
    out_bindings_path: PathBuf,
) {
    let builder = bindgen::Builder::default()
        .rust_target(bindgen::RustTarget::Stable_1_28)
        .size_t_is_usize(true)
        .use_core()
        .ctypes_prefix("libc")
        .header(
            find_in3_header(header_search_paths, "in3.rs.h")
                .expect("Could not find header")
                .to_str()
                .unwrap(),
        )
        .disable_name_namespacing()
        .prepend_enum_name(false)
        .generate_comments(true)
        .impl_debug(true)
        .constified_enum_module("in3_ret_t")
        .rustified_enum(".*");

    let bindings = builder.generate().expect("Unable to generate bindings");
    bindings
        .write_to_file(&out_bindings_path)
        .expect("Unable to write bindings");

    if update_pregenerated_bindings {
        copy(out_bindings_path, pregenerated_bindgen_header)
            .expect("Unable to update in3 bindings");
    }
}

fn main() {
    let mut header_search_paths: Vec<PathBuf> = Vec::new();
    header_search_paths.push([IN3_DIR, "include"].iter().collect());

    build_in3_cc();

    println!("cargo:rustc-link-lib=static=in3_{}", env_var("TARGET"));

    // fixme: find a way to get workspace dir through an env var so we can point to native libs
    // until then workspace cargo commands (cargo build/run/etc.) will find libin3 in in3-sys/native
    // and others (cargo publish) that run from inside rust/in3-sys will find libin3 in native/
    println!("cargo:rustc-link-search=native=in3-sys/native");
    println!("cargo:rustc-link-search=native=native");

    let pregenerated_bindgen_header: PathBuf = [
        env_var("CARGO_MANIFEST_DIR"),
        "pre_generated".into(),
        BINDINGS_FILE.into(),
    ]
        .iter()
        .collect();

    let out_bindings_path = PathBuf::from(env_var("OUT_DIR")).join(BINDINGS_FILE);

    write_bindgen_bindings(
        &header_search_paths,
        env::var("UPDATE_IN3_BINDINGS").is_ok(),
        pregenerated_bindgen_header,
        out_bindings_path,
    );
}
