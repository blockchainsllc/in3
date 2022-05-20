/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
 *
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 *
 *
 * COMMERCIAL LICENSE USAGE
 *
 * Licensees holding a valid commercial license may use this file in accordance
 * with the commercial license agreement provided with the Software or, alternatively,
 * in accordance with the terms contained in a written agreement between you and
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further
 * information please contact slock.it at in3@slock.it.
 *
 * Alternatively, this file may be used under the AGPL license as follows:
 *
 * AGPL LICENSE USAGE
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available
 * complete source code of licensed works and modifications, which include larger
 * works using a licensed work, under the same license. Copyright and license notices
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

package in3;

import in3.config.ClientConfiguration;
import in3.eth1.SimpleWallet;
import in3.utils.Crypto;
import in3.utils.CurveType;
import in3.utils.JSON;
import in3.utils.PayloadType;
import in3.utils.SignatureType;
import in3.utils.Signer;
import in3.utils.StorageProvider;
import in3.utils.TransportException;

/**
 * This is the main class creating the incubed client. The client can then be
 * configured.
 *
 */
public class IN3 {
  private static final String CONFIG      = "in3_config";
  private static final String NODE_LIST   = "in3_nodeList";
  private static final String SIGN        = "in3_sign";
  private static final String CACHE_CLEAR = "in3_cacheClear";

  private static final String ENS_SUFFIX = ".ETH";

  static {
    Loader.loadLibrary();
  }

  private long            ptr;
  private StorageProvider provider;
  private Signer          signer;

  private static IN3Transport transport = new IN3DefaultTransport();
  private ClientConfiguration config;

  private IN3(long chainAlias) {
    ptr         = init(chainAlias);
    this.config = new ClientConfiguration(this.getDefaultConfig());
  }

  /**
   * creates a client with the default config.
   */
  public IN3() {
    ptr         = init(1);
    this.config = new ClientConfiguration(this.getDefaultConfig());
    this.config.markAsSynced();
  }

  /**
   * create a Incubed client using the chain-config. if chainId is
   * Chain.MULTICHAIN, the client can later be switched between different chains,
   * for all other chains, it will be initialized only with the chainspec for this
   * one chain (safes memory)
   */
  public static IN3 forChain(long chainId) {
    return new IN3(chainId);
  }

  protected void finalize() {
    free();
  }

  /**
   * sets config object in the client
   */
  private native void setConfig(String val);

  protected void setConfig(ClientConfiguration config) {
    this.config = config;
  }

  protected void applyConfig() {
    setConfig(config.toJSON());
    config.markAsSynced();
  }

  /**
   * returns the current configuration. any changes to the configuration will be
   * applied witth the next request.
   */
  public ClientConfiguration getConfig() {
    return config;
  }

  /**
   * sets the signer or wallet.
   */
  public void setSigner(Signer signer) {
    this.signer = signer;
  }

  /**
   * returns the signer or wallet.
   */
  public Signer getSigner() {
    return signer;
  }

  /**
   * gets the ipfs-api
   */
  public in3.ipfs.API getIpfs() {
    return new in3.ipfs.API(this);
  }

  /**
   * gets the zksync-api
   */
  public in3.zksync.API getZksync() {
    return new in3.zksync.API(this);
  }

  /**
   * gets the btc-api
   */
  public in3.btc.API getBtcAPI() {
    return new in3.btc.API(this);
  }

  /**
   * gets the ethereum-api
   */
  public in3.eth1.API getEth1API() {
    return new in3.eth1.API(this);
  }

  /**
   * gets the utils/crypto-api
   */
  public Crypto getCrypto() {
    return new Crypto(this);
  }

  /**
   * provides the ability to cache content like nodelists, contract codes and
   * validatorlists
   */
  public void setStorageProvider(StorageProvider val) {
    provider = val;
    initcache();
  }

  /**
   * provides the ability to cache content
   */
  public StorageProvider getStorageProvider() {
    return provider;
  }

  /**
   * sets The transport interface. This allows to fetch the result of the incubed
   * in a different way.
   */
  public void setTransport(IN3Transport newTransport) {
    IN3.transport = newTransport;
  }

  /**
   * returns the current transport implementation.
   */
  public IN3Transport getTransport() {
    return IN3.transport;
  }

  /**
   * servers to filter for the given chain. The chain-id based on EIP-155.
   */
  public native long getChainId();

  /**
   * sets the chain to be used. The chain-id based on EIP-155.
   */
  public native void setChainId(long val);

  /**
   * send a request. The request must a valid json-string with method and params
   */
  public String send(String request) {
    if (!config.isSynced()) {
      this.applyConfig();
    }
    return sendinternal(request);
  }

  private native String sendinternal(String request);

  /**
   * send a request but returns a object like array or map with the parsed
   * response. The request must a valid json-string with method and params
   */
  public Object sendobject(String request) {
    if (!config.isSynced()) {
      this.applyConfig();
    }
    return sendobjectinternal(request);
  }

  private native Object sendobjectinternal(String request);

