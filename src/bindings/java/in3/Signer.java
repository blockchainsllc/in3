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

package in3;

import in3.IN3;
import in3.eth1.TransactionRequest;

/**
 * a Interface responsible for signing data or transactions.
 */
public interface Signer {
    /**
     * optiional method which allows to change the transaction-data before sending
     * it. This can be used for redirecting it through a multisig.
     */
    TransactionRequest prepareTransaction(IN3 in3, TransactionRequest tx);

    /** returns true if the account is supported (or unlocked) */
    boolean hasAccount(String address);

    /** signing of the raw data. */
    String sign(String data, String address);

}
