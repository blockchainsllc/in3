## Installing

The Incubed Java client uses JNI in order to call native functions. But all the native-libraries are bundled inside the jar-file.
This jar file ha **no** dependencies and can even be used standalone:

like

```sh
java -cp in3.jar in3.IN3 eth_getBlockByNumber latest false
```

### Downloading

The jar file can be downloaded from the latest release. [here](https://github.com/blockchainsllc/in3/releases).

Alternatively, If you wish to download Incubed using the maven package manager, add this to your pom.xml

```
<dependency>
  <groupId>it.slock</groupId>
  <artifactId>in3</artifactId>
  <version>2.21</version>
</dependency>
```

After which, install in3 with `mvn install `.

### Building

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


## How to use IN3 in android project.
1. clone [in3](https://github.com/blockchainsllc/in3.git) in your project (or use script to update it):

```sh
#!/usr/bin/env sh
if [ -f in3/CMakeLists.txt ]; then
    cd in3
    git pull
    cd ..
else
    git clone https://github.com/blockchainsllc/in3.git
fi
```

2. add the native-build section and the additional source-set in your `build.gradle` in the app-folder inside the `android`-section:

```js
    externalNativeBuild {
        cmake {
            path file('../in3/CMakeLists.txt')
        }
    }
    sourceSets {
      main.java.srcDirs += ['../in3/java/src']
    }
```

if you want to configure which modules should be included, you can also specify the `externalNativeBuild` in the `defaultConfig':` 

```js
    defaultConfig {
        externalNativeBuild {
            cmake {
                arguments "-DBTC=OFF", "-DZKSYNC=OFF"
            }
        }
    }

```

For possible options, see https://in3.readthedocs.io/en/develop/api-c.html#cmake-options

Now you can use any Functions as defined here https://in3.readthedocs.io/en/develop/api-java.html

Here is example how to use it:

https://github.com/blockchainsllc/in3-example-android
