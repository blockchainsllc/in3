# addrs.awk --- simple mailing list program

# Records are separated by blank lines.
# Each line is one field.
BEGIN { print "## CMake options"; print "" ;RS = "" ; FS = "\n"  }

{
    if (split($1, d, /\/\//) == 2 && index($2,"CMAKE") == 0 && index($2,"CURL_") == 0 &&  index($2,"JAVA_")==0 && index($2,"DOXYGEN_")==0 && index($2,"Java_") == 0) 
    {
      split($2, a, /[:=]/)
      print "### "a[1]
      print ""
      print " "d[2]
      print ""
      print "  Type: `" a[2], "` ,    Default-Value: `" a[3]"`"
      print ""
      print ""

    }
}