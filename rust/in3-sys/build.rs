//! The following environment variables affect the build:
//!
//! * `UPDATE_IN3_BINDINGS`: setting indicates that the pre-generated `in3.rs` should be
//!   updated with the output bindgen
//!
//! # Bindgen enum mapping
//!
//! Bindgen can convert C enums in several ways:
//!
//! 1. **"Rustified" enum**: Bindgen creates a Rust enum, which provides the most "type safety" and
//!    reduces the chance of confusing variants for a different type. For variants whose
//!    discriminant values are not distinct, bindgen defines constants.
//! 2. **"Constified" enum**: Bindgen defines constants for each enum variant.
//! 3. **"Constified" enum module**: Bindgen defines constants for each enum variant in a separate
//!    module.
//!
//! # Rationale for enum types
//!
//! Rustified enum: these have distinct variant discriminants
//!
//! * `cs_arch`
//! * `cs_op_type`
//! * `cs_opt_type`
//!
//! Constified enum module:
//!
//! * `cs_err`: avoid undefined behavior in case an error is instantiated with an invalid value; the
//!   compiler could make false assumptions that the value is only within a certain range.
//! * `cs_group_type`/`ARCH_insn_group`: each architecture adds group types to the `cs_group_type`,
//!   so we constify to avoid needing to transmute.
//! * `cs_mode`: used as a bitmask; when values are OR'd together, they are not a valid discriminant
//!   value
//! * `cs_opt_value`/`ARCH_reg`: variant discriminants are not unique
//!
//! Bitfield enum: fields are OR'd together to form new values
//! * `cs_mode`

extern crate bindgen;

use std::env;
use std::fs::copy;
use std::path::PathBuf;

include!("common.rs");

const IN3_DIR: &'static str = "../../c";

/// Indicates how in3 library should be linked
#[allow(dead_code)]
enum LinkType {
    Dynamic,
    Static,
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

/// Create bindings using bindgen
fn write_bindgen_bindings(
    header_search_paths: &Vec<PathBuf>,
    out_bindings_path: PathBuf,
) {
    let pregenerated_bindgen_header: PathBuf = [
        env_var("CARGO_MANIFEST_DIR"),
        "pre_generated".into(),
        BINDINGS_FILE.into(),
    ]
        .iter()
        .collect();
    let mut builder = bindgen::Builder::default()
        .rust_target(bindgen::RustTarget::Stable_1_19)
        .size_t_is_usize(true)
        .use_core()
        .ctypes_prefix("libc")
        .header(
            find_in3_header(header_search_paths, "in3.h")
                .expect("Could not find header")
                .to_str()
                .unwrap(),
        )
        .disable_name_namespacing()
        .prepend_enum_name(false)
        .generate_comments(true)
        .impl_debug(true)
        .rustified_enum(".*");

    // Whitelist cs_.* functions and types
    let pattern = String::from(".*");
    builder = builder
        .whitelist_function(&pattern)
        .whitelist_type(&pattern);

    let bindings = builder.generate().expect("Unable to generate bindings");

    // Write bindings to $OUT_DIR/bindings.rs
    bindings
        .write_to_file(&out_bindings_path)
        .expect("Unable to write bindings");
    //Copy binding to other path 
    copy(out_bindings_path, pregenerated_bindgen_header)
        .expect("Unable to update in3 bindings");
}

fn main() {

    // C header search paths
    let mut header_search_paths: Vec<PathBuf> = Vec::new();

    header_search_paths.push([IN3_DIR, "include"].iter().collect());
    println!("cargo:rustc-link-lib=static=in3");
    println!("cargo:rustc-link-lib=static=transport_curl");
    println!("cargo:rustc-link-lib=curl");
    println!("cargo:rustc-link-search={}/../../build/lib", env_var("CARGO_MANIFEST_DIR"));

    let out_bindings_path = PathBuf::from(env_var("OUT_DIR")).join(BINDINGS_FILE);

    // Only run bindgen if we are *not* using the bundled in3 bindings
    write_bindgen_bindings(
        &header_search_paths,
        out_bindings_path,
    );
}
