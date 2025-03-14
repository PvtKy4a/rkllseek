# Debug / Release / RelWithDebInfo
if [[ -z ${BUILD_TYPE} ]];then
    BUILD_TYPE=Release
fi

TARGET_ARCH=aarch64

SYSTEM_NAME=Linux

GCC_COMPILER_PATH=/usr/bin/aarch64-linux-gnu-

C_COMPILER=${GCC_COMPILER_PATH}gcc

CXX_COMPILER=${GCC_COMPILER_PATH}g++

STRIP_COMPILER=${GCC_COMPILER_PATH}-strip

ROOT_PWD=$( cd "$( dirname $0 )" && cd -P "$( dirname "$SOURCE" )" && pwd )

BUILD_DIR=${ROOT_PWD}/build

cmake \
    -DCMAKE_BUILD_TYPE:STRING=${BUILD_TYPE} \
    -DCMAKE_SYSTEM_PROCESSOR=${TARGET_ARCH} \
    -DCMAKE_SYSTEM_NAME=${SYSTEM_NAME} \
    -DCMAKE_C_COMPILER=${C_COMPILER} \
    -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
    --no-warn-unused-cli \
    -S${ROOT_PWD} \
    -B${BUILD_DIR} \
    -G "Unix Makefiles" \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \

cmake \
    --build ${BUILD_DIR} \
    --config ${BUILD_TYPE} \
    --target all \
    -j 10 --

cmake --install ${BUILD_DIR}