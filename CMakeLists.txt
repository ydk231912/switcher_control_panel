# 指定camke最低版本
cmake_minimum_required(VERSION 3.12)
# 指定项目名称
project(switcher_keyboard_demmo_cc)
# 设置编译器
#set(CMAKE_C_COMPILER gcc)   
#set(CMAKE_CXX_COMPILER g++)
# 设置C++版本
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE debug)

# 获取项目中涉及的源文件
#aux_source_directory(ThreadPool_cpp SRC_ThreadPool)  
#aux_source_directory(Server SRC_Server)

# 导入库
#include_directories(ThreadPool_cpp)
#include_directories(Server)

# 寻找系统安装的pthread库
#find_package(Threads REQUIRED)   

# 生成可执行文件
#add_executable(TCPServer ${SRC_ThreadPool} ${SRC_Server})

# 链接到可执行文件中
#target_link_libraries(TCPServer Threads::Threads ws2_32) 

add_subdirectory(src)
