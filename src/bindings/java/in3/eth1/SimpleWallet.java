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

import java.util.HashMap;
import java.util.Map;

import in3.IN3;
import in3.Signer;

/**
 * a simple Implementation for holding private keys to sing data or
 * transactions.
 */
public class SimpleWallet implements Signer {

    // ke for holding the map
    Map<String, String> privateKeys = new HashMap<String, String>();

    /**
     * adds a key to the wallet and returns its public address.
     */
    public String addRawKey(String data) {
        String address = getAddressFromKey(data);
        // create address
        privateKeys.put(address.toLowerCase(), data);
        return address;
    }

    /**
     * adds a key to the wallet and returns its public address.
     */
    public String addKeyStore(String jsonData, String passphrase) {
        String data = decodeKeystore(jsonData, passphrase);
        if (data == null)
            throw new RuntimeException("Invalid password");
        // create address
        return addRawKey(data);
    }

    /**
     * optiional method which allows to change the transaction-data before sending
     * it. This can be used for redirecting it through a multisig.
     */
    public TransactionRequest prepareTransaction(IN3 in3, TransactionRequest tx) {
        // TODO here you could transform the data in order to support multisigs.
        // for now we don't manipulate the data.
        return tx;
    }

    /** returns true if the account is supported (or unlocked) */
    public boolean hasAccount(String address) {
        return privateKeys.containsKey(address.toLowerCase());
    }

    /** signing of the raw data. */
    public String sign(String data, String address) {
        String key = privateKeys.get(address.toLowerCase());
        return signData(key, data);
    }

    private static native String getAddressFromKey(String key);

    private static native String signData(String key, String data);

    private static native String decodeKeystore(String keystore, String passwd);

}
