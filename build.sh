#!/bin/bash
docker run --rm \
    -v "$(pwd)":/project \
    -w /project \
    ghcr.io/dragonminded/libdragon:latest \
    sh -c 'make -C libdragon install tools-install && make -C tiny3d clean && make -C tiny3d CFLAGS="-Wno-error=all" CXXFLAGS="-Wno-error=all" && make "$@"'
