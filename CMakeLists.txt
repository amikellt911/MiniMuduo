# 1. CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 2. 项目名称
project(llt_muduo LANGUAGES CXX) # 项目名称是 llt_muduo

# 3. 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# set(CMAKE_CXX_EXTENSIONS OFF)

# 4. 添加 include 目录作为头文件根搜索路径
# 这样你在代码中就可以 #include "llt_muduo/net/TcpServer.h"
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 5. 查找所有库的源文件
# 使用 GLOB_RECURSE 递归查找 src 目录下的所有 .cpp 文件
# 注意：GLOB_RECURSE 同样有 GLOB 的缺点（增删文件需重新运行 CMake）
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
# 如果手动列出，会更长但更健壮:
# set(LIB_SOURCES
#     src/base/Logger.cpp
#     src/base/Timestamp.cpp
#     # ... base 的其他 cpp
#     src/net/Acceptor.cpp
#     src/net/Buffer.cpp
#     # ... net 的其他 cpp
# )

# 6. 创建 llt_muduo 库 (静态库)
add_library(llt_muduo STATIC ${LIB_SOURCES}) # 库目标名称 llt_muduo

# 7. 添加库所需的公共头文件目录 (现代方式)
# PUBLIC: 使用这个库(llt_muduo)和库本身都需要包含这些目录
# 因为你的 #include 是从 "llt_muduo/..." 开始的，所以需要包含 include/ 目录
target_include_directories(llt_muduo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 8. 查找并链接依赖项 (pthreads)
find_package(Threads REQUIRED)
target_link_libraries(llt_muduo PUBLIC Threads::Threads)

# --- 构建测试程序 (testserver) ---

# 9. 查找测试源文件
set(TEST_SOURCES test/testserver.cpp) # 直接指定测试文件

# 10. 创建可执行文件
add_executable(testserver ${TEST_SOURCES}) # 可执行文件目标名称 testserver

# 11. 链接可执行文件到 llt_muduo 库
# testserver 需要使用 llt_muduo 库
target_link_libraries(testserver PRIVATE llt_muduo)
# 注意: 因为 llt_muduo 库已经 PUBLIC 链接了 Threads::Threads,
# CMake 会自动处理 testserver 对线程库的间接依赖，
# 通常不需要在这里再次链接 Threads::Threads

# 可选: 添加编译选项
target_compile_options(llt_muduo PRIVATE $<$<CONFIG:Debug>:-g>)
target_compile_options(testserver PRIVATE $<$<CONFIG:Debug>:-g>)
target_compile_options(llt_muduo PRIVATE $<$<CONFIG:Release>:-O2>)
target_compile_options(testserver PRIVATE $<$<CONFIG:Release>:-O2>)

# 可选: 输出信息
message(STATUS "Configuring llt_muduo Project")
message(STATUS "Library sources: ${LIB_SOURCES}")
message(STATUS "Test sources: ${TEST_SOURCES}")

# --- End of CMakeLists.txt ---