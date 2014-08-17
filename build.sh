#!/bin/BASH
rm -rf build
mkdir build
cd build
cmake -DUSE_OSX_HOMEBREW_CURSES=TRUE -DCMAKE_BUILD_TYPE=Debug -DENABLE_SERVER=FALSE  -DSHELLDIR=$HOME/tmp/unnh -DBINDIR=$HOME/tmp/unnh/data -DDATADIR=$HOME/tmp/unnh/data -DLIBDIR=$HOME/tmp/unnh/data ..
make
make install
cd ..
