cmake_minimum_required(VERSION 3.0)
project(chat)

# 1. 显式设置编译类型为 Debug
set(CMAKE_BUILD_TYPE "Debug")

# 2. 配置编译选项：加入 -g (调试) 和 ASan (内存检测)
# -fno-omit-frame-pointer 会让崩溃时的堆栈信息更详细————————debug的时候添加
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -fsanitize=address -fno-omit-frame-pointer")

# 3. 配置链接选项：确保链接器也加上 ASan
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")

# 配置最终的可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)

# 加载子目录
add_subdirectory(src)