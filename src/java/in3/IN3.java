package in3;

import in3.Proof;
import in3.StorageProvider;

public class IN3 {  

    private long ptr;

    /** number of seconds requests can be cached. */
    public native int getCacheTimeout();
    public native void setCacheTimeout(int val);

    /** the limit of nodes to store in the client. */
    public native int getNodeLimit();
    public native void setNodeLimit(int val);

    /** the client key to sign requests */
    public native byte[] getKey(); 
    public native void setKey(byte[] val); 

    /** number of max bytes used to cache the code in memory */
    public native int getMaxCodeCache();
    public native void setMaxCodeCache(int val);

    /** number of number of blocks cached  in memory */
    public native int getMaxBlockCache();
    public native void setMaxBlockCache(int val);

    /** the type of proof used */
    public native Proof getProof();
    public native void setProof(Proof val);

    /** the number of request send when getting a first answer */
    public native int getRequestCount(); 
    public native void setRequestCount(int val); 

    /** the number of signatures used to proof the blockhash. */
    public native int getSignatureCount();
    public native void setSignatureCount(int val);

    /** min stake of the server. Only nodes owning at least this amount will be chosen. */
    public native long getMinDeposit();
    public native void setMinDeposit(long val);

    /** if specified, the blocknumber *latest* will be replaced by blockNumber- specified value */
    public native int getReplaceLatestBlock();
    public native void setReplaceLatestBlock(int val);

    /** the number of signatures in percent required for the request*/
    public native int getFinality();
    public native void setFinality(int val);

    /** the max number of attempts before giving up*/
    public native int getMaxAttempts();
    public native void setMaxAttempts(int val);

    /** specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection. */
    public native int getTimeout();  
    public native void setTimeout(int val);  

    /** servers to filter for the given chain. The chain-id based on EIP-155.*/
    public native long getChainId(); 
    public native void setChainId(long val); 

    /** if true the nodelist will be automaticly updated if the lastBlock is newer */
    public native boolean isAutoUpdateList(); 
    public native void setAutoUpdateList(boolean val); 

    /** provides the ability to cache content*/
    public native StorageProvider getStorageProvider();
    public native void setStorageProvider(StorageProvider val);

    /** send a request. The request must a valid json-string with method and params */
    public native String send(String request);

    public String sendRPC(String method, Object[] params) {
        String p = "";
        for (int i=0;i<params.length;i++) {
            if (p.length()>0) p+=",";
            if (params[i]==null)
               p+="null";
            else if (params[i] instanceof String) {
                String s = (String) params[i];
                if (s.charAt(0)=='{' || s.equals("true") || s.equals("false"))
                   p+=s;
                else
                   p+="\""+s+"\"";
            }
            else
               p+=params[i].toString();
        }
 
        return this.send("{\"method\":\""+method+"\", \"params\":["+p+"]}");
    }

    private native void free();

    public IN3() {
        ptr = init();
    }

    protected void finalize() {
        free();
    }

    /** a cache handler offering 2 functions ( setItem(string,string), getItem(string) ) */
//    in3_storage_handler_t* cacheStorage;

  
   static {
      System.loadLibrary("in3"); 
   }
 
   // Declare an instance native method sayHello() which receives no parameter and returns void
   private native long init();

 

   // Test Driver
   public static void main(String[] args) {
       Object[] params = new Object[args.length-1];
       for (int i=1;i<args.length;i++)
          params[i-1]=args[i];
//       try{System.in.read();}
//       catch(Exception e){}
       System.out.println(new IN3().sendRPC(args[0],params));
//      new HelloJNI().sayHello();  // Create an instance and invoke the native method
   }
}