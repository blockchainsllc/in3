import java.util.*;

import org.bouncycastle.util.encoders.Hex;
import org.bouncycastle.util.encoders.HexEncoder;

import in3.*;
import in3.eth1.*;

import java.io.UnsupportedEncodingException;
import java.math.BigInteger;

public class SendTransaction {
	
	static String TO_ADDR = "0xd46e8dd67c5d32be8058bb8eb970870f07244567";
	static String FROM_ADDR = "0x63FaC9201494f0bd17B9892B9fae4d52fe3BD377";
    static Signer SIGNER = new TransactionSigner();
	public static void main(String[] args) throws Exception {
		// create incubed
		IN3 in3 = IN3.forChain(Chain.MAINNET); // set it to mainnet (which is also dthe default)

		String txnHash = sendTransactionAPI(in3);
		if (null != txnHash) {
			System.out.println(txnHash);
		}

		String rpcResponse = sendTransactionRPC(in3);

		if (null != rpcResponse) {
			System.out.println(rpcResponse);
		}
	}

	private static String sendTransactionRPC(IN3 in3) {

		return in3.sendRPC("eth_sendRawTransaction",
				new Object[] 
						{ 
						"0xf892808609184e72a0008296c094d46e8dd67c5d32be8058bb8eb97"
						+ "0870f07244567849184e72aa9d46e8dd67c5d32be8d46e8dd67c5d32be80"
						+ "58bb8eb970870f072445675058bb8eb970870f07244567526a06f0103fccdc"
						+ "ae0d6b265f8c38ee42f4a722c1cb36230fe8da40315acc30519a8a06252a68b"
						+ "26a5575f76a65ac08a7f684bc37b0c98d9e715d73ddce696b58f2c72"
                        });

	}

	private static String sendTransactionAPI(IN3 in3) {
		// d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675
		TransactionRequest tr = new TransactionRequest();
//		byte [] data = Hex.decode("d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675");
//		try {
//			tr.setData(new String(data,"UTF-8"));
//		} catch (UnsupportedEncodingException e) {
//			// TODO Auto-generated catch block
//			e.printStackTrace();
//		}
		String data = "0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675";
		tr.setData(data);
		tr.setFrom(FROM_ADDR);
		tr.setTo(TO_ADDR);
		tr.setGas(Long.decode("0x96c0"));

		tr.setGasPrice(Long.decode("0x9184e72a000"));
		tr.setValue(BigInteger.valueOf(Long.decode("0x9184e72a")));
		 
        in3.setSigner(SIGNER);
		return in3.getEth1API().sendTransaction(tr);
	}

	

}
