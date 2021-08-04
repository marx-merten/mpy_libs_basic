curdir=$(pwd)
find . -name "__pycache__" -type d -prune -exec rm -rf '{}' '+'

. ./environment.sh

pushd ${MICROPYTHON_DIR}/ports/esp32

echo "building"
make -j8 $*
error=$?
if [ $error -ne 0 ]; then
    echo "BUILD Failed please check logfile or console. - $error"
    exit $error
fi
[[ -f "./build-${BOARD}/firmware.bin" ]] && cp ./build-${BOARD}/firmware.bin ${curdir}/remote-firmware.bin
[[ -f "./build-${BOARD}/application.bin" ]] && cp ./build-${BOARD}/micropython.bin ${curdir}/remote-application.bin
[[ -f "./build-${BOARD}/partition_table/partition-table.bin" ]] \
    && python ${ESPIDF}/components/partition_table/gen_esp32part.py \
            ./build-${BOARD}/partition_table/partition-table.bin > ${curdir}/built_partition.table
popd

#make -C ../../../environments/micropython//ports/esp32 -j4 V=0 MICROPY_PY_BTREE=0 MICROPY_PY_FFI=0 MICROPY_PY_USSL=0 MICROPY_PY_AXTLS=0 MICROPY_FATFS=0 MICROPY_PY_THREAD=0 CFLAGS_EXTRA="-DMODULE_UPYWRAPTEST_ENABLED=1" BUILD=build-usercmod UPYWRAP_BUILD_CPPMODULE=1 UPYWRAP_PORT_DIR=../../../environments/micropython//ports/esp32 all