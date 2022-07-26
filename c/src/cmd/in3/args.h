// This is a generated file, please don't edit it manually!

#include <stdlib.h>

const char* bool_props[] = {"includeCode", "debug", "useTxType2", "keepIn3", "stats", "useBinary", "experimental", "autoUpdateList", "bootWeights", "useHttp", "nodes.needsUpdate", "btc.testnet", "clearCache", "eth", "wait", "json", "hex", "debug", "quiet", "human", "test-request", "test-health-request", "response.in", "response.out", "onlysign", "noproof", "nostats", "version", "help", NULL};

const char* help_args = "\
--chainId                     -c     the chainId or the name of a known chain\n\
--finality                    -f     the number in percent needed in order reach finality (% of signature of the validators)\n\
--includeCode                        if true, the request should include the codes of all accounts\n\
--debug                              if true, debug messages will be written to stderr\n\
--maxAttempts                 -a     max number of attempts in case a response is rejected\n\
--useTxType2                         if true send Transaction will create TxType 2 Transactions unless explicitly specifed\n\
--gasPrio                     -g     the factor in percent to be used when calculating the gasPrice (50 = half of average gasPrice, 200 =...\n\
--keepIn3                     -kin3  if true, requests sent to the input sream of the comandline util will be send theor responses in the...\n\
--stats                              if true, requests sent will be used for stats\n\
--useBinary                          if true the client will use binary format\n\
--experimental                -x     if true the client allows to use use experimental features, otherwise a exception is thrown if those...\n\
--timeout                            specifies the number of milliseconds before the request times out\n\
--proof                       -p     if true the nodes should send a proof of the response\n\
--replaceLatestBlock          -l     if specified, the blocknumber *latest* will be replaced by blockNumber- specified value\n\
--autoUpdateList                     if true the nodelist will be automaticly updated if the lastBlock is newer\n\
--signatureCount              -s     number of signatures requested in order to verify the blockhash\n\
--bootWeights                 -bw    if true, the first request (updating the nodelist) will also fetch the current health status and use...\n\
--useHttp                            if true the client will try to use http instead of https\n\
--minDeposit                         min stake of the server\n\
--nodeProps                          used to identify the capabilities of the node\n\
--requestCount                -rc    the number of request send in parallel when getting an answer\n\
--rpc                                url of one or more direct rpc-endpoints to use\n\
--nodes                              defining the nodelist\n\
--nodes.contract                     address of the registry contract\n\
--nodes.whiteListContract            address of the whiteList contract\n\
--nodes.whiteList                    manual whitelist\n\
--nodes.registryId                   identifier of the registry\n\
--nodes.needsUpdate                  if set, the nodeList will be updated before next request\n\
--nodes.avgBlockTime                 average block time (seconds) for this chain\n\
--nodes.verifiedHashes               if the client sends an array of blockhashes the server will not deliver any signatures or blockheade...\n\
--nodes.verifiedHashes.block         block number\n\
--nodes.verifiedHashes.hash          verified hash corresponding to block number\n\
--nodes.nodeList                     manual nodeList\n\
--nodes.nodeList.url                 URL of the node\n\
--nodes.nodeList.address             address of the node\n\
--nodes.nodeList.props               used to identify the capabilities of the node (defaults to 0xFFFF)\n\
--zksync                             configuration for zksync-api  ( only available if build with `-DZKSYNC=true`, which is on per defaul...\n\
--zksync.provider_url         -zks   url of the zksync-server (if not defined it will be choosen depending on the chain)\n\
--zksync.rest_api             -zkr   url of the zksync rest api (if not defined it will be choosen depending on the chain)\n\
--zksync.account              -zka   the account to be used\n\
--zksync.sync_key             -zsk   the seed used to generate the sync_key\n\
--zksync.main_contract               address of the main contract- If not specified it will be taken from the server\n\
--zksync.signer_type          -zkat  type of the account\n\
--zksync.musig_pub_keys       -zms   concatenated packed public keys (32byte) of the musig signers\n\
--zksync.musig_urls           -zmu   a array of strings with urls based on the `musig_pub_keys`\n\
--zksync.create2              -zc2   create2-arguments for sign_type `create2`\n\
--zksync.create2.creator             The address of contract or EOA deploying the contract ( for example the GnosisSafeFactory )\n\
--zksync.create2.saltarg             a salt-argument, which will be added to the pubkeyhash and create the create2-salt\n\
--zksync.create2.codehash            the hash of the actual deploy-tx including the constructor-arguments\n\
--zksync.verify_proof_method  -zvpm  rpc-method, which will be used to verify the incomming proof before cosigning\n\
--zksync.create_proof_method  -zcpm  rpc-method, which will be used to create the proof needed for cosigning\n\
--key                         -k     the client key to sign requests\n\
--pk                          -pk    registers raw private keys as signers for transactions\n\
--pk_ed25519                  -pk_ed25519registers raw private keys as signers for ed25519 transactions\n\
--btc                                configure the Bitcoin verification\n\
--btc.maxDAP                         max number of DAPs (Difficulty Adjustment Periods) allowed when accepting new targets\n\
--btc.maxDiff                        max increase (in percent) of the difference between targets when accepting new targets\n\
--btc.testnet                        If this flag is set to true, all address prefixes and scripts will be set to testnet mode\n\
--clearCache                  -ccacheclears the cache before performing any operation\n\
--eth                         -e     converts the result (as wei) to ether\n\
--port                        -port  if specified it will run as http-server listening to the given port\n\
--allowed-methods             -am    only works if port is specified and declares a comma-seperated list of rpc-methods which are allowed\n\
--block                       -b     the blocknumber to use when making calls\n\
--to                          -to    the target address of the call\n\
--from                        -from  the sender of a call or tx (only needed if no signer is registered)\n\
--data                        -d     the data for a transaction\n\
--gas_price                   -gp    the gas price to use when sending transactions\n\
--gas                         -gas   the gas limit to use when sending transactions\n\
--token                       -token the address of the erc20-token contract\n\
--nonce                       -nonce the nonce\n\
--test                        -test  creates a new json-test written to stdout with the name as specified\n\
--path                        -path  the HD wallet derivation path \n\
--sigtype                     -st    the type of the signature data\n\
--password                    -pwd   password to unlock the key\n\
--value                       -value the value to send when sending a transaction\n\
--wait                        -w     if given, instead returning the transaction, it will wait until the transaction is mined and return ...\n\
--json                        -json  if given the result will be returned as json, which is especially important for eth_call results wit...\n\
--hex                         -hex   if given the result will be returned as hex\n\
--debug                       -debug if given incubed will output debug information when executing\n\
--quiet                       -q     quiet\n\
--human                       -h     human readable, which removes the json -structure and oly displays the values\n\
--test-request                -tr    runs test request when showing in3_weights\n\
--test-health-request         -thr   runs test request including health-check when showing in3_weights\n\
--multisig                    -ms    adds a multisig as signer this needs to be done in the right order! (first the pk then the multisaig...\n\
--ms.signatures               -sigs  add additional signatures, which will be useds when sending through a multisig!\n\
--response.in                 -ri    read response from stdin\n\
--response.out                -ro    write raw response to stdout\n\
--file.in                     -fi    reads a prerecorded request from the filepath and executes it with the recorded data\n\
--file.out                    -fo    records a request and writes the reproducable data in a file (including all cache-data, timestamps \n\
--nodelist                    -nl    a coma seperated list of urls (or address:url) to be used as fixed nodelist\n\
--bootnodes                   -bn    a coma seperated list of urls (or address:url) to be used as boot nodes\n\
--onlysign                    -os    only sign, do not send the raw Transaction\n\
--noproof                     -np    alias for --proof=none\n\
--nostats                     -ns    alias for --stats=false, which will mark all requests as not counting in the stats\n\
--version                     -v     displays the version\n\
--help                        -h     displays this help message\n\
\n\
In addition to the documented rpc-methods, those methods are also supported:\n\
\n\
send <signature> ...args\n\
   based on the -to, -value and -pk a transaction is build, signed and send.\n\
   if there is another argument after send, this would be taken as a function-signature of the smart contract followed by optional argument of the function.\n\
   \n\
call <signature> ...args\n\
   uses eth_call to call a function. Following the call argument the function-signature and its arguments must follow.\n\
   \n\
in3_nodeList returns the nodeList of the Incubed NodeRegistry as json.\n\
in3_sign <blocknumber>\n\
   requests a node to sign. in order to specify the signer, you need to pass the url with -c\n\
   \n\
abi_encode <signature> ...args\n\
   encodes the arguments as described in the method signature using ABI-Encoding\n\
   \n\
abi_decode <signature> data\n\
   decodes the data based on the signature.\n\
   \n\
pk2address <privatekey>\n\
   extracts the public address from a private key\n\
   \n\
pk2public <privatekey>\n\
   extracts the public key from a private key\n\
   \n\
ecrecover <msg> <signature>\n\
   extracts the address and public key from a signature\n\
   \n\
createKey generates a new private key. See in3_createKey.\n\
   \n\
key <keyfile>\n\
   reads the private key from JSON-Keystore file and returns the private key.\n\
   \n\
in3_weights list all current weights and stats\n\
   \n";

