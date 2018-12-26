cd src/libin3/java
javac -h .. in3/*.java
cp -r in3 ../../../build/src/libin3
cd ../../../build/src/libin3
java in3.IN3 eth_blockNumber
cd ../../../ 