  private String toRPC(String method, Object[] params) {
    StringBuilder p = new StringBuilder();
    for (int i = 0; i < params.length; i++) {
      if (p.length() > 0)
        p.append(",");
      if (params[i] == null)
        p.append("null");
      else if (params[i] instanceof byte[]) {
        byte[] b = (byte[]) params[i];
        p.append("\"0x");
        for (int j = 0; j < b.length; j++) {
          p.append(Character.forDigit((b[j] >> 4) & 0xF, 16));
          p.append(Character.forDigit(b[j] & 0xF, 16));
        }
        p.append("\"");
      }
      else if (params[i] instanceof String) {
        String s = (String) params[i];
        if (s.charAt(0) == '{' || s.equals("true") || s.equals("false"))
          p.append(s);
        else
          p.append("\"" + s + "\"");
      }
      else
        p.append(JSON.toJson(params[i]));
    }
    return "{\"method\":\"" + method + "\", \"params\":[" + p + "]}";
  }

  private String toRPC(String method, Object[] params, IN3Props props) {
    String p = "";
    for (int i = 0; i < params.length; i++) {
      if (p.length() > 0)
        p += ",";
      if (params[i] == null)
        p += "null";
      else if (params[i] instanceof String) {
        String s = (String) params[i];
        if (s.charAt(0) == '{' || s.equals("true") || s.equals("false"))
          p += s;
        else
          p += "\"" + s + "\"";
      }
      else
        p += JSON.toJson(params[i]);
    }

    return "{\"in3\":" + props.toJSON() + ", \"method\":\"" + method + "\", \"params\":[" + p + "]}";
  }

  /**
   * send a RPC request by only passing the method and params. It will create the
   * raw request from it and return the result.
   */
  public String sendRPC(String method, Object[] params) {
    return this.send(toRPC(method, params));
  }

  private Object sendObjectRPC(String method, Object[] params, IN3Props props) {
    return this.sendobject(toRPC(method, params, props));
  }

  public Object sendRPCasObject(String method, Object[] params, boolean useEnsResolver) {
    Object[] resolvedParams = useEnsResolver ? handleEns(params) : params;
    return this.sendobject(toRPC(method, resolvedParams));
  }

  /**
   * send a RPC request by only passing the method and params. It will create the
   * raw request from it and return the result.
   */
  public Object sendRPCasObject(String method, Object[] params) {
    return sendRPCasObject(method, params, true);
  }

  /**
   * internal function to handle the internal requests
   */
  static byte[][] sendRequest(String method, String[] urls, byte[] payload, String[] headers) throws TransportException {
    return IN3.transport.handle(method, urls, payload, headers);
  }

  private native void free();

  private native long init(long chainId);

  private native void initcache();

  private native Object getDefaultConfig();

  /**
   * returns the current incubed version.
   */
  public static native String getVersion();

  /**
   * clears the cache.
   */
  public boolean cacheClear() {
    return (boolean) sendRPCasObject(CACHE_CLEAR, new Object[] {});
  }

  /**
   * restrieves the node list
   */
  public IN3Node[] nodeList() {
    return nodeList(new String[] {});
  }

  /**
   * restrieves the node list
   */
  protected IN3Node[] nodeList(String[] signerNodeAddresses) {
    NodeList nl;
    if (signerNodeAddresses != null && signerNodeAddresses.length > 0) {
      IN3Props props = new IN3Props();
      props.setSignerNodes(signerNodeAddresses);
      nl = NodeList.asNodeList(sendObjectRPC(NODE_LIST, new Object[] {}, props));
    }

    nl = NodeList.asNodeList(sendRPCasObject(NODE_LIST, new Object[] {}));
    return nl.getNodes();
  }

  /**
   * request for a signature of an already verified hash.
   */
  public SignedBlockHash[] sign(BlockID[] blocks, String[] dataNodeAdresses) {
    if (dataNodeAdresses != null && dataNodeAdresses.length > 0) {
      IN3Props props = new IN3Props();
      props.setDataNodes(dataNodeAdresses);
      return SignedBlockHash.asSignedBlockHashs(sendObjectRPC(SIGN, new Object[] {blocks}, props));
    }

    return SignedBlockHash.asSignedBlockHashs(sendRPCasObject(SIGN, new Object[] {blocks}));
  }

  protected Object[] handleEns(Object[] params) {
    Object[] result = params.clone();
    for (int i = 0; i < result.length; i++) {
      if (result[i] != null && result[i].toString().toUpperCase().endsWith(ENS_SUFFIX)) {
        result[i] = (Object) getEth1API().ens(result[i].toString());
      }
    }

    return result;
  }

  // Test it
  public static void main(String[] args) {
    IN3          in3      = new IN3(Chain.GOERLI);
    SimpleWallet wallet   = new SimpleWallet();
    String       pk       = "0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6";
    String       signerId = wallet.addRawKey(pk);
    byte[] res            = wallet.sign("1e194c68360307cfb715bf17878791ad1ced8da7d2e5f42b691074c577f41eac",
                                        signerId, SignatureType.ethSign, PayloadType.ethTx, CurveType.ecdsa, null);

    System.out.println(res);
  }
}