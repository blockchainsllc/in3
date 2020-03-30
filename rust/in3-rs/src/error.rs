use core::fmt;
use core::result;
use std::ffi;

use in3_sys::in3_ret_t;

#[must_use]
pub type Result<T> = result::Result<T, Error>;

#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq)]
pub enum Error {
    Ok = in3_ret_t::IN3_OK as isize,
    NoMemory = in3_ret_t::IN3_ENOMEM as isize,
    UnknownError = in3_ret_t::IN3_EUNKNOWN as isize,
    NotSupported = in3_ret_t::IN3_ENOTSUP as isize,
    InvalidValue = in3_ret_t::IN3_EINVAL as isize,
    NotFound = in3_ret_t::IN3_EFIND as isize,
    InvalidConfig = in3_ret_t::IN3_ECONFIG as isize,
    LimitReached = in3_ret_t::IN3_ELIMIT as isize,
    VersionMismatch = in3_ret_t::IN3_EVERS as isize,
    DataInvalid = in3_ret_t::IN3_EINVALDT as isize,
    WrongPassword = in3_ret_t::IN3_EPASS as isize,
    RpcError = in3_ret_t::IN3_ERPC as isize,
    RpcNoResponse = in3_ret_t::IN3_ERPCNRES as isize,
    UsnUrlParseError = in3_ret_t::IN3_EUSNURL as isize,
    TransportError = in3_ret_t::IN3_ETRANS as isize,
    OutOfRange = in3_ret_t::IN3_ERANGE as isize,
    Waiting = in3_ret_t::IN3_WAITING as isize,
    IgnorableError = in3_ret_t::IN3_EIGNORE as isize,
}

impl From<in3_sys::in3_ret_t> for Error {
    fn from(err: in3_sys::in3_ret_t) -> Self {
        use Error::*;
        match err {
            in3_ret_t::IN3_OK => Error::Ok,
            in3_ret_t::IN3_ENOMEM => NoMemory,
            in3_ret_t::IN3_EUNKNOWN => UnknownError,
            in3_ret_t::IN3_ENOTSUP => NotSupported,
            in3_ret_t::IN3_EINVAL => InvalidValue,
            in3_ret_t::IN3_EFIND => NotFound,
            in3_ret_t::IN3_ECONFIG => InvalidConfig,
            in3_ret_t::IN3_ELIMIT => LimitReached,
            in3_ret_t::IN3_EVERS => VersionMismatch,
            in3_ret_t::IN3_EINVALDT => DataInvalid,
            in3_ret_t::IN3_EPASS => WrongPassword,
            in3_ret_t::IN3_ERPC => RpcError,
            in3_ret_t::IN3_ERPCNRES => RpcNoResponse,
            in3_ret_t::IN3_EUSNURL => UsnUrlParseError,
            in3_ret_t::IN3_ETRANS => TransportError,
            in3_ret_t::IN3_ERANGE => OutOfRange,
            in3_ret_t::IN3_WAITING => Waiting,
            in3_ret_t::IN3_EIGNORE => IgnorableError,
        }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
        write!(fmt, "{}", self.description())
    }
}

impl Error {
    fn description(&self) -> &str {
        unsafe {
            return ffi::CStr::from_ptr(in3_sys::in3_errmsg(self.into())).to_str().unwrap();
        }
    }
}

impl From<&Error> for in3_sys::in3_ret_t {
    fn from(err: &Error) -> Self {
        use Error::*;
        match err {
            Ok => in3_ret_t::IN3_OK,
            NoMemory => in3_ret_t::IN3_ENOMEM,
            UnknownError => in3_ret_t::IN3_EUNKNOWN,
            NotSupported => in3_ret_t::IN3_ENOTSUP,
            InvalidValue => in3_ret_t::IN3_EINVAL,
            NotFound => in3_ret_t::IN3_EFIND,
            InvalidConfig => in3_ret_t::IN3_ECONFIG,
            LimitReached => in3_ret_t::IN3_ELIMIT,
            VersionMismatch => in3_ret_t::IN3_EVERS,
            DataInvalid => in3_ret_t::IN3_EINVALDT,
            WrongPassword => in3_ret_t::IN3_EPASS,
            RpcError => in3_ret_t::IN3_ERPC,
            RpcNoResponse => in3_ret_t::IN3_ERPCNRES,
            UsnUrlParseError => in3_ret_t::IN3_EUSNURL,
            TransportError => in3_ret_t::IN3_ETRANS,
            OutOfRange => in3_ret_t::IN3_ERANGE,
            Waiting => in3_ret_t::IN3_WAITING,
            IgnorableError => in3_ret_t::IN3_EIGNORE,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Error;

    #[test]
    fn test_error() {
        let errors = [
            Error::NoMemory,
            Error::UnknownError,
        ];

        for error in errors.iter() {
            println!("{}", error);
        }
    }
}