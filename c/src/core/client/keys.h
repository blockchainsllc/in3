/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

// clang-format off
#define K_ID                key("id")
#define K_METHOD            key("method")
#define K_PARAMS            key("params")
#define K_ERROR             key("error")
#define K_MESSAGE           key("message")
#define K_EXEC_TIME         key("execTime")
#define K_RPC_TIME          key("rpcTime")
#define K_RPC_COUNT         key("rpcCount")
#define K_CURRENT_BLOCK     key("currentBlock")
#define K_VERSION           key("version")

#define K_RESULT            key("result")
#define K_IN3               key("in3")
#define K_PROOF             key("proof")
#define K_REQUEST_COUNT     key("requestCount")

#define K_NODES             key("nodes")
#define K_LAST_BLOCK_NUMBER key("lastBlockNumber")
#define K_LAST_NODE_LIST    key("lastNodeList")
#define K_LAST_WHITE_LIST    key("lastWhiteList")

#define K_CAPACITY          key("capacity")
#define K_INDEX             key("index")
#define K_DEPOSIT           key("deposit")
#define K_PROPS             key("props")
#define K_WEIGHT            key("weight")
#define K_URL               key("url")
#define K_ADDRESS           key("address")
#define K_CODE              key("code")

#define K_BLOCK             key("block")
#define K_BLOCK_HASH        key("blockHash")
#define K_BLOCK_NUMBER      key("blockNumber")
#define K_TX_INDEX          key("txIndex")
#define K_TX_HASH           key("txHash")
#define K_MERKLE_PROOF      key("merkleProof")
#define K_TX_PROOF          key("txProof")

#define K_MSG_HASH          key("msgHash")
#define K_R                 key("r")
#define K_S                 key("s")
#define K_V                 key("v")

#define K_NONCE             key("nonce")
#define K_GAS               key("gas")
#define K_GAS_PRICE         key("gasPrice")
#define K_TO                key("to")
#define K_VALUE             key("value")
#define K_INPUT             key("input")
#define K_DATA              key("data")
#define K_STATUS            key("status")
#define K_ROOT              key("root")
#define K_CUMULATIVE_GAS_USED key("cumulativeGasUsed")
#define K_LOGS              key("logs")
#define K_LOG_INDEX         key("logIndex")
#define K_TOPICS            key("topics")
#define K_TRANSACTION_LOG_INDEX key("transactionLogIndex")

#define K_SIGNATURES        key("signatures")

#define K_TRANSACTION_INDEX key("transactionIndex")
#define K_TRANSACTION_HASH  key("transactionHash")

#define K_HASH              key("hash")
#define K_NUMBER            key("number")
#define K_PARENT_HASH       key("parentHash")
#define K_SHA3_UNCLES       key("sha3Uncles")
#define K_MINER             key("miner")
#define K_AUTHOR            key("author")
#define K_COINBASE          key("coinbase")
#define K_STATE_ROOT        key("stateRoot")
#define K_TRANSACTIONS_ROOT key("transactionsRoot")
#define K_RECEIPTS_ROOT     key("receiptsRoot")
#define K_RECEIPT_ROOT      key("receiptRoot")
#define K_LOGS_BLOOM        key("logsBloom")
#define K_DIFFICULTY        key("difficulty")
#define K_GAS_LIMIT         key("gasLimit")
#define K_GAS_USED          key("gasUsed")
#define K_TIMESTAMP         key("timestamp")
#define K_TIMEOUT           key("timeout")
#define K_REGISTER_TIME     key("registerTime")
#define K_EXTRA_DATA        key("extraData")
#define K_SEAL_FIELDS       key("sealFields")
#define K_MIX_HASH          key("mixHash")
#define K_TRANSACTIONS      key("transactions")
#define K_UNCLES            key("uncles")

#define K_PUBLIC_KEY        key("publicKey")
#define K_CHAIN_ID          key("chainId")
#define K_RAW               key("raw")
#define K_FROM              key("from")
#define K_STANDARD_V        key("standardV")

#define K_FINALITY_BLOCKS   key("finalityBlocks")

//# chainspec
#define K_ENGINE            key("engine")
#define K_VALIDATOR_LIST    key("validatorList")

#define K_TOTAL_SERVERS     key("totalServers")
#define K_VALIDATOR_CONTRACT key("validatorContract")
#define K_VALIDATORS        key("validators")
#define K_STATES            key("states")
#define K_LAST_VALIDATOR_CHANGE key("lastValidatorChange")
#define K_LIST              key("list")
#define K_BYPASS_FINALITY   key("bypassFinality")

#define K_CODE_HASH         key("codeHash")
#define K_ACCOUNT_PROOF     key("accountProof")
#define K_ACCOUNTS          key("accounts")
#define K_TYPE              key("type")
#define K_BALANCE           key("balance")
#define K_STORAGE_HASH      key("storageHash")
#define K_KEY               key("key")
#define K_STORAGE_PROOF     key("storageProof")

#define K_CONTRACT          key("contract")
#define K_CONTRACT_ADDRESS  key("contractAddress")
#define K_LOG_PROOF         key("logProof")
#define K_RECEIPTS          key("receipts")
#define K_REMOVED           key("removed")

#define K_FROM_BLOCK        key("fromBlock")
#define K_TO_BLOCK          key("toBlock")
#define K_SIGNER_NODES      key("signerNodes")
#define K_DATA_NODES        key("dataNodes")
