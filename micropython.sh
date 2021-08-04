#!/bin/bash
export MICROPYPATH=./pysrc/firmware/:./pysrc/modules/:./pysrc/live
../../../environments/micropython/ports/unix/micropython $*