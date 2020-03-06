package in3;

import org.junit.jupiter.api.*;

public class IN3Test {

    private static IN3 in3;

    @BeforeAll
    public static void setBuilder() {
        String[][] mockedResponses = { { "eth_call", "eth_call_2.json" } };
        IN3MockBuilder builder = new IN3MockBuilder();
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
        String address = "0xbc0ea09c1651a3d5d40bacb4356fb59159a99564";
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
}
