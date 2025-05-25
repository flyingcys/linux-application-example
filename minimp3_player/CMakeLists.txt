# 最小版本要求
cmake_minimum_required(VERSION 3.10)

# 项目信息
project(minimp3_player LANGUAGES C)

# 设置C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 源文件列表
set(SRC_FILES
    minimp3_player.c
    alsa.c
)

# 头文件目录
include_directories(src)

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