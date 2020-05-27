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

pub mod error;
pub mod eth1;
pub mod in3;
pub mod signer;
pub mod traits;
pub mod types;

pub mod transport;

/// Contains items that you probably want to always import
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
    pub use crate::traits::Api as ApiTrait;
    pub use crate::traits::Client as ClientTrait;
    pub use crate::traits::{Signer, Storage, Transport};
    pub use crate::transport::{HttpTransport, MockTransport};
}
