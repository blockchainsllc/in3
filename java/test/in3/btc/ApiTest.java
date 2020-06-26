package in3.btc;

import in3.Chain;
import in3.IN3;
import in3.IN3MockBuilder;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

public class ApiTest {
  private IN3MockBuilder builder;

  @BeforeEach
  public void setupClient() {
    builder = new IN3MockBuilder(Chain.BTC);
  }

  @Test
  public void getBlockHeader() {
    String[][] mockedResponses = {new String[] {"getblockheader", "btc_getBlockHeader_1.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    BlockHeader result = in3.getBtcAPI()
                             .getBlockHeader("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60");

    Assertions.assertEquals("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60", result.getHash());
    Assertions.assertEquals(13, result.getConfirmations());
    Assertions.assertEquals(634026, result.getHeight());
    Assertions.assertEquals(1073676288, result.getVersion());
    Assertions.assertEquals("3fff0000", result.getVersionHex());
    Assertions.assertEquals("0b27b05fb5c9c53f595cf12696daa3ea4fb03ea67e0ffd909a1f2bb6544b63c1",
                            result.getMerkleroot());
    Assertions.assertEquals(1591782412, result.getTime());
    Assertions.assertEquals(1591778995, result.getMediantime());
    Assertions.assertEquals(783981944, result.getNonce());
    Assertions.assertEquals("17147f35", result.getBits());
    Assertions.assertEquals(13732352106018.34f, result.getDifficulty());
    Assertions.assertEquals("00000000000000000000000000000000000000001038fd1e673c4ff045dbd6c1",
                            result.getChainwork());
    Assertions.assertEquals(1659, result.getNTx());
    Assertions.assertEquals("00000000000000000013a6ca3921ce63af646ac191c733d9728103d7a9e3236e",
                            result.getPreviousblockhash());
    Assertions.assertEquals("0000000000000000000d1e4cf9f6f0a6fa88aa162e08a966bf3043f7f77e21bb",
                            result.getNextblockhash());
  }

  @Test
  public void getBlockHeaderBytes() {
    String[][] mockedResponses = {new String[] {"getblockheader", "btc_getBlockHeaderBytes_1.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    byte[] expectedBytes = new byte[] {0, 0, (byte) 255, 63, 110, 35, (byte) 227, (byte) 169, (byte) 215, 3, (byte) 129, 114, (byte) 217, 51, (byte) 199, (byte) 145, (byte) 193,
                                       106, 100, (byte) 175, 99, (byte) 206, 33, 57, (byte) 202, (byte) 166, 19, 0, 0, 0, 0, 0, 0, 0, 0, 0, (byte) 193, 99, 75, 84, (byte) 182, 43, 31,
                                       (byte) 154, (byte) 144, (byte) 253, 15, 126, (byte) 166, 62, (byte) 176, 79, (byte) 234, (byte) 163, (byte) 218, (byte) 150, 38, (byte) 241, 92, 89, 63, (byte) 197, (byte) 201, (byte) 181, 95,
                                       (byte) 176, 39, 11, 12, (byte) 172, (byte) 224, 94, 53, 127, 20, 23, 120, (byte) 157, (byte) 186, 46};

    byte[] result = in3.getBtcAPI()
                        .getBlockHeaderBytes("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60");

    Assertions.assertArrayEquals(expectedBytes, result);
  }

  @Test
  public void getTransaction() {
    String[][] mockedResponses = {new String[] {"getrawtransaction", "btc_getTransaction_1.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    Transaction result = in3.getBtcAPI()
                             .getTransaction("1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d");

    Assertions.assertEquals("1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d", result.getTxid());
    Assertions.assertEquals(1591782412, result.getBlocktime());
    Assertions.assertEquals(17, result.getConfirmations());
    Assertions.assertEquals("371d1b497b6a9930b1f40c30ea18df4027bf452bb312c5945648d85d1f56dad5", result.getHash());
    Assertions.assertEquals(
        "02000000000101adba296f842330296785f061ca9e152dec63fe235d0ef1d4d38cc4a67f586c7c0000000000feffffff0280f0fa020000000017a914e9f20f1225a9528739495649405861ae5d77ba1e871cc9df050000000017a9141abf9a43d3a56f06930b95b1a8f0161bc0b0c9be8702483045022100a1c74e429c4e40ef90d915556ce4b54a9aa4a83872622d0dbbaca3029f07f2d802204d99cd230b2e1d7378401d502cf589227272173f93b3ccc4aed6f97988067e780121035ad17694971c7dadab72369ab2444e355ae7d17ed8fba67aab80da9a3556d37c7cac0900",
        result.getHex());
    Assertions.assertEquals(633980, result.getLocktime());
    Assertions.assertEquals(225, result.getSize());
    Assertions.assertEquals(1591782412, result.getTime());
    Assertions.assertEquals(2, result.getVersion());
    Assertions.assertEquals(1, result.getVin().length);
    Assertions.assertEquals(2, result.getVout().length);

    Assertions.assertEquals(4294967294L, result.getVin()[0].getSequence());
    Assertions.assertEquals(2, result.getVin()[0].getTxinwitness().length);
    Assertions.assertEquals(0, result.getVin()[0].getYout());
    Assertions.assertEquals("", result.getVin()[0].getScriptSig().getAsm());
    Assertions.assertEquals("", result.getVin()[0].getScriptSig().getHex());

    Assertions.assertEquals(2, result.getVout().length);
    Assertions.assertEquals(0, result.getVout()[0].getN());
    Assertions.assertEquals(0.5f, result.getVout()[0].getValue());
    Assertions.assertEquals("OP_HASH160 e9f20f1225a9528739495649405861ae5d77ba1e OP_EQUAL", result.getVout()[0].getScriptPubKey().getAsm());
    Assertions.assertEquals("a914e9f20f1225a9528739495649405861ae5d77ba1e87", result.getVout()[0].getScriptPubKey().getHex());
    Assertions.assertEquals(1, result.getVout()[0].getScriptPubKey().getAddresses().length);

    Assertions.assertEquals(143, result.getVsize());
    Assertions.assertEquals(570, result.getWeight());
  }

  @Test
  public void getTransactionBytes() {
    String[][] mockedResponses = {new String[] {"getrawtransaction", "btc_getTransactionBytes_1.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    byte[] result = in3.getBtcAPI()
                        .getTransactionBytes("1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d");

    Assertions.assertEquals(225, result.length);
  }

  @Test
  public void getBlockWithTxIds() {
    String[][] mockedResponses = {new String[] {"getblock", "btc_getBlockWithTxIds_1.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    Block result = in3.getBtcAPI()
                       .getBlockWithTxIds("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60");

    Assertions.assertEquals(result, result);
    Assertions.assertEquals("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60", result.getHash());
    Assertions.assertEquals(51, result.getConfirmations());
    Assertions.assertEquals(634026, result.getHeight());
    Assertions.assertEquals(1073676288, result.getVersion());
    Assertions.assertEquals("3fff0000", result.getVersionHex());
    Assertions.assertEquals("0b27b05fb5c9c53f595cf12696daa3ea4fb03ea67e0ffd909a1f2bb6544b63c1",
                            result.getMerkleroot());
    Assertions.assertEquals(1591782412, result.getTime());
    Assertions.assertEquals(1591778995, result.getMediantime());
    Assertions.assertEquals(783981944, result.getNonce());
    Assertions.assertEquals("17147f35", result.getBits());
    Assertions.assertEquals(13732352106018.34f, result.getDifficulty());
    Assertions.assertEquals("00000000000000000000000000000000000000001038fd1e673c4ff045dbd6c1",
                            result.getChainwork());
    Assertions.assertEquals(1659, result.getNTx());
    Assertions.assertEquals("00000000000000000013a6ca3921ce63af646ac191c733d9728103d7a9e3236e",
                            result.getPreviousblockhash());
    Assertions.assertEquals("0000000000000000000d1e4cf9f6f0a6fa88aa162e08a966bf3043f7f77e21bb",
                            result.getNextblockhash());
    Assertions.assertEquals(1659, result.getTransactionHashes().length);
  }

  @Test
  public void getBlockWithTxData() throws Exception {
    String[][] mockedResponses = {new String[] {"getblock", "btc_getBlockWithTxData_1.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    Block result = in3.getBtcAPI()
                       .getBlockWithTxData("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22");

    Assertions.assertEquals("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22", result.getHash());
    Assertions.assertEquals(83, result.getConfirmations());
    Assertions.assertEquals(634007, result.getHeight());
    Assertions.assertEquals(536870912, result.getVersion());
    Assertions.assertEquals("20000000", result.getVersionHex());
    Assertions.assertEquals("22f78daf63c48b582142421ea17e3a989b1421c1a9d07585668962f19fe12558",
                            result.getMerkleroot());
    Assertions.assertEquals(1591770949, result.getTime());
    Assertions.assertEquals(1591768893, result.getMediantime());
    Assertions.assertEquals(3201445374L, result.getNonce());
    Assertions.assertEquals("17147f35", result.getBits());
    Assertions.assertEquals(13732352106018.34f, result.getDifficulty());
    Assertions.assertEquals("000000000000000000000000000000000000000010380fd08a8436abc886cbc8",
                            result.getChainwork());
    Assertions.assertEquals(131, result.getNTx());
    Assertions.assertEquals("00000000000000000008c486d4d80a3bd24cc33b0011538baa8d1c3fa7d54c76",
                            result.getPreviousblockhash());
    Assertions.assertEquals("00000000000000000001d7fb302a3c72f7cc77d4def5d1d9503195469eb2049f",
                            result.getNextblockhash());
    Assertions.assertEquals(131, result.getTransactions().length);
  }

  @Test
  public void getBlockBytes() {
    String[][] mockedResponses = {new String[] {"getblock", "btc_getBlockBytes_1.json"}};
    IN3 in3                    = builder.constructClient(mockedResponses);

    byte[] result = in3.getBtcAPI()
                        .getBlockBytes("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22");

    Assertions.assertEquals(55812, result.length);
  }
}
