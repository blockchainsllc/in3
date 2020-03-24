# Integration of ledger nano s with incubed 
 1. Setup development environment for ledger nano s
 2. Build and install ledger nano Signer app into Ledger nano s usb device 
 3. Start using ledger nano s device with Incubed 

# Setup development environment for ledger nano s
 Setting up dev environment for Ledger nano s is one time activity and Signer application will be available to install directly from Ledger Manager in future. Ledger applications need linux System (recommended is Ubuntu) to build the binary to be installed on Ledger nano devices
  
## Download Toolchains and Nanos ledger SDK (As per latest Ubuntu LTS)

Download the Nano S SDK in bolos-sdk folder
$ git clone https://github.com/ledgerhq/nanos-secure-sdk

Download a prebuild gcc and move it to bolos-sdk folder
		https://launchpad.net/gcc-arm-embedded/+milestone/5-2016-q1-update

Download a prebuild clang and rename the folder to clang-arm-fropi then move it to bolos-sdk folder
		http://releases.llvm.org/download.html#4.0.0 


## Add environment variables:

sudo -H gedit /etc/environment

ADD PATH TO BOLOS SDK:
BOLOS_SDK="<path>/nanos-secure-sdk"

ADD GCCPATH VARIABLE
GCCPATH="<path>/gcc-arm-none-eabi-5_3-2016q1/bin/"

ADD CLANGPATH
CLANGPATH="<path>/clang-arm-fropi/bin/"


## Download and install ledger python tools 

Installation prerequisites :
$ sudo apt-get install libudev-dev
$ sudo apt-get install libusb-1.0-0-dev
$ sudo apt-get install python-dev (python 2.7)
$ sudo apt-get install virtualenv

Installation of ledgerblue:
$ virtualenv ledger
$ source ledger/bin/activate
$ pip install ledgerblue

Ref: https://github.com/LedgerHQ/blue-loader-python



## Download and install ledger udev rules 

$ git clone https://github.com/LedgerHQ/udev-rules

run script from the above download 
$ sudo ./add_udev_rules.sh



## Open new terminal and check for following installations :-

$ sudo apt-get install gcc-multilib
$ sudo apt-get install libc6-dev:i386
===================================================================

# Build and install ledger nano Signer app into Ledger nano s usb device 
Once the setup is done,  go to ledger-incubed-firmware-app folder and run:-

$ make
$ make load

===================================================================

# Start using ledger nano s device with Incubed 



Open the application on your ledger nano s usb device and make signing requests from incubed