#!/bin/bash

. ./environment.sh >/dev/null
echo $PART_SRC

appSize=$(ls -l ./remote-application.bin  | awk  '{print $5}')

otaHex=$(grep app built_partition.table | head -n 1 | awk -F , '{print $5}' | awk -F K '{ print $1}')

echo "Application Size     $appSize"
otaDec=$(bc <<< " ${otaHex}*1024")
echo "Flash Partition Size $otaDec"
echo "Available            $(bc <<< "scale=2; ${otaDec}-${appSize}")"
percentage=$(bc <<< "scale=2; ${appSize}*100/${otaDec}")
echo "Used $percentage %"
