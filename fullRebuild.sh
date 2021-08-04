#!/bin/bash
rm -f .firmware_md5
rm -f .life_md5
./buildFirmware.sh clean
if [[ "$1" -eq "erase" ]]; then
    ./buildFirmware.sh erase
fi
./rebuild.sh $*