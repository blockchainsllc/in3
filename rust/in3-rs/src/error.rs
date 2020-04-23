use core::fmt;
use core::result;
use std::{convert, ffi};

use in3_sys::in3_ret_t::*;

macro_rules! in3_error_def {
    ( $( $( #[$attr:meta] )* => $rust_variant:ident = $cs_variant:ident; )* ) => {
        #[derive(Clone, Debug, Eq, Hash, PartialEq)]

        pub enum Error {
            $(
                $(
                    #[$attr]
                )*
                $rust_variant,
            )*

            UnknownIn3Error,
            TryAgain,
            CustomError(String),
        }

        impl From<in3_sys::in3_ret_t::Type> for Error {
            fn from(err: in3_sys::in3_ret_t::Type) -> Self {
                match err {
                    $(
                        $cs_variant => Error::$rust_variant,
                    )*
                    _ => Error::UnknownIn3Error,
                }
            }
        }

        impl From<&Error> for i32 {
            fn from(err: &Error) -> i32 {
                match err {
                    $(
                         Error::$rust_variant => $cs_variant,
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

#[must_use]
pub type In3Result<T> = result::Result<T, Error>;

impl fmt::Display for Error {
    fn fmt(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
        write!(fmt, "{}", self.description())
    }
}

impl Error {
    fn description(&self) -> &str {
        use self::Error::*;
        match self {
            CustomError(ref msg) => msg,
            UnknownIn3Error => "Unknown error",
            _ => unsafe {
                ffi::CStr::from_ptr(in3_sys::in3_errmsg(self.into()))
                    .to_str()
                    .unwrap()
            },
        }
    }
}

impl convert::From<serde_json::error::Error> for Error {
    fn from(_: serde_json::error::Error) -> Self {
        Self::DataInvalid
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

    use super::Error;

    #[test]
    fn test_error() {
        let errors = [
            Error::NoMemory,
            Error::UnknownIn3Error,
            Error::CustomError("Custom error".to_string()),
            Error::from(in3_ret_t::IN3_ECONFIG),
            Error::from(500 as in3_ret_t::Type),
        ];

        for error in errors.iter() {
            println!("{}", error);
        }
    }
}
