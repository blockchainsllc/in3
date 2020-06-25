//! Errors used throughout the library.
use core::result;
use std::convert;

use in3_sys::in3_ret_t::*;

macro_rules! in3_error_def {
    ( $( $( #[$attr:meta] )* => $rust_variant:ident = $cs_variant:ident; )* ) => {
        /// Errors either originating in the C code (that map to `in3_ret_t`) or low-level errors
        /// in unsafe code.
        /// Enabling logging should help with debugging such errors.
        #[derive(Debug, PartialEq, Eq)]
        pub enum SysError {
            $(
                $(
                    #[$attr]
                )*
                $rust_variant,
            )*
            /// Error that cannot be mapped to `in3_ret_t` variants
            UnknownIn3Error,
            /// Resource temporarily unavailable
            TryAgain,
            /// Error response
            ResponseError(String),
            /// Could not find last waiting context in execute loop
            ContextError,
        }

        impl From<in3_sys::in3_ret_t::Type> for SysError {
            fn from(err: in3_sys::in3_ret_t::Type) -> Self {
                match err {
                    $(
                        $cs_variant => SysError::$rust_variant,
                    )*
                    _ => SysError::UnknownIn3Error,
                }
            }
        }

        impl From<&SysError> for i32 {
            fn from(err: &SysError) -> i32 {
                match err {
                    $(
                         SysError::$rust_variant => $cs_variant,
                    )*
                    _ => std::i32::MIN,
                }
            }
        }
    }
}

in3_error_def!(
    => NoMemory = IN3_ENOMEM;
    => UnknownError = IN3_EUNKNOWN;
    => NotSupported = IN3_ENOTSUP;
    => InvalidValue = IN3_EINVAL;
    => NotFound = IN3_EFIND;
    => InvalidConfig = IN3_ECONFIG;
    => LimitReached = IN3_ELIMIT;
    => VersionMismatch = IN3_EVERS;
    => DataInvalid = IN3_EINVALDT;
    => WrongPassword = IN3_EPASS;
    => RpcError = IN3_ERPC;
    => RpcNoResponse = IN3_ERPCNRES;
    => UsnUrlParseError = IN3_EUSNURL;
    => TransportError = IN3_ETRANS;
    => OutOfRange = IN3_ERANGE;
    => Waiting = IN3_WAITING;
    => IgnorableError = IN3_EIGNORE;
);

/// Generic Result type for IN3 errors.
#[must_use]
pub type In3Result<T> = result::Result<T, Error>;

/// Error type that represents all possible errors in the lib
#[derive(Debug)]
pub enum Error {
    /// Errors originating in the C code
    InternalError(SysError),
    /// JSON parser errors
    JsonError(serde_json::error::Error),
    /// Base64 decoding errors
    Base64Error(base64::DecodeError),
    /// JSON RPC errors
    JsonRpcError(crate::json_rpc::Error),
    /// Custom error type for ease of use
    CustomError(String),
}

impl convert::From<SysError> for Error {
    fn from(err: SysError) -> Self {
        Self::InternalError(err)
    }
}

impl convert::From<serde_json::error::Error> for Error {
    fn from(err: serde_json::error::Error) -> Self {
        Self::JsonError(err)
    }
}

impl convert::From<&str> for Error {
    fn from(err: &str) -> Self {
        err.to_string().into()
    }
}

impl convert::From<String> for Error {
    fn from(err: String) -> Self {
        Self::CustomError(err)
    }
}

#[cfg(test)]
mod tests {
    use in3_sys::in3_ret_t;

    use super::SysError;

    #[test]
    fn test_error() {
        let errors = [
            SysError::NoMemory,
            SysError::UnknownIn3Error,
            SysError::from(in3_ret_t::IN3_ECONFIG),
            SysError::from(500 as in3_ret_t::Type),
        ];

        for error in errors.iter() {
            println!("{:?}", error);
        }
    }
}
