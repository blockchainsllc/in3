package in3;

import java.math.BigInteger;

import org.junit.jupiter.api.*;

public class IN3Test {

    private static IN3 in3;

    @BeforeAll
    public static void setBuilder() {
        String[][] mockedResponses = {
                { "eth_call", "eth_call_2.json" },
                { "in3_nodeList", "in3_nodeList.json" },
                { "in3_sign", "in3_sign.json" }
        };
        IN3MockBuilder builder = new IN3MockBuilder(Chain.MAINNET);
        in3 = builder.constructClient(mockedResponses);
    }

    @Test
    public void abiEncode() {
        String signature = "getBalance(address)";
        String[] params = new String[] { "0x1234567890123456789012345678901234567890" };
        String expected = "0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890";
        String result = in3.abiEncode(signature, params);
        Assertions.assertEquals(expected, result);
    }

    @Test
    public void abiDecode() {
        String signature = "(address,uint256)";
        String encoded = "0x00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000005";
        String[] expectedDecode = new String[] { "0x1234567890123456789012345678901234567890", "0x5" }; 
        String[] result = in3.abiDecode(signature, encoded);
        Assertions.assertArrayEquals(expectedDecode, result);
    }

    @Test
    public void checkSum() {
        String address = "0xBc0ea09C1651A3D5D40Bacb4356FB59159A99564";
        String result = in3.checksumAddress("0xbc0ea09c1651a3d5d40bacb4356fb59159a99564");
        Assertions.assertEquals(address, result);
    }

    @Test
    public void ens() {
        String result = in3.ens("cryptokitties.eth");
        Assertions.assertEquals("0x06012c8cf97bead5deae237070f9587f8e7a266d", result);
    }

    @Test
    public void pk2address() {
        String result = in3.pk2address("0x0fd65f7da55d811634495754f27ab318a3309e8b4b8a978a50c20a661117435a");
        Assertions.assertEquals("0xdc5c4280d8a286f0f9c8f7f55a5a0c67125efcfd", result);
    }

    @Test
    public void pk2public() {
        String result = in3.pk2public("0x0fd65f7da55d811634495754f27ab318a3309e8b4b8a978a50c20a661117435a");
        Assertions.assertEquals("0x0903329708d9380aca47b02f3955800179e18bffbb29be3a644593c5f87e4c7fa960983f78186577eccc909cec71cb5763acd92ef4c74e5fa3c43f3a172c6de1", result);
    }

    @Test
    public void ecrecover() {
        String msg = "0x487b2cbb7997e45b4e9771d14c336b47c87dc2424b11590e32b3a8b9ab327999";
        String signature = "0x0f804ff891e97e8a1c35a2ebafc5e7f129a630a70787fb86ad5aec0758d98c7b454dee5564310d497ddfe814839c8babd3a727692be40330b5b41e7693a445b71c";
        EcRecoverResult result = in3.ecrecover(msg, signature, SignatureType.hash);

        Assertions.assertEquals("0x94b26bafa6406d7b636fbb4de4edd62a2654eeecda9505e9a478a66c4f42e504c4481bad171e5ba6f15a5f11c26acfc620f802c6768b603dbcbe5151355bbffb", result.getPublicKey());
        Assertions.assertEquals("0xf68a4703314e9a9cf65be688bd6d9b3b34594ab4", result.getAddress());
    }

    @Test
    public void nodeList() {
        String url = "https://in3-v2.slock.it/mainnet/nd-1";
        String address = "0x45d45e6ff99e6c34a235d263965910298985fcfe";
        int index = 0;
        String deposit = "0x2386f26fc10000";
        long props = 0x06000001ddL;
        int timeout = 3456000;
        int registerTime = 1576224418;
        int weight = 2000;

        Node[] list = in3.nodeList();

        Assertions.assertTrue(list.length > 0);
        Assertions.assertEquals(url, list[0].getUrl());
        Assertions.assertEquals(address, list[0].getAddress());
        Assertions.assertEquals(index, list[0].getIndex());
        Assertions.assertEquals(deposit, list[0].getDeposit());
        Assertions.assertEquals(props, list[0].getProps());
        Assertions.assertEquals(timeout, list[0].getTimeout());
        Assertions.assertEquals(registerTime, list[0].getRegisterTime());
        Assertions.assertEquals(weight, list[0].getWeight());
    }

    @Test
    public void cacheClear() {
        // This test, for now is a lie.
        boolean result = in3.cacheClear();
        Assertions.assertTrue(result);
    }

