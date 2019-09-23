/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 *
 * Copyright (C) 2019 Blockchains, LLC
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
 * 
 * If you cannot meet the requirements of AGPL, 
 * you should contact us to inquire about a commercial license.
 *******************************************************************************/

package in3.eth1;

import in3.*;
import java.math.*;

/**
 * represents a Transaction in ethereum.
 * 
 */

public class Transaction {

    private JSON data;

    private Transaction(JSON data) {
        this.data = data;
    }

    protected static Transaction asTransaction(Object o) {
        return o == null ? null : new Transaction((JSON) o);
    }

    /**
     * the blockhash of the block containing this transaction.
     */
    public String getBlockHash() {
        return data.getString("blockHash");
    }

    /**
     * the block number of the block containing this transaction.
     */
    public long getBlockNumber() {
        return data.getLong("blockNumber");
    }

    /**
     * the chainId of this transaction.
     */
    public String getChainId() {
        return data.getString("chainId");
    }

    /**
     * the address of the deployed contract (if successfull)
     */
    public String getCreatedContractAddress() {
        return data.getString("creates");
    }

    /**
     * the address of the sender.
     */
    public String getFrom() {
        return data.getString("from");
    }

    /**
     * the Transaction hash.
     */
    public String getHash() {
        return data.getString("hash");
    }

    /**
     * the Transaction data or input data.
     */
    public String getData() {
        return data.getString("input");
    }

    /**
     * the nonce used in the transaction.
     */
    public long getNonce() {
        return data.getLong("nonce");
    }

    /**
     * the public key of the sender.
     */
    public String getPublicKey() {
        return data.getString("publicKey");
    }

    /**
     * the value send in wei.
     */
    public BigInteger getValue() {
        return data.getBigInteger("value");
    }

    /**
     * the raw transaction as rlp encoded data.
     */
    public String getRaw() {
        return data.getString("raw");
    }

    /**
     * the address of the receipient or contract.
     */
    public String getTo() {
        return data.getString("to");
    }

    /**
     * the signature of the sender - a array of the [ r, s, v]
     */
    public String[] getSignature() {
        return new String[] { data.getString("r"), data.getString("s"), data.getString("v") };
    }

    /**
     * the gas price provided by the sender.
     */
    public long getGasPrice() {
        return data.getLong("gasPrice");
    }

    /**
     * the gas provided by the sender.
     */
    public long getGas() {
        return data.getLong("gas");
    }
}
