//! Types common to all modules.
use std::convert::TryFrom;
use std::fmt;
use std::fmt::Formatter;

use rustc_hex::{FromHex, FromHexError, ToHex};
use serde::{Deserialize, Deserializer, Serialize, Serializer};
use serde::de::{Error, Visitor};

/// Newtype wrapper around vector of bytes
#[derive(PartialEq, Eq, Default, Hash, Clone)]
pub struct Bytes(pub Vec<u8>);

impl fmt::Debug for Bytes {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        let mut serialized = "0x".to_owned();
        serialized.push_str(self.0.to_hex().as_str());
        write!(f, "{}", serialized)
    }
}

impl TryFrom<&str> for Bytes {
    type Error = FromHexError;

    fn try_from(s: &str) -> Result<Self, Self::Error> {
        Ok(s.from_hex()?.into())
    }
}

impl From<Vec<u8>> for Bytes {
    fn from(vec: Vec<u8>) -> Bytes {
        Bytes(vec)
    }
}

impl From<&[u8]> for Bytes {
    fn from(slice: &[u8]) -> Bytes {
        Bytes(slice.to_vec())
    }
}

impl From<in3_sys::bytes> for Bytes {
    fn from(bytes: in3_sys::bytes) -> Bytes {
        let slice = unsafe { std::slice::from_raw_parts(bytes.data, bytes.len as usize) };
        Bytes(slice.to_vec())
    }
}

impl Serialize for Bytes {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: Serializer,
    {
        let mut serialized = "0x".to_owned();
        serialized.push_str(self.0.to_hex().as_str());
        serializer.serialize_str(serialized.as_ref())
    }
}

impl<'a> Deserialize<'a> for Bytes {
    fn deserialize<D>(deserializer: D) -> Result<Bytes, D::Error>
        where
            D: Deserializer<'a>,
    {
        deserializer.deserialize_str(BytesVisitor)
    }
}

struct BytesVisitor;

impl<'a> Visitor<'a> for BytesVisitor {
    type Value = Bytes;

    fn expecting(&self, formatter: &mut Formatter) -> fmt::Result {
        write!(formatter, "a hex string (optionally prefixed with '0x')")
    }

    fn visit_str<E>(self, value: &str) -> Result<Self::Value, E> where E: Error {
        let start = if value.starts_with("0x") { 2 } else { 0 };
        Ok(FromHex::from_hex(&value[start..]).map_err(|e| Error::custom(format!("Invalid hex: {}", e)))?.into())
    }
}
