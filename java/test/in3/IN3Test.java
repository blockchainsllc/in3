package in3;

import org.junit.jupiter.api.*;

public class IN3Test {
  @Test
  public void nodeList() {
    String[][] mockedResponses = {
        {"eth_call", "eth_call_2.json"},
        {"in3_nodeList", "in3_nodeList.json"},
        {"in3_sign", "in3_sign.json"}};
    IN3MockBuilder builder = new IN3MockBuilder(Chain.GOERLI);
    IN3            in3     = builder.constructClient(mockedResponses);

    String url          = "https://in3-v2.slock.it/goerli/nd-1";
    String address      = "0x45d45e6ff99e6c34a235d263965910298985fcfe";
    int    index        = 0;
    String deposit      = "0x2386f26fc10000";
    long   props        = 0x1ddL;
    int    timeout      = 3456000;
    int    registerTime = 1576227711;
    int    weight       = 2000;

    IN3Node[] list = in3.nodeList(new String[] {
        "0x45d45e6ff99e6c34a235d263965910298985fcfe"});

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
    String[][] mockedResponses = {
        {"eth_call", "eth_call_2.json"},
        {"in3_nodeList", "in3_nodeList.json"},
        {"in3_sign", "in3_sign.json"}};
    IN3MockBuilder builder = new IN3MockBuilder(Chain.GOERLI);
    IN3            in3     = builder.constructClient(mockedResponses);

    // This test, for now is a lie.
    boolean result = in3.cacheClear();
    Assertions.assertTrue(result);
  }

  @Test
  public void sign() {
    String[][] mockedResponses = {
        {"eth_call", "eth_call_2.json"},
        {"in3_nodeList", "in3_nodeList.json"},
        {"in3_sign", "in3_sign.json"}};
    IN3MockBuilder builder = new IN3MockBuilder(Chain.GOERLI);
    IN3            in3     = builder.constructClient(mockedResponses);

    BlockID[] vR = new BlockID[] {
        BlockID.fromHash("0x2a8bf38abe3fec478a2029e74ac95ecdbef95ff2fb832786ba4c5231c8cea480")};
    String[] address         = new String[] {"0x1fe2e9bf29aa1938859af64c413361227d04059a"};
    SignedBlockHash[] result = in3.sign(vR, address);
    Assertions.assertEquals("0x2a8bf38abe3fec478a2029e74ac95ecdbef95ff2fb832786ba4c5231c8cea480", result[0].getBlockHash());
    Assertions.assertEquals(9551793, result[0].getBlock());
    Assertions.assertEquals("0x16cf7b94a7276532ceff4bf6fd50fd01f2c2962a5f0887c70c75943ce9e08b77", result[0].getR());
    Assertions.assertEquals("0x01f51c90e7f3493acf3c5b4efbed63534711f6209b2c47c34074f4882b92feaa", result[0].getS());
    Assertions.assertEquals(27, result[0].getV());
    Assertions.assertEquals("0x0284928ef6fd270bacbfdba3087d0c50ad2db74af54c8bda8209b8ac9e41cfe1", result[0].getMsgHash());
  }

  @Test
  public void getConfig() {
    String[][] mockedResponses = {
        {"eth_call", "eth_call_2.json"},
        {"in3_nodeList", "in3_nodeList.json"},
        {"in3_sign", "in3_sign.json"}};
    IN3MockBuilder builder = new IN3MockBuilder(Chain.GOERLI);
    IN3            in3     = builder.constructClient(mockedResponses);

    // This test may seem pointless but it intends to test the proper conversion of the jni getConfig to a valid String.
    String configJson = in3.getConfig().toJSON();
    Assertions.assertNotNull(configJson);
  }

  @Test
  public void handleEns() {
    String[][] mockedResponses = {
        {"eth_call", "eth_call_2.json"},
        {"in3_nodeList", "in3_nodeList.json"},
        {"in3_sign", "in3_sign.json"}};
    IN3MockBuilder builder = new IN3MockBuilder(Chain.MAINNET);
    IN3            in3     = builder.constructClient(mockedResponses);

    String[] params = new String[] {
        "cryptokitties.eth",
        "0x0102030405060708090a0b0c0d0e0f",
        "0xa8b8759ec8b59d7c13ef3630e8530f47ddb47eba12f00f9024d3d48247b62852"};

    String[] expected = new String[] {
        "0x06012c8cf97bead5deae237070f9587f8e7a266d",
        "0x0102030405060708090a0b0c0d0e0f",
        "0xa8b8759ec8b59d7c13ef3630e8530f47ddb47eba12f00f9024d3d48247b62852"};

    String[] transformed = (String[]) in3.handleEns(params);
    Assertions.assertArrayEquals(expected, transformed);
  }

  @Test
  public void free() {
    String[][] mockedResponses = {
        {"eth_call", "eth_call_2.json"},
        {"in3_nodeList", "in3_nodeList.json"},
        {"in3_sign", "in3_sign.json"}};
    IN3MockBuilder builder = new IN3MockBuilder(Chain.GOERLI);
    IN3            in3     = builder.constructClient(mockedResponses);

    in3.free();
  }
}
