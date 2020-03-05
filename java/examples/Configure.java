/// Changing the default configuration

// In order to change the default configuration, just use the classes inside in3.config package.

package in3;

import in3.*;
import in3.config.*;
import in3.eth1.Block;

public class Configure {
  //
  public static void main(String[] args) {
    // create incubed client
    IN3 in3 = IN3.forChain(Chain.GOERLI); // set it to goerli

    // Setup a Configuration object for the client
    ClientConfiguration clientConfig = in3.getConfig();
    clientConfig.setReplaceLatestBlock(6); // define that latest will be -6
    clientConfig.setAutoUpdateList(false); // prevents node automatic update
    clientConfig.setMaxAttempts(1);        // sets max attempts to 1 before giving up
    clientConfig.setProof(Proof.none);     // does not require proof (not recommended)

    // Setup the ChainConfiguration object for the nodes on a certain chain
    ChainConfiguration chainConfiguration = new ChainConfiguration(Chain.GOERLI, clientConfig);
    chainConfiguration.setNeedsUpdate(false);
    chainConfiguration.setContract("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
    chainConfiguration.setRegistryId("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");

    in3.setConfig(clientConfig);

    Block block = in3.getEth1API().getBlockByNumber(Block.LATEST, true);
    System.out.println(block.getHash());
  }
}