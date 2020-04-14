// Contains code common to the build script and main crate
//
// Needs to be included with include! macro

#[derive(Clone, Copy, PartialEq, Eq, Debug, Hash)]
/// Information specific to architecture
pub struct In3ArchInfo<'a> {
    /// name of C header
    header_name: &'a str,

    /// name used within in3 C library
    cs_name: &'a str,
}

impl<'a> In3ArchInfo<'a> {
    /// Get the name of the C header
    pub fn header_name(&self) -> &str {
        self.header_name
    }

    /// Get the arch name used in In3 types
    pub fn cs_name(&self) -> &str {
        self.cs_name
    }
}

pub static BINDINGS_FILE: &'static str = "in3.rs";
