#!/usr/bin/bash

set -xe

clang galaga.c glad.c -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -g -o hello
./hello

