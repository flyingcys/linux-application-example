# 最小版本要求
cmake_minimum_required(VERSION 3.10)

# 项目信息
project(helix_player LANGUAGES C)

# 设置C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 强制使用纯C实现，禁用所有汇编优化
add_definitions(-DNO_ASSEMBLY)

# 源文件列表
set(SRC_FILES
    libhelix-mp3/testwrap/debug.c
    libhelix-mp3/mp3dec.c
    libhelix-mp3/mp3tabs.c
    libhelix-mp3/real/bitstream.c
    libhelix-mp3/real/buffers.c
    libhelix-mp3/real/dct32.c
    libhelix-mp3/real/dequant.c
    libhelix-mp3/real/dqchan.c
    libhelix-mp3/real/huffman.c
    libhelix-mp3/real/hufftabs.c
    libhelix-mp3/real/imdct.c
    libhelix-mp3/real/polyphase.c
    libhelix-mp3/real/scalfact.c
    libhelix-mp3/real/stproc.c
    libhelix-mp3/real/subband.c
    libhelix-mp3/real/trigtabs.c

    helix_player.c
    alsa.c
)

set(INC 
    libhelix-mp3/pub
)
# 头文件目录
include_directories(${INC})

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SRC_FILES})

# 查找ALSA库
find_package(ALSA REQUIRED)

message(STATUS "ALSA_LIBRARIES: ${ALSA_LIBRARIES}")
# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${ALSA_LIBRARIES}
)

# 安装规则
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# 交叉编译支持
# 使用方法: cmake -DCMAKE_TOOLCHAIN_FILE=<工具链文件路径> ..
if(CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
endif()