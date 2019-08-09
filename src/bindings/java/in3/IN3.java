package in3;

import java.net.*;
import java.io.*;
import in3.JSON;
import in3.Proof;
import in3.StorageProvider;

/**
 * This is the main class creating the incubed client. The client can then be
 * configured.
 *
 */
public class IN3 {

    private long ptr;

    /** number of seconds requests can be cached. */
    public native int getCacheTimeout();

    /** sets number of seconds requests can be cached. */
    public native void setCacheTimeout(int val);

    /** the limit of nodes to store in the client. */
    public native int getNodeLimit();

    /** sets the limit of nodes to store in the client. */
    public native void setNodeLimit(int val);

    /** the client key to sign requests */
    public native byte[] getKey();

    /** sets the client key to sign requests */
    public native void setKey(byte[] val);

    /** number of max bytes used to cache the code in memory */
    public native int getMaxCodeCache();

    /** sets number of max bytes used to cache the code in memory */
    public native void setMaxCodeCache(int val);

    /** number of blocks cached in memory */
    public native int getMaxBlockCache();

    /** sets the number of blocks cached in memory */
    public native void setMaxBlockCache(int val);

    /** the type of proof used */
    public native Proof getProof();

    /** sets the type of proof used */
    public native void setProof(Proof val);

    /** the number of request send when getting a first answer */
    public native int getRequestCount();

    /** sets the number of requests send when getting a first answer */
    public native void setRequestCount(int val);

    /** the number of signatures used to proof the blockhash. */
    public native int getSignatureCount();

    /** sets the number of signatures used to proof the blockhash. */
    public native void setSignatureCount(int val);

    /**
     * min stake of the server. Only nodes owning at least this amount will be
     * chosen.
     */
    public native long getMinDeposit();

    /**
     * sets min stake of the server. Only nodes owning at least this amount will be
     * chosen.
     */
    public native void setMinDeposit(long val);

    /**
     * if specified, the blocknumber *latest* will be replaced by blockNumber-
     * specified value
     */
    public native int getReplaceLatestBlock();

    /** replaces the *latest* with blockNumber- specified value */
    public native void setReplaceLatestBlock(int val);

    /** the number of signatures in percent required for the request */
    public native int getFinality();

    /** sets the number of signatures in percent required for the request */
    public native void setFinality(int val);

    /** the max number of attempts before giving up */
    public native int getMaxAttempts();

    /** sets the max number of attempts before giving up */
    public native void setMaxAttempts(int val);

    /**
     * specifies the number of milliseconds before the request times out. increasing
     * may be helpful if the device uses a slow connection.
     */
    public native int getTimeout();

    /**
     * specifies the number of milliseconds before the request times out. increasing
     * may be helpful if the device uses a slow connection.
     */
    public native void setTimeout(int val);

    /** servers to filter for the given chain. The chain-id based on EIP-155. */
    public native long getChainId();

    /** sets the chain to be used. The chain-id based on EIP-155. */
    public native void setChainId(long val);

    /**
     * if true the nodelist will be automaticly updated if the lastBlock is newer
     */
    public native boolean isAutoUpdateList();

    /**
     * activates the auto update.if true the nodelist will be automaticly updated if
     * the lastBlock is newer
     */
    public native void setAutoUpdateList(boolean val);

    /** provides the ability to cache content */
    public native StorageProvider getStorageProvider();

    /**
     * provides the ability to cache content like nodelists, contract codes and
     * validatorlists
     */
    public native void setStorageProvider(StorageProvider val);

    /**
     * send a request. The request must a valid json-string with method and params
     */
    public native String send(String request);

    /**
     * send a request but returns a object like array or map with the parsed
     * response. The request must a valid json-string with method and params
     */
    public native Object sendobject(String request);

    private String toRPC(String method, Object[] params) {
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
            } else
                p += params[i].toString();
        }
        return "{\"method\":\"" + method + "\", \"params\":[" + p + "]}";
    }

    /**
     * send a RPC request by only passing the method and params. It will create the
     * raw request from it and return the result.
     */
    public String sendRPC(String method, Object[] params) {
        return this.send(toRPC(method, params));
    }

    /**
     * send a RPC request by only passing the method and params. It will create the
     * raw request from it and return the result.
     */
    public Object sendRPCasObject(String method, Object[] params) {
        JSON j = new JSON();
        return this.sendobject(toRPC(method, params));
    }

    private native void free();

    private native long init();

    /** constrcutor. creates a new Incubed client. */
    public IN3() {
        ptr = init();
    }

    protected void finalize() {
        free();
    }

    static {
        System.loadLibrary("in3");
    }

    /** internal function to handle the internal requests */
    static byte[][] sendRequest(String[] urls, byte[] payload) {
        byte[][] result = new byte[urls.length][];
        // todo run it in threads....
        for (int i = 0; i < urls.length; i++) {
            try {
                URL url = new URL(urls[i]);
                URLConnection con = url.openConnection();
                HttpURLConnection http = (HttpURLConnection) con;
                http.setRequestMethod("POST");
                http.setUseCaches(false);
                http.setDoOutput(true);
                http.setRequestProperty("Content-Type", "application/json");
                http.setRequestProperty("Accept", "application/json");
                http.setRequestProperty("charsets", "utf-8");
                http.connect();
                OutputStream os = http.getOutputStream();
                os.write(payload);
                InputStream is = http.getInputStream();

                ByteArrayOutputStream buffer = new ByteArrayOutputStream();
                int nRead;
                byte[] data = new byte[1024];
                while ((nRead = is.read(data, 0, data.length)) != -1)
                    buffer.write(data, 0, nRead);

                buffer.flush();
                is.close();
                result[i] = buffer.toByteArray();
            } catch (Exception ex) {
                result[i] = null;
            }
        }
        return result;
    }

    // Test it
    public static void main(String[] args) {
        Object[] params = new Object[args.length - 1];
        for (int i = 1; i < args.length; i++)
            params[i - 1] = args[i];
        System.out.println(new IN3().sendRPC(args[0], params));
    }
}