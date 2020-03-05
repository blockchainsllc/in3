package in3.eth1;

import in3.*;
import java.math.*;
import org.junit.jupiter.api.*;

public class APITest {
  private static IN3MockBuilder builder;

  @BeforeAll
  public static void setBuilder() {
    builder = new IN3MockBuilder(Chain.MAINNET);
  }

  @Test
  public void call() {
    String[][] mockedResponses = {{"eth_call", "eth_call_1.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    String to       = "0x2736D225f85740f42D17987100dc8d58e9e16252";
    String function = "servers(uint256):(string,address,uint32,uint256,uint256,address)";
    Object[] params = new Object[] {1};

    TransactionRequest request = new TransactionRequest();
    request.setTo(to);
    request.setFunction(function);
    request.setParams(params);

    Object[] res1 = (Object[]) in3.getEth1API().call(request, Block.LATEST);

    Assertions.assertEquals(6, res1.length);
    Assertions.assertEquals("https://in3.slock.it/mainnet/nd-4", res1[0]);
    Assertions.assertEquals("0xbc0ea09c1651a3d5d40bacb4356fb59159a99564", res1[1]);
    Assertions.assertEquals("0xffff", res1[2]);
    Assertions.assertEquals("0xffff", res1[3]);
  }

  @Test
  public void setTransaction() {
    String[][] mockedResponses = {{"eth_gasPrice", "eth_gasPrice.json"},
                                  {"eth_estimateGas", "eth_estimateGas.json"},
                                  {"eth_getTransactionCount", "eth_getTransactionCount.json"},
                                  {"eth_sendRawTransaction", "eth_sendRawTransaction.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    String       pk     = "0829B3C639A3A8F2226C8057F100128D4F7AE8102C92048BA6DE38CF4D3BC6F1";
    SimpleWallet wallet = new SimpleWallet();
    String       from   = wallet.addRawKey(pk);
    in3.setSigner(wallet);

    TransactionRequest request = new TransactionRequest();
    request.setFrom(from);
    request.setTo("0x3940256B93c4BE0B1d5931A6A036608c25706B0c");
    request.setGas(21000);
    request.setValue(new BigInteger("100000000"));

    String hash = in3.getEth1API().sendTransaction(request);

    // expect multiple calls here too

    Assertions.assertEquals("0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38", hash);
  }

  @Test
  public void blockNumber() {
    String[][] mockedResponses = {{"eth_blockNumber", "eth_blockNumber.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    long response = in3.getEth1API().getBlockNumber();
    Assertions.assertEquals(3220, response);
  }

  @Test
  public void getLogs() {
    String[][] mockedResponses = {{"eth_getLogs", "eth_getLogs.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    LogFilter filter = new LogFilter();
    filter.setFromBlock(new BigInteger("834B77", 16).longValue());
    filter.setToBlock(new BigInteger("834B77", 16).longValue());
    filter.setAddress("0xdac17f958d2ee523a2206206994597c13d831ec7");
    Log[] response = in3.getEth1API().getLogs(filter);

    Assertions.assertEquals("0x20be6d27ed6a4c99c5dbeeb9081e114a9b400c52b80c4d10096c94ad7d3c1af6",
                            response[0].getTransactionHash());
  }

  @Test
  public void getTransactionReceipt() {
    String[][] mockedResponses = {{"eth_getTransactionReceipt", "eth_getTransactionReceipt.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    TransactionReceipt receipt = in3.getEth1API()
                                     .getTransactionReceipt("0x6188bf0672c005e30ad7c2542f2f048521662e30c91539d976408adf379bdae2");
    Assertions.assertEquals("0x5b8174e20996ec743f01d3b55a35dd376429c596", receipt.getTo());
    Assertions.assertTrue(receipt.getStatus());
    Assertions.assertEquals("0x5b8174e20996ec743f01d3b55a35dd376429c596", receipt.getLogs()[0].getAddress());
  }

  @Test
  public void getTransactionByHash() {
    String[][] mockedResponses = {{"eth_getTransactionByHash", "eth_getTransactionByHash.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    Transaction tx = in3.getEth1API()
                         .getTransactionByHash("0x6188bf0672c005e30ad7c2542f2f048521662e30c91539d976408adf379bdae2");
    Assertions.assertEquals("0x8220e66456e40636bff3a440832c9f179e4811d4e28269c7ab70142c3e5f9be2",
                            tx.getBlockHash());
    Assertions.assertEquals("0x3a9e354dee60df25c0389badafec8457e36ebfd2", tx.getFrom());
  }

  @Test
  public void getBalance() {
    String[][] mockedResponses = {{"eth_getBalance", "eth_getBalance.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    BigInteger balance = in3.getEth1API().getBalance("0x4144FFD5430a8518fa2d84ef5606Fd7e1921cE27", Block.LATEST);
    Assertions.assertEquals(new BigInteger("3646260000000000000"), balance);
  }

  @Test
  public void getCode() {
    String[][] mockedResponses = {{"eth_getCode", "eth_getCode.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    String response = in3.getEth1API().getCode("0xdAC17F958D2ee523a2206206994597C13D831ec7", Block.LATEST);
    Assertions.assertNotNull(response);
    // Assertions.assertEquals(contractCode.codetest.result, response);
  }

  @Test
  public void getStorageAt() {
    String[][] mockedResponses = {{"eth_getStorageAt", "eth_getStorageAt.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    String storage = in3.getEth1API().getStorageAt("0x862174623bc39e57de552538f424806b947d3d05",
                                                   new BigInteger("0"), Block.LATEST);
    Assertions.assertEquals("0x0000000000000000000000000000000000000000000000000000000000000000", storage);
  }

  @Test
  public void getUncleCountByBlockNumber() {
    String[][] mockedResponses = {{"eth_getUncleCountByBlockNumber", "eth_getUncleCountByBlockNumber.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    long count = in3.getEth1API().getUncleCountByBlockNumber(9280006);
    Assertions.assertEquals(2, count);
  }

  @Test
  public void getChainId() {
    // This might be worth testing since there are some integration issues that might arise from the java <-> c interop even if, at the end, this is just a fancy getter.
    String[][] mockedResponses = {};
    IN3 in3                    = builder.constructClient(mockedResponses);
    Assertions.assertEquals("0x1", in3.getEth1API().getChainId());
  }

  @Test
  public void getBlockByNumber() {
    String[][] mockedResponses = {{"eth_getBlockByNumber", "eth_getBlockByNumber.json"}};

    IN3   in3    = builder.constructClient(mockedResponses);
    Block latest = in3.getEth1API().getBlockByNumber(Block.EARLIEST, true);

    Assertions.assertEquals(0, latest.getNumber());
    Assertions.assertEquals(540, latest.getSize());
  }

  @Test
  public void getBlockByHash() {
    String[][] mockedResponses = {{"eth_getBlockByHash", "eth_getBlockByHash.json"}};

    IN3   in3    = builder.constructClient(mockedResponses);
    Block latest = in3.getEth1API()
                       .getBlockByHash("0xd4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3", true);
    Assertions.assertEquals(0, latest.getNumber());
    Assertions.assertEquals(540, latest.getSize());
  }

  @Test
  public void getGasPrice() {
    String[][] mockedResponses = {{"eth_gasPrice", "eth_gasPrice.json"}};

    IN3  in3             = builder.constructClient(mockedResponses);
    long currentGasPrice = in3.getEth1API().getGasPrice();
    Assertions.assertEquals(2100000000, currentGasPrice);
  }

  @Test
  public void estimateGas() {
    String[][] mockedResponses = {{"eth_estimateGas", "eth_estimateGas.json"}};

    IN3          in3    = builder.constructClient(mockedResponses);
    String       pk     = "0829B3C639A3A8F2226C8057F100128D4F7AE8102C92048BA6DE38CF4D3BC6F1";
    SimpleWallet wallet = new SimpleWallet();
    String       from   = wallet.addRawKey(pk);
    in3.setSigner(wallet);

    TransactionRequest tx = new TransactionRequest();
    ;
    tx.setGasPrice(1);
    tx.setFrom(from);
    tx.setTo("0xEA674fdDe714fd979de3EdF0F56AA9716B898ec8");

    long gasEstimate = in3.getEth1API().estimateGas(tx, Block.LATEST);
    Assertions.assertEquals(21000, gasEstimate);
  }

  @Test
  public void getBlockTransactionCountByHash() {
    String[][] mockedResponses = {
        {"eth_getBlockTransactionCountByHash", "eth_getBlockTransactionCountByHash.json"}};

    IN3  in3              = builder.constructClient(mockedResponses);
    long transactionCount = in3.getEth1API()
                                .getBlockTransactionCountByHash("0xd4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3");
    Assertions.assertEquals(0, transactionCount);
  }

  @Test
  public void getBlockTransactionCountByNumber() {
    String[][] mockedResponses = {
        {"eth_getBlockTransactionCountByNumber", "eth_getBlockTransactionCountByNumber.json"}};

    IN3 in3 = builder.constructClient(mockedResponses);

    long transactionCount = in3.getEth1API().getBlockTransactionCountByNumber(9298869);
    Assertions.assertEquals(121, transactionCount);
  }

  @Test
  public void getTransactionByBlockHashAndIndex() {
    String[][] mockedResponses = {
        {"eth_getTransactionByBlockHashAndIndex", "eth_getTransactionByBlockHashAndIndex.json"}};

    IN3         in3 = builder.constructClient(mockedResponses);
    Transaction tx  = in3.getEth1API().getTransactionByBlockHashAndIndex(
        "0xd03f4a0ce830ce568be08aa37bc0a72374e92da5b388e839b35f24a144a5085d", 1);

    Assertions.assertEquals(tx.getValue(), new BigInteger("48958690000000000"));
  }

  @Test
  public void getTransactionByBlockNumberAndIndex() {
    String[][] mockedResponses = {
        {"eth_getTransactionByBlockNumberAndIndex", "eth_getTransactionByBlockNumberAndIndex.json"}};

    IN3         in3 = builder.constructClient(mockedResponses);
    Transaction tx  = in3.getEth1API().getTransactionByBlockNumberAndIndex(9319093, 1);

    Assertions.assertEquals(tx.getValue(), new BigInteger("48958690000000000"));
    Assertions.assertEquals(tx.getFrom(), "0xe3649077ce21a48caf34041e983b92e332e80fd9");
  }

  @Test
  public void getTransactionCount() {
    String[][] mockedResponses = {{"eth_getTransactionCount", "eth_getTransactionCount.json"}};

    String from = "0x7fc7032e731f5bcbd4843406945acaf917087fde";
    IN3    in3  = builder.constructClient(mockedResponses);

    BigInteger transactionCount = in3.getEth1API().getTransactionCount(from, Block.LATEST);
    Assertions.assertEquals(transactionCount, BigInteger.valueOf(19));
  }

  @Test
  public void getUncleByBlockNumberAndIndex() {
    String[][] mockedResponses = {
        {"eth_getUncleByBlockNumberAndIndex", "eth_getUncleByBlockNumberAndIndex.json"}};

    IN3   in3   = builder.constructClient(mockedResponses);
    Block uncle = in3.getEth1API().getUncleByBlockNumberAndIndex(9317999, 0);

    Assertions.assertEquals(9317998, uncle.getNumber());
    Assertions.assertEquals(37088, uncle.getSize());
  }

  @Test
  public void getUncleCountByBlockHash() {
    String[][] mockedResponses = {{"eth_getUncleCountByBlockHash", "eth_getUncleCountByBlockHash.json"}};

    IN3  in3        = builder.constructClient(mockedResponses);
    long uncleCount = in3.getEth1API()
                          .getUncleCountByBlockHash("0x211389bcc4b2ccc1f19d7f8470b0ad0778f7027e4cdf45b1c64c78895b0bee19");

    Assertions.assertEquals(1, uncleCount);
  }

  @Test
  public void sendRawTransaction() {
    String[][] mockedResponses = {{"eth_sendRawTransaction", "eth_sendRawTransaction_1.json"}};

    IN3    in3            = builder.constructClient(mockedResponses);
    String rawTransaction = "0xf8671b8477359400825208943940256b93c4be0b1d5931a6a036608c25706b0c8405f5e100802da0278d2c010a59688fc12a55563d81239b1dc7e3d9c6a535b34600329b0c640ad8a03894daf8d7c25b56caec71b695c5f6b1b6fd903ecfa441b2c4e15fd1c72c54a9";
    String hash           = in3.getEth1API().sendRawTransaction(rawTransaction);

    // expect multiple calls here too
    Assertions.assertEquals("0xd55a8b0cf4896ffbbb10b125bf20d89c8006f42cc327a9859c59ac54e439b388", hash);
  }

  @Test
  public void newBlockFilter() {
    String[][] mockedResponses = {{"eth_blockNumber", "eth_blockNumber_1.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    long filterId = in3.getEth1API().newBlockFilter();
    Assertions.assertTrue(filterId > 0);

    // Given the same mock response, this should increment the filter_id
    long filterId2 = in3.getEth1API().newBlockFilter();
    Assertions.assertEquals(filterId + 1, filterId2);
  }

  @Test
  public void newLogFilter() {
    String[][] mockedResponses = {{"eth_blockNumber", "eth_blockNumber_1.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    LogFilter filter = new LogFilter();
    filter.setAddress("0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455");

    long filterId = in3.getEth1API().newLogFilter(filter);
    Assertions.assertTrue(filterId > 0);

    // Given the same mock response, this should increment the filter_id
    long filterId2 = in3.getEth1API().newLogFilter(filter);
    Assertions.assertEquals(filterId + 1, filterId2);
  }

  @Test
  public void uninstallFilter() {
    String[][] mockedResponses = {{"eth_blockNumber", "eth_blockNumber_1.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    LogFilter filter = new LogFilter();
    filter.setAddress("0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455");
    long filterId = in3.getEth1API().newLogFilter(filter);

    boolean success1 = in3.getEth1API().uninstallFilter(filterId);
    Assertions.assertTrue(success1);
    boolean success2 = in3.getEth1API().uninstallFilter(filterId);
    Assertions.assertTrue(!success2);
  }

  @Test
  public void getFilterLogs() {
    String[][] mockedResponses = {{"eth_blockNumber", "eth_blockNumber_2.json"},
                                  {"eth_getLogs", "eth_getLogs_1.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    LogFilter filter = new LogFilter();
    filter.setAddress("0x1c81079b0752881f3231318a1355e21de26bbeb5");
    filter.setFromBlock(2050343);

    long filterId = in3.getEth1API().newLogFilter(filter);

    Log[] logs1 = in3.getEth1API().getFilterLogs(filterId);
    Assertions.assertTrue(logs1.length > 0);
    Log[] logs2 = in3.getEth1API().getFilterLogs(filterId);
    Assertions.assertTrue(logs2.length == 0);
  }

  @Test
  public void getFilterChangesFromLogs() {
    String[][] mockedResponses = {{"eth_blockNumber", "eth_blockNumber_2.json"},
                                  {"eth_getLogs", "eth_getLogs_1.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    LogFilter filter = new LogFilter();
    filter.setAddress("0x1c81079b0752881f3231318a1355e21de26bbeb5");
    filter.setFromBlock(2050343);

    long filterId = in3.getEth1API().newLogFilter(filter);

    Log[] logs1 = in3.getEth1API().getFilterLogs(filterId);
    Assertions.assertTrue(logs1.length > 0);
    Log[] logs2 = in3.getEth1API().getFilterLogs(filterId);
    Assertions.assertTrue(logs2.length == 0);
  }

  @Test
  public void getFilterChangesFromBlocks() {
    String[][] mockedResponses1 = {{"eth_blockNumber", "eth_blockNumber_3.json"}};

    String[][] mockedResponses2 = {{"eth_blockNumber", "eth_blockNumber_4.json"},
                                   {"eth_getBlockByNumber", "eth_getBlockByNumber_1.json"}};

    IN3  in3      = builder.constructClient(mockedResponses1);
    long filterId = in3.getEth1API().newBlockFilter();

    // This actually changes the internal state of mock transport with the new requests (not the best since relies on a side-effect).
    builder.buildTransport(mockedResponses2);

    String[] hashList = in3.getEth1API().getFilterChangesFromBlocks(filterId);
    Assertions.assertArrayEquals(
        new String[] {"0x03b1815a066ba71eab8e6622afa3e596b80580c2b1056990199dd974db66337e"}, hashList);
  }

  @Test
  public void abiEncode() {
    IN3    in3       = builder.constructClient(new String[][] {});
    String signature = "getBalance(address)";
    String[] params  = new String[] {"0x1234567890123456789012345678901234567890"};
    String expected  = "0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890";
    String result    = in3.getEth1API().abiEncode(signature, params);
    Assertions.assertEquals(expected, result);
  }

  @Test
  public void abiDecode() {
    IN3    in3              = builder.constructClient(new String[][] {});
    String signature        = "(address,uint256)";
    String encoded          = "0x00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000005";
    String[] expectedDecode = new String[] {"0x1234567890123456789012345678901234567890", "0x5"};
    String[] result         = in3.getEth1API().abiDecode(signature, encoded);
    Assertions.assertArrayEquals(expectedDecode, result);
  }

  @Test
  public void checkSumAddress() {
    IN3    in3     = builder.constructClient(new String[][] {});
    String address = "0xBc0ea09C1651A3D5D40Bacb4356FB59159A99564";
    String result  = in3.getEth1API().checksumAddress("0xbc0ea09c1651a3d5d40bacb4356fb59159a99564");
    Assertions.assertEquals(address, result);
  }

  @Test
  public void ens() {
    IN3    in3    = builder.constructClient(new String[][] {
        {"eth_call", "eth_call_2.json"}});
    String result = in3.getEth1API().ens("cryptokitties.eth");
    Assertions.assertEquals("0x06012c8cf97bead5deae237070f9587f8e7a266d", result);
  }
}
