#!/bin/sh
set -x
set -e

make clean
rm -rf ./pack_space

make -j$(nproc) wasm

FILES=$(echo package.json libvcdparse_api.js build/libvcdparse*)

for s in $FILES; do
    d=./pack_space/$s
    mkdir -p $(dirname $d)
    cp $s $d
done

tar -czf vcd_parser_plus.tar.gz $FILES

cd ./pack_space
yarn pack