#!/bin/bash -eu

# source dir is where this scriptis located
DIR_SRC=$(dirname $0)
PICO_SRC=${DIR_SRC}/picotcp
PICO_BSD=${DIR_SRC}/picotcp-bsd

if [[ ! -e ${PICO_SRC} ]]; then
  echo "Please add picotcp as a submodule...!"
  exit 1
else
  cd ${PICO_SRC}
  make clean
  make TAP=1
  cd ..
fi

if [[ ! -e ${PICO_BSD} ]]; then
   echo "Please add picotcp-bsd as a submodule...!"
   exit 1
else
   cd ${PICO_BSD} 
   make clean
   make
   cd ..
fi

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
