#!/bin/bash
. ./environment.sh


oldmd5=$(cat .life_md5)
newmd5=$(find pysrc/live -type f -exec md5sum '{}' ';' | md5sum | awk '{print$1}')
if [[ "$newmd5" != "$oldmd5" ]]; then
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
${DIR}/boardShell.sh rsync -m  $DIR/pysrc/live /pyboard
fi
echo "$newmd5">.life_md5
