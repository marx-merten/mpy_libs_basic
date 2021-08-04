#!/bin/bash
. ./environment.sh

echo "Choosen $PORT"
rshell -p ${PORT} $*