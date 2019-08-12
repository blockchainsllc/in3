#!/bin/sh
echo "" >> debian/changelog

# need to modify the below line to use the tag number as the version number
echo "in3 (0.1.13~bionic) bionic; urgency=medium" >> debian/changelog
echo "" >> debian/changelog

# need to find a better way to add descriptions, maybe we take the tag message
echo "  * Updated file" >> debian/changelog
echo "" >> debian/changelog

echo  " -- devops_slock.it <devops@slock.it> " $(date -R)  >> debian/changelog
echo "" >> debian/changelog
