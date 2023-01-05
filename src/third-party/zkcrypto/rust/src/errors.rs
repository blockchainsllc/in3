#[derive(Debug, PartialEq)]
pub enum MusigABIError {
    MuSigError,
    InvalidInputData,
    EncodingError,
}

impl MusigABIError {
    pub fn description(&self) -> &str {
        match *self {
            MusigABIError::InvalidInputData => "Invalid input length",
            MusigABIError::MuSigError => "Error propogated from original musig",
            MusigABIError::EncodingError => "Can't encode output",
        }
    }
}

impl std::error::Error for MusigABIError {}

impl std::fmt::Display for MusigABIError {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> Result<(), std::fmt::Error> {
        write!(f, "{}", self.description())
    }
}
