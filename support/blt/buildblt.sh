#!/bin/bash

cd blt2.5
./configure --prefix=$SBSEPICS/tcl/blt --with-tcl=/usr/lib64
make
make install