const char* aliases[] = {
    "c", "chainId",
    "f", "finality",
    "a", "maxAttempts",
    "g", "gasPrio",
    "kin3", "keepIn3=true",
    "x", "experimental=true",
    "p", "proof",
    "l", "replaceLatestBlock",
    "s", "signatureCount",
    "bw", "bootWeights=true",
    "rc", "requestCount",
    "zks", "zksync.provider_url",
    "zkr", "zksync.rest_api",
    "zka", "zksync.account",
    "zsk", "zksync.sync_key",
    "zkat", "zksync.signer_type",
    "zms", "zksync.musig_pub_keys",
    "zmu", "zksync.musig_urls",
    "zc2", "zksync.create2",
    "zvpm", "zksync.verify_proof_method",
    "zcpm", "zksync.create_proof_method",
    "k", "key",
    "pk", "pk",
    "pk_ed25519", "pk_ed25519",
    "ccache", "clearCache=true",
    "e", "eth=true",
    "port", "port",
    "am", "allowed-methods",
    "b", "block",
    "to", "to",
    "from", "from",
    "d", "data",
    "gp", "gas_price",
    "gas", "gas",
    "token", "token",
    "nonce", "nonce",
    "test", "test",
    "path", "path",
    "st", "sigtype",
    "pwd", "password",
    "value", "value",
    "w", "wait=true",
    "json", "json=true",
    "hex", "hex=true",
    "debug", "debug=true",
    "q", "quiet=true",
    "h", "human=true",
    "tr", "test-request=true",
    "thr", "test-health-request=true",
    "ms", "multisig",
    "sigs", "ms.signatures",
    "ri", "response.in=true",
    "ro", "response.out=true",
    "fi", "file.in",
    "fo", "file.out",
    "nl", "nodelist",
    "bn", "bootnodes",
    "os", "onlysign=true",
    "np", "proof=none",
    "ns", "stats=false",
    "v", "version=true",
    "h", "help=true",
    NULL};
