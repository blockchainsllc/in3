//! Bindings to the [Incubed](https://github.com/slockit/in3-c/) C library.
//!
//! This crate is a wrapper around the
//! [Incubed](https://github.com/slockit/in3-c/) C library."
//!
//! The IN3 client is a
//! * Crypto-Economic
//! * Non-syncronizing and stateless, but fully verifying
//! * Minimal resource consuming
//!
//! blockchain client (Crypto-Economic Client, Minimal Verification Client, Ultra Light Client).
//!
//! The [`Client`](in3/struct.Client.html) struct is the main interface to the library.
//!

pub mod btc;
pub mod error;
pub mod eth1;
pub mod in3;
pub mod ipfs;
pub mod json_rpc;
pub mod logging;
pub mod signer;
pub mod traits;
pub mod transport;
pub mod types;

/// Contains items that you probably want to always import.
///
/// # Example
///
/// ```
/// use in3::prelude::*;
/// ```
pub mod prelude {
    pub use crate::error::*;
    pub use crate::in3::*;
    pub use crate::signer::*;
    pub use crate::traits::{Api as ApiTrait, Client as ClientTrait, Signer, Storage, Transport};
    pub use crate::transport::{HttpTransport, MockJsonTransport, MockTransport};
    pub use crate::types::*;
}


#[inline]
pub fn init() {
    use std::sync::Once;

    /// Used to prevent concurrent or duplicate initialization.
    static INIT: Once = Once::new();

    /// An exported constructor function. On supported platforms, this will be
    /// invoked automatically before the program's `main` is called.
    #[cfg_attr(
    any(target_os = "linux", target_os = "freebsd", target_os = "android"),
    link_section = ".init_array"
    )]
    #[cfg_attr(target_os = "macos", link_section = "__DATA,__mod_init_func")]
    #[cfg_attr(target_os = "windows", link_section = ".CRT$XCU")]
    static INIT_CTOR: extern "C" fn() = init_inner;

    /// This is the body of our constructor function.
    #[cfg_attr(
    any(target_os = "linux", target_os = "android"),
    link_section = ".text.startup"
    )]
    extern "C" fn init_inner() {
        INIT.call_once(|| {
            unsafe {
                in3_sys::in3_init();
            }
        });
    }

    // We invoke our init function through our static to ensure the symbol isn't
    // optimized away by a bug: https://github.com/rust-lang/rust/issues/47384
    INIT_CTOR();
}
