//! Core trait definitions.
use async_trait::async_trait;

use crate::error::In3Result;
use crate::types::Bytes;

/// Identifier for client type to allow for user-defined classification of Clients
type ClientTypeId = u32;

/// Transport trait methods.
///
/// Interface for a facility that encapsulates getting data from a remote endpoint, potentially
/// across network/system boundaries.
#[async_trait]
pub trait Transport {
    /// Sends the request to all of the uris (endpoints) and delivers the response as Strings
    /// in an async fashion.
    async fn fetch(&mut self, method: &str, request: &str, uris: &[&str],headers: &[&str]) -> Vec<Result<String, String>>;

    /// Same as fetch() but may block.
    #[cfg(feature = "blocking")]
    fn fetch_blocking(&mut self, method: &str, request: &str, uris: &[&str],headers: &[&str]) -> Vec<Result<String, String>>;
}

/// Signer trait methods.
///
/// Interface for a utility that can cryptographically sign arbitrary data thereby providing a
/// means of authentication and non-repudiation.
#[async_trait(? Send)]
pub trait Signer {
    /// Returns signed message.
    async fn sign(&mut self, msg: Bytes) -> In3Result<Bytes>;

    /// Transforms message before signing. (Optional)
    async fn prepare(&mut self, msg: Bytes) -> In3Result<Bytes> {
        Ok(msg)
    }
}

/// Storage trait methods.
///
/// Interface for a key-value style data store.
pub trait Storage {
    /// Gets the value (as bytes) that corresponds to a given key from storage.
    fn get(&self, key: &str) -> Option<Vec<u8>>;

    /// Sets the value for given key.
    fn set(&mut self, key: &str, value: &[u8]);

    /// Clears the storage by deleting all KV pairs.
    fn clear(&mut self);
}

/// Client trait methods.
///
/// Interface for a RPC capable client with configurable transport, signing and storage
/// facilities.
#[async_trait(? Send)]
pub trait Client {
    /// Returns the client's type id.
    fn id(&self) -> ClientTypeId;

    /// Configures the client using the given config string.
    fn configure(&mut self, config: &str) -> In3Result<()>;

    /// Sets a custom transport implementation to be used by the client.
    fn set_transport(&mut self, transport: Box<dyn Transport>);

    /// Sets a custom signer implementation to be used by the client.
    fn set_signer(&mut self, signer: Box<dyn Signer>);

    /// Sets a custom storage implementation to be used by the client.
    fn set_storage(&mut self, storage: Box<dyn Storage>);

    /// Makes a remote procedure call and returns the result as a String asynchronously.
    async fn rpc(&mut self, call: &str) -> In3Result<String>;

    /// Same as rpc() but may block.
    #[cfg(feature = "blocking")]
    fn rpc_blocking(&mut self, call: &str) -> In3Result<String>;
}

/// Api trait methods.
///
/// Interface for a facility that provides a user API using the RPC Client's services.
pub trait Api {
    /// Create an API instance by consuming a Client.
    fn new(client: Box<dyn Client>) -> Self;

    /// Get a mutable reference to the client, for eg. to configure it dynamically.
    fn client(&mut self) -> &mut Box<dyn Client>;
}
