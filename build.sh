#!/bin/bash -eu

# source dir is where this scriptis located
DIR_SRC=$(dirname $0)

# build dir will be a subdirectory of the current directory, where this script
# is invoked in.
BUILD_SRC=$(pwd)/build

if [[ ! -e ${BUILD_SRC} ]]; then
    mkdir -p ${BUILD_SRC}
    (
        cd ${BUILD_SRC}
        cmake -G Ninja ../${DIR_SRC}
    )
fi

# build project
(
    cd ${BUILD_SRC}
    ninja
)
