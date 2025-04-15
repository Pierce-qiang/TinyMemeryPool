cmake_minimum_required(VERSION 3.10)
project(memory_pool)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
add_compile_options(-Wall -O3)


# # 测试内存泄漏
# SET(CMAKE_BUILD_TYPE Debug)
# SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb -Wno-attributes -Wno-unused")
# SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -Wno-attributes")




# 查找pthread库
find_package(Threads REQUIRED)
# find_package(mimalloc REQUIRED)
# set_target_properties(mimalloc PROPERTIES MI_OVERRIDE "OFF") # 关闭mimalloc的override

# 设置目录
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_SOURCE_DIR}/include)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests)

# 源文件
file(GLOB SOURCES "${SRC_DIR}/*.cpp")

# 添加头文件目录
include_directories(${INC_DIR})

# 创建单元测试可执行文件
add_executable(unit_test 
    ${SOURCES}
    ${TEST_DIR}/UnitTest.cpp
)

# 创建性能测试可执行文件
add_executable(perf_test
    ${SOURCES}
    ${TEST_DIR}/PerformanceTest.cpp
)

# 链接pthread库
target_link_libraries(unit_test PRIVATE Threads::Threads)
target_link_libraries(perf_test PRIVATE Threads::Threads)
# target_link_libraries(unit_test PRIVATE mimalloc)
# target_link_libraries(perf_test PRIVATE mimalloc)

# 添加测试命令
add_custom_target(test
    COMMAND ./unit_test
    DEPENDS unit_test
)

add_custom_target(perf
    COMMAND ./perf_test
    DEPENDS perf_test
)