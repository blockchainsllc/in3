//! Core trait definitions.
use libc::c_char;

use async_trait::async_trait;

use crate::error;

/// Transport trait methods.
///
/// Interface for a facility that encapsulates getting data from a remote endpoint, potentially
/// across network/system boundaries.
#[async_trait]
pub trait Transport {
    /// Sends the request to all of the uris (endpoints) and delivers the response as Strings
    /// in an async fashion.
    async fn fetch(&mut self, request: &str, uris: &[&str]) -> Vec<Result<String, String>>;

    /// Same as fetch() but may block.
    #[cfg(feature = "blocking")]
    fn fetch_blocking(&mut self, request: &str, uris: &[&str]) -> Vec<Result<String, String>>;
}

/// Signer trait methods.
///
/// Interface for a utility that can cryptographically sign arbitrary data thereby providing a
/// means of authentication and non-repudiation.
pub trait Signer {
    /// Signs the message.
    fn sign(&mut self, msg: &str) -> *const c_char;
}

/// Storage trait methods.
///
/// Interface for a key-value style data store.
pub trait Storage {
    /// Gets the value (as bytes) that corresponds to a given key from storage.
    fn get(&self, key: &str) -> Option<Vec<u8>>;

    /// Sets the value for given key.
    fn set(&mut self, key: &str, value: &[u8]);

    /// Clears the storage by deleting are KV pairs.
    fn clear(&mut self);
}

/// Client trait methods.
///
/// Interface for a RPC capable client with configurable transport, signing and storage
/// facilities.
#[async_trait(? Send)]
pub trait Client {
    /// Configures the client using the given config string.
    fn configure(&mut self, config: &str) -> Result<(), String>;

    /// Sets a custom transport implementation to be used by the client.
    fn set_transport(&mut self, transport: Box<dyn Transport>);

    /// Sets a custom signer implementation to be used by the client.
    fn set_signer(&mut self, signer: Box<dyn Signer>);

    /// Sets a custom storage implementation to be used by the client.
    fn set_storage(&mut self, storage: Box<dyn Storage>);

    /// Makes a remote procedure call and returns the result as a String asynchronously.
    async fn rpc(&mut self, call: &str) -> error::In3Result<String>;

    /// Same as rpc() but may block.
    #[cfg(feature = "blocking")]
    fn rpc_blocking(&mut self, call: &str) -> error::In3Result<String>;

    /// Sets the private key that must be used for signing.
    fn set_pk_signer(&mut self, data: &str);
}

/// Api trait methods.
///
/// Interface for a facility that provides a user API using the RPC Client's services.
pub trait Api {
    /// Create an API instance by consuming a Client
    fn new(client: Box<dyn Client>) -> Self;

    /// Get a mutable reference to the client, for eg. to configure it dynamically.
    fn client(&mut self) -> &mut Box<dyn Client>;
}
