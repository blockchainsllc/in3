Key Management Demo
###################

Overview
********
The demo application verifies the hardware unique key derivation and store encryption keys.

Building
********
Docker Image
************
1. Enter the project root directory here it is 'key-management'directory.
#. Run the docker image: docker.slock.it/build-images/cmake:zephyr-sdk
.. code-block:: console
    docker run -it -v $PWD:$PWD docker.slock.it/build-images/cmake:zephyr-sdk
#. Change directory to 'demo/key-management/'
#. Execute the following command. 
.. code-block:: console
    mkdir build && cd build 
#. Now execute the below given cmake command. 
.. code-block:: console
    cmake -DBOARD=nrf5340dk_nrf5340_cpuapp -DARM_MBEDTLS_PATH=/ncs/mbedtls/ ..

VSCode Extension
****************
1. Install the NRF Connect extension for VSCode in Visual Studio Code.
2. Select nRF Connect from the side icon of Visual Studio Code.
3. Next open our demo application(nRF based) by clicking "Add an existing application" and browse to desired application. 
4. Expand the side icon "ACTIONS" and just click "Build". 

Flashing
********
Docker Image
************
There is some constraints in flash and debug the application from the docker image. Will figure it out how to achieve that as well. 

VSCode Extension
****************
1. Connect the target board and host machine using an USB cable.
2. Power On the target(nRF5340 DK) and you will see the LED4 blinking.
3. Check for the JLINK is detected in your host machine. 
4. Expand the "CONNECTED DEVICES" and check the device's serial number is visible. 
5. If everything looks good as mentioned above steps then click the "Flash" icon from "Actions".


Test
**** 
