#!/bin/bash

mkdir -p ../build
pushd ../build
gcc -Wall -Werror ../code/sdl_handmade.cpp -o handmadehero -g `pkg-config --cflags --libs sdl3`
popd



