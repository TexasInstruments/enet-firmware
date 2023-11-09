#!/bin/bash

Me=$0

if [ "$#" -lt 1 ]; then
    echo "Must provide file name"
    echo " $Me FILENAME"
    exit 1
else
    filename=$1
fi

if ! [ -f $filename ]; then
    echo "$filename doesn't exist"
    exit 1
fi

sed -i -e 's/\buint8_t\b/uint8/g' $filename
sed -i -e 's/\buint16_t\b/uint16/g' $filename
sed -i -e 's/\buint32_t\b/uint32/g' $filename
sed -i -e 's/\buint64_t\b/uint64/g' $filename

sed -i -e 's/\bint8_t\b/sint8/g' $filename
sed -i -e 's/\bint16_t\b/sint16/g' $filename
sed -i -e 's/\bint32_t\b/sint32/g' $filename
sed -i -e 's/\bint64_t\b/sint64/g' $filename

sed -i -e 's/\bbool\b/boolean/g' $filename

sed -i -e 's/\bETHREMOTECFG_H_\b/ETH_RPC_H_/g' $filename
sed -i -e 's/\bstdint\b/Std_Types/g' $filename

echo "Done"
