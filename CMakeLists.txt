cmake_minimum_required(VERSION 3.15)

project(rkllseek LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    src/rkllseek.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/../rknn-llm/rkllm-runtime/Linux/librkllm_api/include)

set(LIBRKLLM ${CMAKE_SOURCE_DIR}/../rknn-llm//rkllm-runtime/Linux/librkllm_api/aarch64/librkllmrt.so)

target_link_libraries(${PROJECT_NAME}
    ${LIBRKLLM}
    readline)

set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)

install(TARGETS ${PROJECT_NAME})
install(PROGRAMS ${LIBRKLLM} DESTINATION lib)
