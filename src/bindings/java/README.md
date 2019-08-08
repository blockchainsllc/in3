## Installing



The Incubed Java client uses JNI in order to call native functions. That's why to use it you need to put the shared library in the path where java will be able to find it. 

The shared library (`in3.dll` (windows), `libin3.so` (linux) or `libin3.dylib` (osx) ), can either be downloaded (make sure you know your targetsystem) or build from sources.

like

```sh
java -Djava.library.path="path_to_in3;${env_var:PATH}" HelloIN3.class
```


###  Building

For building the shared library you need to enable java by using the `-DJAVA=true` flag:

```sh
git clone git@github.com:slockit/in3-core.git
mkdir -p in3-core/build
cd in3-core/build
cmake -DJAVA=true .. && make
```

You will find the `in3.jar` and the `libin3.so` in the build/lib - folder.

### Android

In order to use incubed in android simply follow this example:

https://github.com/SlockItEarlyAccess/in3-android-example


## Example


```java
import org.json.*;
import in3.IN3;

public class HelloIN3 {  
   // 
   public static void main(String[] args) {
       String blockNumber = args[0]; 
       IN3 in3 = new IN3();
       JSONObject result = new JSONObject(in3.sendRPC("eth_getBlockByNumber",{ blockNumber ,true})));
       ....
   }
}
```

