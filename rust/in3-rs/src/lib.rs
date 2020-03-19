//! Bindings to the [in3 library][upstream] 
//!
//! The [`In3`](struct.In3.html) struct is the main interface to the library.
//!
//! ```rust
//! extern crate in3;
//!
//!
//!

#![no_std]

#[macro_use]
extern crate alloc;

#[cfg(test)]
#[macro_use]
extern crate std;

#[cfg(test)]
#[global_allocator]
static ALLOCATOR: std::alloc::System = std::alloc::System;
mod in3;

#[cfg(test)]
mod test;

pub use crate::in3::*;

/// Contains items that you probably want to always import
///
/// For example:
///
/// ```
/// use in3::prelude::*;
/// ```
pub mod prelude {
    pub use crate::{
        In3,
    };
}
