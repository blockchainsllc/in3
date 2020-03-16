package in3.utils;

import in3.Chain;
import in3.IN3;
import in3.IN3MockBuilder;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

class CryptoTest {

  private static IN3 in3;

  @BeforeAll
  public static void setBuilder() {
    String[][] mockedResponses = {};
    IN3MockBuilder builder     = new IN3MockBuilder(Chain.MAINNET);
    in3                        = builder.constructClient(mockedResponses);
  }

  @Test
  public void signData() {
    String    msg       = "0x0102030405060708090a0b0c0d0e0f";
    String    key       = "0xa8b8759ec8b59d7c13ef3630e8530f47ddb47eba12f00f9024d3d48247b62852";
    Signature signature = in3.getCrypto().signData(msg, key, SignatureType.raw);

    Assertions.assertEquals(signature.getMessage(), "0x0102030405060708090a0b0c0d0e0f");
    Assertions.assertEquals(signature.getMessageHash(), "0x1d4f6fccf1e27711667605e29b6f15adfda262e5aedfc5db904feea2baa75e67");
    Assertions.assertEquals(signature.getSignature(), "0xa5dea9537d27e4e20b6dfc89fa4b3bc4babe9a2375d64fb32a2eab04559e95792264ad1fb83be70c145aec69045da7986b95ee957fb9c5b6d315daa5c0c3e1521b");
    Assertions.assertEquals(signature.getR(), "0xa5dea9537d27e4e20b6dfc89fa4b3bc4babe9a2375d64fb32a2eab04559e9579");
    Assertions.assertEquals(signature.getS(), "0x2264ad1fb83be70c145aec69045da7986b95ee957fb9c5b6d315daa5c0c3e152");
    Assertions.assertEquals(signature.getV(), 27);
  }

  @Test
  public void decryptKey() {
    String key        = "{\"version\": 3,\"id\": \"f6b5c0b1-ba7a-4b67-9086-a01ea54ec638\",\"address\": \"08aa30739030f362a8dd597fd3fcde283e36f4a1\",\"crypto\": {\"ciphertext\": \"d5c5aafdee81d25bb5ac4048c8c6954dd50c595ee918f120f5a2066951ef992d\",\"cipherparams\": {\"iv\": \"415440d2b1d6811d5c8a3f4c92c73f49\"},\"cipher\": \"aes-128-ctr\",\"kdf\": \"pbkdf2\",\"kdfparams\": {\"dklen\": 32,\"salt\": \"691e9ad0da2b44404f65e0a60cf6aabe3e92d2c23b7410fd187eeeb2c1de4a0d\",\"c\": 16384,\"prf\": \"hmac-sha256\"},\"mac\": \"de651c04fc67fd552002b4235fa23ab2178d3a500caa7070b554168e73359610\"}}";
    String passphrase = "test";
    String result     = in3.getCrypto().decryptKey(key, passphrase);
    Assertions.assertEquals("0x1ff25594a5e12c1e31ebd8112bdf107d217c1393da8dc7fc9d57696263457546", result);
  }

  @Test
  public void pk2address() {
    String result = in3.getCrypto().pk2address("0x0fd65f7da55d811634495754f27ab318a3309e8b4b8a978a50c20a661117435a");
    Assertions.assertEquals("0xdc5c4280d8a286f0f9c8f7f55a5a0c67125efcfd", result);
  }

  @Test
  public void pk2public() {
    String result = in3.getCrypto().pk2public("0x0fd65f7da55d811634495754f27ab318a3309e8b4b8a978a50c20a661117435a");
    Assertions.assertEquals("0x0903329708d9380aca47b02f3955800179e18bffbb29be3a644593c5f87e4c7fa960983f78186577eccc909cec71cb5763acd92ef4c74e5fa3c43f3a172c6de1", result);
  }

  @Test
  public void ecrecover() {
    String  msg       = "0x487b2cbb7997e45b4e9771d14c336b47c87dc2424b11590e32b3a8b9ab327999";
    String  signature = "0x0f804ff891e97e8a1c35a2ebafc5e7f129a630a70787fb86ad5aec0758d98c7b454dee5564310d497ddfe814839c8babd3a727692be40330b5b41e7693a445b71c";
    Account result    = in3.getCrypto().ecrecover(msg, signature, SignatureType.hash);

    Assertions.assertEquals("0x94b26bafa6406d7b636fbb4de4edd62a2654eeecda9505e9a478a66c4f42e504c4481bad171e5ba6f15a5f11c26acfc620f802c6768b603dbcbe5151355bbffb", result.getPublicKey());
    Assertions.assertEquals("0xf68a4703314e9a9cf65be688bd6d9b3b34594ab4", result.getAddress());
  }
}