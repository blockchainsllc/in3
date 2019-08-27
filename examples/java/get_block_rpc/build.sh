#!/bin/sh

# build ...
javac -cp ../../../build/lib/in3.jar Example.java

#run
java -cp ../../../build/lib/in3.jar:. Example
