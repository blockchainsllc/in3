import java.io.UnsupportedEncodingException;
import java.math.BigInteger;
import java.security.InvalidKeyException;
import java.security.KeyFactory;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.PrivateKey;
import java.security.Security;
import java.security.Signature;
import java.security.SignatureException;
import java.security.spec.InvalidKeySpecException;

import org.bouncycastle.jce.ECNamedCurveTable;
import org.bouncycastle.jce.provider.BouncyCastleProvider;
import org.bouncycastle.jce.spec.ECNamedCurveParameterSpec;
import org.bouncycastle.jce.spec.ECPrivateKeySpec;

import in3.IN3;
import in3.Signer;
import in3.eth1.TransactionRequest;

public class TransactionSigner implements Signer{

	// TODO: This is only for the sake of demo. Do NOT store private keys as
		// plaintext.
   
   static String ETH_PRIVATE_KEY = "0x8da4ef21b864d2cc526dbdb2a120bd2874c36c9d0a1fb7f8c63d7f7a8b41de8f";
   static {
   Security.addProvider(new BouncyCastleProvider());
   }
	@Override
	public TransactionRequest prepareTransaction(IN3 in3, TransactionRequest tx) {
		// TODO Auto-generated method stub
		return tx;
	}

	@Override
	public boolean hasAccount(String address) {
		// TODO Auto-generated method stub
		return true;
	}

	@Override
	public String sign(String data, String address) {
		KeyFactory factory;
		try {
			factory = KeyFactory.getInstance("ECDSA", "BC");
			ECNamedCurveParameterSpec spec = ECNamedCurveTable.getParameterSpec("prime256v1");
			ECPrivateKeySpec ecPrivateKeySpec = new org.bouncycastle.jce.spec.ECPrivateKeySpec(new BigInteger( ETH_PRIVATE_KEY), spec);
			PrivateKey privateKey = factory.generatePrivate(ecPrivateKeySpec);
			
			 Signature dsa = Signature.getInstance("SHA1withECDSA");
			 dsa.initSign(privateKey);
			 dsa.update(data.getBytes("UTF-8"));
			 byte[] signature = dsa.sign();
			 String signatureStr = new String(signature, "UTF-8");
			 System.out.println("Signature : "+signatureStr);
			 return signatureStr;
		} catch (NoSuchAlgorithmException | NoSuchProviderException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (InvalidKeySpecException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (SignatureException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (InvalidKeyException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (UnsupportedEncodingException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		return null;
	}

}
