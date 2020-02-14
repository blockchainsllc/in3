import java.util.*;
import in3.*;
import in3.eth1.*;
import java.math.BigInteger;

public class GetTransactionReceipt {
	static String TRANSACTION_HASH = "0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab5252287c34bc5d12457eab0e";

	//
	public static void main(String[] args) throws Exception {
		// create incubed
		IN3 in3 = IN3.forChain(Chain.MAINNET); // set it to mainnet (which is also dthe default)

		TransactionReceipt txn = GetTransactionReceiptAPI(in3);
		if (null != txn) {
			System.out.println("Transaction #" + txn.getTransactionIndex() + " Block num " + txn.getBlockNumber()
					+ " Gas used " + txn.getGasUsed() + " status " + txn.getStatus());
		}

		String rpcResponse = GetTransactionReceiptRPC(in3);

		if (null != rpcResponse) {
			System.out.println(rpcResponse);
		}
	}

	static TransactionReceipt GetTransactionReceiptAPI(IN3 in3) {

		return in3.getEth1API().getTransactionReceipt(TRANSACTION_HASH);

	}

	static String GetTransactionReceiptRPC(IN3 in3) {

		return in3.sendRPC("eth_getTransactionReceipt", new Object[] { TRANSACTION_HASH

		});
	}

}
