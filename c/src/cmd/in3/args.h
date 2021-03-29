const char* help_args = "\
--chainId                     -c    the chainId or the name of a known chain\n\
--finality                    -f    the number in percent needed in order reach finality (% of signature of the validators)\n\
--includeCode                       if true, the request should include the codes of all accounts\n\
--maxAttempts                 -a    max number of attempts in case a response is rejected\n\
--keepIn3                           if true, requests sent to the input sream of the comandline util will be send theor responses in the...\n\
--useBinary                         if true the client will use binary format\n\
--experimental                -x    iif true the client allows to use use experimental features, otherwise a exception is thrown if thos...\n\
--timeout                           specifies the number of milliseconds before the request times out\n\
--proof                       -p    if true the nodes should send a proof of the response\n\
--replaceLatestBlock          -l    if specified, the blocknumber *latest* will be replaced by blockNumber- specified value\n\
--autoUpdateList                    if true the nodelist will be automaticly updated if the lastBlock is newer\n\
--signatureCount              -s    number of signatures requested in order to verify the blockhash\n\
--bootWeights                       if true, the first request (updating the nodelist) will also fetch the current health status and use...\n\
--useHttp                           if true the client will try to use http instead of https\n\
--minDeposit                        min stake of the server\n\
--nodeProps                         used to identify the capabilities of the node\n\
--requestCount                -rc   the number of request send in parallel when getting an answer\n\
--rpc                               url of one or more direct rpc-endpoints to use\n\
--nodes                             defining the nodelist\n\
--nodes.contract                    address of the registry contract\n\
--nodes.whiteListContract           address of the whiteList contract\n\
--nodes.whiteList                   manual whitelist\n\
--nodes.registryId                  identifier of the registry\n\
--nodes.needsUpdate                 if set, the nodeList will be updated before next request\n\
--nodes.avgBlockTime                average block time (seconds) for this chain\n\
--nodes.verifiedHashes              if the client sends an array of blockhashes the server will not deliver any signatures or blockheade...\n\
--nodes.verifiedHashes.block        block number\n\
--nodes.verifiedHashes.hash         verified hash corresponding to block number\n\
--nodes.nodeList                    manual nodeList\n\
--nodes.nodeList.url                URL of the node\n\
--nodes.nodeList.address            address of the node\n\
--nodes.nodeList.props              used to identify the capabilities of the node (defaults to 0xFFFF)\n\
--zksync                            configuration for zksync-api  ( only available if build with `-DZKSYNC=true`, which is on per defaul...\n\
--zksync.provider_url         -zks  url of the zksync-server (if not defined it will be choosen depending on the chain)\n\
--zksync.account              -zka  the account to be used\n\
--zksync.sync_key             -zsk  the seed used to generate the sync_key\n\
--zksync.main_contract              address of the main contract- If not specified it will be taken from the server\n\
--zksync.signer_type          -zkat type of the account\n\
--zksync.musig_pub_keys       -zms  concatenated packed public keys (32byte) of the musig signers\n\
--zksync.musig_urls           -zmu  a array of strings with urls based on the `musig_pub_keys`\n\
--zksync.create2              -zc2  create2-arguments for sign_type `create2`\n\
--zksync.create2.creator            The address of contract or EOA deploying the contract ( for example the GnosisSafeFactory )\n\
--zksync.create2.saltarg            a salt-argument, which will be added to the pubkeyhash and create the create2-salt\n\
--zksync.create2.codehash           the hash of the actual deploy-tx including the constructor-arguments\n\
--key                         -k    the client key to sign requests\n\
--pk                          -pk   registers raw private keys as signers for transactions\n\
--btc                               configure the Bitcoin verification\n\
--btc.maxDAP                        max number of DAPs (Difficulty Adjustment Periods) allowed when accepting new targets\n\
--btc.maxDiff                       max increase (in percent) of the difference between targets when accepting new targets\n";

 const char* aliases[] = {
    "c","chainId",
    "f","finality",
    "a","maxAttempts",
    "x","experimental=true",
    "p","proof",
    "l","replaceLatestBlock",
    "s","signatureCount",
    "rc","requestCount",
    "zks","zksync.provider_url",
    "zka","zksync.account",
    "zsk","zksync.sync_key",
    "zkat","zksync.signer_type",
    "zms","zksync.musig_pub_keys",
    "zmu","zksync.musig_urls",
    "zc2","zksync.create2",
    "k","key",
    "pk","pk",
    NULL};
