## Installing



The Incubed Java client uses JNI in order to call native functions. But all the native-libraries are bundled inside the jar-file.
This jar file ha **no** dependencies and can even be used standalone: 

like

```sh
java -cp in3.jar in3.IN3 eth_getBlockByNumber latest false
```

### Downloading


The jar file can be downloaded from the latest release. [here](https://github.com/slockit/in3-c/releases).

Alternatively, If you wish to download Incubed using the maven package manager, add this to your pom.xml
```
<dependency>
  <groupId>it.slock</groupId>
  <artifactId>in3</artifactId>
  <version>2.21</version>
</dependency> 
```

After which, install in3 with ```mvn install ```.

###  Building

For building the shared library you need to enable java by using the `-DJAVA=true` flag:

```sh
git clone git@github.com:slockit/in3-c.git
mkdir -p in3-c/build
cd in3-c/build
cmake -DJAVA=true .. && make
```

You will find the `in3.jar` in the build/lib - folder.

### Android

In order to use Incubed in android simply follow these steps:

Step 1: Create a top-level CMakeLists.txt in android project inside app folder and link this to gradle. Follow the steps using this [guide](https://developer.android.com/studio/projects/gradle-external-native-builds) on howto link.

The Content of the `CMakeLists.txt` should look like this:

```sh

cmake_minimum_required(VERSION 3.4.1)

# turn off FAST_MATH in the evm.
ADD_DEFINITIONS(-DIN3_MATH_LITE)

# loop through the required module and cretae the build-folders
foreach(module 
  c/src/core 
  c/src/verifier/eth1/nano 
  c/src/verifier/eth1/evm 
  c/src/verifier/eth1/basic 
  c/src/verifier/eth1/full 
  java/src
  c/src/third-party/crypto 
  c/src/third-party/tommath 
  c/src/api/eth1)
        file(MAKE_DIRECTORY in3-c/${module}/outputs)
        add_subdirectory( in3-c/${module} in3-c/${module}/outputs )
endforeach()

```

Step 2: clone [in3-c](https://github.com/slockit/in3-c.git) into the `app`-folder or use this script to clone and update in3:

```sh
#!/usr/bin/env sh

#github-url for in3-c
IN3_SRC=https://github.com/slockit/in3-c.git

cd app

# if it exists we only call git pull
if [ -d in3-c ]; then
    cd in3-c
    git pull
    cd ..
else
# if not we clone it
    git clone $IN3_SRC
fi


# copy the java-sources to the main java path
cp -r in3-c/java/src/in3 src/main/java/
```

Step 3: Use methods available in app/src/main/java/in3/IN3.java from android activity to access IN3 functions.


Here is example how to use it:

https://github.com/slockit/in3-example-android


