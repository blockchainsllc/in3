//! Minimal logger implementation that interfaces with the underlying C impl
use std::ffi::CString;

use in3_sys::{
    in3_log_disable_prefix_, in3_log_enable_prefix_, in3_log_level_t, in3_log_set_quiet_,
};

/// Singleton for logging, wraps the underlying C log impl.
/// Direct access is discouraged, instead use helpers like `enable()`, `disable()`, `set_level()`
pub static mut LOGGER: Log = Log { 0: () };

/// Enable logging
pub fn enable() {
    unsafe {
        LOGGER.quiet(false);
    }
}

/// Disable logging
pub fn disable() {
    unsafe {
        LOGGER.quiet(true);
    }
}

/// Set log level to `level`
pub fn set_level(level: FilterLevel) {
    unsafe {
        LOGGER.set_level(level);
    }
}

/// Logging filter level
pub enum FilterLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
}

/// Log type, direct usage is discouraged.
// The `()` member prevents struct literal initialization
pub struct Log(());

impl Log {
    pub unsafe fn quiet(&mut self, enable: bool) {
        in3_log_set_quiet_(enable.into())
    }

    pub unsafe fn prefix(&mut self, enable: bool) {
        if enable {
            in3_log_enable_prefix_()
        } else {
            in3_log_disable_prefix_()
        }
    }

    pub unsafe fn set_level(&mut self, level: FilterLevel) {
        in3_sys::in3_log_set_level_(level.into())
    }

    pub unsafe fn get_level(&self) -> FilterLevel {
        in3_sys::in3_log_get_level_().into()
    }

    unsafe fn log(&mut self, level: FilterLevel, message: &str) {
        let file = CString::new(format!("{}", file!())).unwrap();
        let column = CString::new(format!("{}", column!())).unwrap();
        let message = CString::new(format!("{}", message)).unwrap();
        in3_sys::in3_log_(
            level.into(),
            file.as_ptr() as *const libc::c_char,
            column.as_ptr() as *const libc::c_char,
            line!() as i32,
            message.as_ptr() as *const libc::c_char,
        )
        .into()
    }

    pub(crate) unsafe fn trace(&mut self, message: &str) {
        self.log(FilterLevel::Trace, message)
    }

    pub(crate) unsafe fn debug(&mut self, message: &str) {
        self.log(FilterLevel::Debug, message)
    }

    pub(crate) unsafe fn info(&mut self, message: &str) {
        self.log(FilterLevel::Info, message)
    }

    pub(crate) unsafe fn warn(&mut self, message: &str) {
        self.log(FilterLevel::Warn, message)
    }

    pub(crate) unsafe fn error(&mut self, message: &str) {
        self.log(FilterLevel::Error, message)
    }
}

impl From<FilterLevel> for in3_log_level_t {
    fn from(level: FilterLevel) -> Self {
        match level {
            FilterLevel::Trace => Self::LOG_TRACE,
            FilterLevel::Debug => Self::LOG_DEBUG,
            FilterLevel::Info => Self::LOG_INFO,
            FilterLevel::Warn => Self::LOG_WARN,
            FilterLevel::Error => Self::LOG_ERROR,
            FilterLevel::Fatal => Self::LOG_FATAL,
        }
    }
}

impl From<in3_log_level_t> for FilterLevel {
    fn from(level: in3_log_level_t) -> Self {
        match level {
            in3_log_level_t::LOG_TRACE => Self::Trace,
            in3_log_level_t::LOG_DEBUG => Self::Debug,
            in3_log_level_t::LOG_INFO => Self::Info,
            in3_log_level_t::LOG_WARN => Self::Warn,
            in3_log_level_t::LOG_ERROR => Self::Error,
            in3_log_level_t::LOG_FATAL => Self::Fatal,
        }
    }
}