    @Test
    public void sign() {
        VerifiedHash[] vR = new VerifiedHash[] {
                new VerifiedHash(9551793, "0x2a8bf38abe3fec478a2029e74ac95ecdbef95ff2fb832786ba4c5231c8cea480")
        };
        String[] address = new String[] { "0x1fe2e9bf29aa1938859af64c413361227d04059a" };
        SignedBlockHash[] result = in3.sign(vR, address);
        Assertions.assertEquals("0x2a8bf38abe3fec478a2029e74ac95ecdbef95ff2fb832786ba4c5231c8cea480", result[0].getBlockHash());
        Assertions.assertEquals(9551793, result[0].getBlock());
        Assertions.assertEquals("0x16cf7b94a7276532ceff4bf6fd50fd01f2c2962a5f0887c70c75943ce9e08b77", result[0].getR());
        Assertions.assertEquals("0x01f51c90e7f3493acf3c5b4efbed63534711f6209b2c47c34074f4882b92feaa", result[0].getS());
        Assertions.assertEquals(27, result[0].getV());
        Assertions.assertEquals("0x0284928ef6fd270bacbfdba3087d0c50ad2db74af54c8bda8209b8ac9e41cfe1", result[0].getMsgHash());
    }

    @Test
    public void signData() {
        String msg = "0x0102030405060708090a0b0c0d0e0f";
        String key = "0xa8b8759ec8b59d7c13ef3630e8530f47ddb47eba12f00f9024d3d48247b62852";
        Signature signature = in3.signData(msg, key, SignatureType.raw);

        Assertions.assertEquals(signature.getMessage(), "0x0102030405060708090a0b0c0d0e0f");
        Assertions.assertEquals(signature.getMessageHash(), "0x1d4f6fccf1e27711667605e29b6f15adfda262e5aedfc5db904feea2baa75e67");
        Assertions.assertEquals(signature.getSignature(), "0xa5dea9537d27e4e20b6dfc89fa4b3bc4babe9a2375d64fb32a2eab04559e95792264ad1fb83be70c145aec69045da7986b95ee957fb9c5b6d315daa5c0c3e1521b");
        Assertions.assertEquals(signature.getR(), "0xa5dea9537d27e4e20b6dfc89fa4b3bc4babe9a2375d64fb32a2eab04559e9579");
        Assertions.assertEquals(signature.getS(), "0x2264ad1fb83be70c145aec69045da7986b95ee957fb9c5b6d315daa5c0c3e152");
        Assertions.assertEquals(signature.getV(), 27);
    }

    @Test
    public void decryptKey() {
        String key = "{\"version\": 3,\"id\": \"f6b5c0b1-ba7a-4b67-9086-a01ea54ec638\",\"address\": \"08aa30739030f362a8dd597fd3fcde283e36f4a1\",\"crypto\": {\"ciphertext\": \"d5c5aafdee81d25bb5ac4048c8c6954dd50c595ee918f120f5a2066951ef992d\",\"cipherparams\": {\"iv\": \"415440d2b1d6811d5c8a3f4c92c73f49\"},\"cipher\": \"aes-128-ctr\",\"kdf\": \"pbkdf2\",\"kdfparams\": {\"dklen\": 32,\"salt\": \"691e9ad0da2b44404f65e0a60cf6aabe3e92d2c23b7410fd187eeeb2c1de4a0d\",\"c\": 16384,\"prf\": \"hmac-sha256\"},\"mac\": \"de651c04fc67fd552002b4235fa23ab2178d3a500caa7070b554168e73359610\"}}";
        String passphrase = "test";
        String result = in3.decryptKey(key, passphrase);
        Assertions.assertEquals("0x1ff25594a5e12c1e31ebd8112bdf107d217c1393da8dc7fc9d57696263457546", result);
    }

    @Test
    public void getConfig() {
        // This test may seem pointless but it intends to test the proper conversion of the jni getConfig to a valid String.
        String configJson = in3.getConfig().toJSON();
        Assertions.assertNotNull(configJson);
    }

    @Test
    public void handleEns() {
        String [] params = new String[] {
                "cryptokitties.eth",
                "0x0102030405060708090a0b0c0d0e0f",
                "0xa8b8759ec8b59d7c13ef3630e8530f47ddb47eba12f00f9024d3d48247b62852"
        };

        String [] expected = new String[] {
                "0x06012c8cf97bead5deae237070f9587f8e7a266d",
                "0x0102030405060708090a0b0c0d0e0f",
                "0xa8b8759ec8b59d7c13ef3630e8530f47ddb47eba12f00f9024d3d48247b62852"
        };

        String[] transformed = (String[]) in3.handleEns(params);
        Assertions.assertArrayEquals(expected, transformed);
    }
}
