//! Minimal logger implementation that interfaces with the underlying C impl
use in3_sys::{in3_log_disable_prefix_, in3_log_enable_prefix_, in3_log_level_t, in3_log_set_quiet_};

pub enum FilterLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
}

// The `()` member prevents struct literal initialization
pub struct Log(());

impl Log {
    pub fn init() -> Log {
        let mut log = Log { 0: () };
        log.quiet(false);
        log
    }

    pub fn quiet(&mut self, enable: bool) {
        unsafe { in3_log_set_quiet_(enable.into()) }
    }

    pub fn prefix(&mut self, enable: bool) {
        unsafe { if enable { in3_log_enable_prefix_() } else { in3_log_disable_prefix_() } }
    }

    pub fn set_level(&mut self, level: FilterLevel) {
        unsafe { in3_sys::in3_log_set_level_(level.into()) }
    }

    pub fn get_level(&self) -> FilterLevel {
        unsafe { in3_sys::in3_log_get_level_().into() }
    }

    fn log(&mut self, level: FilterLevel, message: &str) {
        unsafe {
            in3_sys::in3_log_(level.into(),
                              format!("{}", file!()).as_ptr() as *const libc::c_char,
                              format!("{}", column!()).as_ptr() as *const libc::c_char,
                              line!() as i32,
                              message.as_ptr() as *const i8).into()
        }
    }

    pub fn trace(&mut self, message: &str) { self.log(FilterLevel::Trace, message) }

    pub fn debug(&mut self, message: &str) { self.log(FilterLevel::Debug, message) }

    pub fn info(&mut self, message: &str) { self.log(FilterLevel::Info, message) }

    pub fn warn(&mut self, message: &str) { self.log(FilterLevel::Warn, message) }

    pub fn error(&mut self, message: &str) { self.log(FilterLevel::Error, message) }
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
