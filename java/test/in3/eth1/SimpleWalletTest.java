package in3.eth1;

import in3.eth1.SimpleWallet;
import in3.utils.PayloadType;
import in3.utils.SignatureType;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

public class SimpleWalletTest {
  private SimpleWallet wallet;

  @BeforeEach
  public void setupSigner() {
    wallet = new SimpleWallet();
  }

  @Test
  public void sign() {
    String pk                = "0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6";
    String address           = wallet.addRawKey(pk);
    byte[] expectedSignature = new byte[] {96, 52, 115, 0, -46, -117, 73, 50, 56, 46, -98, -81, 100, 55, 71, -85, -118, 54, 62, 94, 82, -1, 110, 30, 0, 104, -84, 67, -90, -55, -108, -21, 61, -77, -48, 62, 34, 107, -10, -38, 64, -119, -79, -63, 26, -14, 110, -16, -57, -40, -68, -97, -83, 89, -81, -112, 33, 31, -34, -8, -107, 42, -84, 108, 0};

    Assertions.assertEquals(address, "0x082977959d0C5A1bA627720ac753Ec2ADB5Bd7d0".toLowerCase());
    Assertions.assertArrayEquals(wallet.sign("1e194c68360307cfb715bf17878791ad1ced8da7d2e5f42b691074c577f41eac",
                                             address, SignatureType.eth_sign, PayloadType.PL_SIGN_ETHTX, null),
                                 expectedSignature);
  }
}
