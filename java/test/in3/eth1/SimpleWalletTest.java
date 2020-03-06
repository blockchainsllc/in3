package in3.eth1;

import in3.eth1.SimpleWallet;
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
    String pk      = "0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6";
    String address = wallet.addRawKey(pk);

    Assertions.assertEquals(address, "0x082977959d0C5A1bA627720ac753Ec2ADB5Bd7d0".toLowerCase());
    Assertions.assertEquals(
        wallet.sign("1e194c68360307cfb715bf17878791ad1ced8da7d2e5f42b691074c577f41eac", address),
        "0xf16dcaa830a3f710e28444df7df85fa927d8a66f789196fc2a3b934c829dbcaa5329be0711daba3b0c85ab23f1adb32c4e88fd8cb42b951d3be40af1bbd92e7400");
  }

  @Test
  public void canSign() {
    String pk      = "0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6";
    String address = wallet.addRawKey(pk);

    String anotherAddress = "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d";

    Assertions.assertTrue(wallet.canSign(address));
    Assertions.assertFalse(wallet.canSign(anotherAddress));
  }
}
