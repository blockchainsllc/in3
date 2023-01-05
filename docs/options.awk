# addrs.awk --- simple mailing list program

# Records are separated by blank lines.
# Each line is one field.
BEGIN { 
    print "# Building"
    print ""
    print "While we provide binaries, you can also build from source:"
    print ""
    print "### requirements"
    print ""
    print "- cmake"
    print "- curl : curl is used as transport for command-line tools, but you can also compile it without curl (`-DUSE_CURL=false -DCMD=false`), if you want to implement your own transport."
    print ""
    print "Incubed uses cmake for configuring:"
    print ""
    print "```sh"
    print "mkdir build && cd build"
    print "cmake -DCMAKE_BUILD_TYPE=Release .. && make"
    print "make install"
    print "```"
    print ""
    print "### CMake options"; 
    print "" ;
    print "When configuring cmake, you can set a lot of different incubed specific like `cmake -DEVM_GAS=false ..`." ;
    print "" ;
    RS = "" ; 
    FS = "\n"  
}

{
    if (split($1, d, /\/\//) == 2 && index($2,"CMAKE") == 0 && index($2,"CURL_") == 0 &&  index($2,"JAVA_")==0 && index($2,"DOXYGEN_")==0 && index($2,"Java_") == 0 && index($2,"scrypt_") == 0) 
    {
      split($2, a, /[:=]/)
      print "#### "a[1]
      print ""
      print " "d[2]
      print ""
      print "Default-Value: `-D" a[1] "=" a[3]"`"
      print ""
      print ""

    }
}