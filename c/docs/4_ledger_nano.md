## Integration of Ledger Nano S

 1. Ways to integrate Ledger Nano S
 2. Build incubed source with ledger nano module
 3. Start using ledger nano s device with Incubed 


### Ways to integrate Ledger Nano S
 Currently there are two ways to integrate Ledger Nano S with incubed for transaction and message signing:
 1. Install Ethereum app from Ledger Manager
 2. Setup development environment and install incubed  signer app on your Ledger device

Option 1 is the convinient choice for most of the people as incubed signer app is not available to be installed from Ledger Manager and it will take efforts to configure development environment for ledger manager. 
The main differences in above approaches are following:

```
Ethereum official Ledger app requires rlp encoded transactions for  signing and there is not much scope for customization.Currently we have support for following operations with Ethereum app:
1. Getting public key
2. Sign Transactions
3. Sign Messages

Incubed signer app required just hash , so it is better option if you are looking to integrate incubed in such a way that you would manage all data formation on your end and use just hash to get signture from Ledger Nano S and use the signature as per your wish. 
```
If you are confortable with Option 1 , all you need to do is setup you Ledger device as per usual instructions and install Ethereum app form Ledger Manager store. Otherwise if you are interested in Option 2 Please follow all the instructions given in "Setup development environment for ledger nano s" section . 


#### Setup development environment for ledger nano s
 Setting up dev environment for Ledger nano s is one time activity and incubed signer application will be available to install directly from Ledger Manager in future. Ledger nano applications need linux System (recommended is Ubuntu) to build the binary to be installed on Ledger nano devices
  
##### Download Toolchains and Nanos ledger SDK (As per latest Ubuntu LTS)

Download the Nano S SDK in bolos-sdk folder
```sh
$ git clone https://github.com/ledgerhq/nanos-secure-sdk
```

```
Download a prebuild gcc and move it to bolos-sdk folder
		Ref: https://launchpad.net/gcc-arm-embedded/+milestone/5-2016-q1-update

Download a prebuild clang and rename the folder to clang-arm-fropi then move it to bolos-sdk folder
		Ref: http://releases.llvm.org/download.html#4.0.0 
```

##### Add environment variables:
```sh
sudo -H gedit /etc/environment
```

```
ADD PATH TO BOLOS SDK:
BOLOS_SDK="<path>/nanos-secure-sdk"

ADD GCCPATH VARIABLE
GCCPATH="<path>/gcc-arm-none-eabi-5_3-2016q1/bin/"

ADD CLANGPATH
CLANGPATH="<path>/clang-arm-fropi/bin/"
```

##### Download and install ledger python tools 

Installation prerequisites : 

```sh
$ sudo apt-get install libudev-dev <
$ sudo apt-get install libusb-1.0-0-dev 
$ sudo apt-get install python-dev (python 2.7)
$ sudo apt-get install virtualenv
```

##### Installation of ledgerblue:

```sh
$ virtualenv ledger
$ source ledger/bin/activate
$ pip install ledgerblue
```

Ref: https://github.com/LedgerHQ/blue-loader-python

##### Download and install ledger udev rules 
```sh
$ git clone https://github.com/LedgerHQ/udev-rules
```
run script from the above download 
```sh
$ sudo ./add_udev_rules.sh
```

##### Open new terminal and check for following installations
```sh
$ sudo apt-get install gcc-multilib
$ sudo apt-get install libc6-dev:i386
```

##### Install incubed signer app 
Once you complete all the steps, go to folder "c/src/signer/ledger-nano/firmware" and run following command
```sh
make load
```
It will ask you to enter pin for approve installation on ledger nano device. follow all the steps and it will be done. 

### Build incubed source with ledger nano module

To build incubed source with ledger nano:-
1. Open root CMakeLists file and find LEDGER_NANO option
2. Turn LEDGER_NANO option ON which is by default OFF
3. Build incubed source 
```sh
    cd build
    cmake  .. && make
```


### Start using ledger nano s device with Incubed 

Open the application on your ledger nano s usb device and make signing requests from incubed. 

Following is the sample command to sendTransaction from command line utility:- 
```sh
bin/in3 send -to 0xd46e8dd67c5d32be8058bb8eb970870f07244567  -gas 0x96c0  -value 0x9184e72a  -path 0x2c3c000000 -debug
```

-path points to specific public/private key pair inside HD wallet derivation path . For Ethereum the default 
 path is m/44'/60'/0'/0 , which we can pass in simplified way as hex string  i.e [44,60,00,00,00] => 0x2c3c000000

 If you want to use apis to integrate ledger nano support in your incubed application , feel free to explore apis given following header files:-

 ```
 ledger_signer.h : It contains APIs to integrate ledger nano device with incubed signer app.
 ethereum_apdu_client.h : It contains APIs to integrate ledger nano device with Ethereum ledger app.
 ```