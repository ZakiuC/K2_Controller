cmake_minimum_required(VERSION 3.10)
project(K2_Controler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 包含目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/core
    ${PROJECT_SOURCE_DIR}/include/remote
    ${PROJECT_SOURCE_DIR}/include/protocols
    ${PROJECT_SOURCE_DIR}/include/devices
    ${PROJECT_SOURCE_DIR}/include/utils
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/config
)

# 源文件
file(GLOB_RECURSE SOURCES 
    "src/core/*.cpp"
    "src/protocols/*.cpp"
    "src/devices/*.cpp"
    "src/utils/*.cpp"
)

# 主程序
add_executable(K2_Controler src/main.cpp ${SOURCES})

# 远程控制模块 (可选)
option(ENABLE_REMOTE_CONTROL "Enable remote control features" ON)
if(ENABLE_REMOTE_CONTROL)
    file(GLOB REMOTE_SOURCES "src/remote/*.cpp")
    target_sources(K2_Controler PRIVATE ${REMOTE_SOURCES})
    
    # 查找并链接 Boost 库 (WebSocket 需要)
    find_package(Boost REQUIRED COMPONENTS system)
    target_link_libraries(K2_Controler PRIVATE Boost::system)
    
    # JSON 库头文件 (nlohmann/json)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(NLOHMANN_JSON nlohmann_json)
        if(NLOHMANN_JSON_FOUND)
            target_link_libraries(K2_Controler PRIVATE ${NLOHMANN_JSON_LIBRARIES})
            target_include_directories(K2_Controler PRIVATE ${NLOHMANN_JSON_INCLUDE_DIRS})
        endif()
    endif()
    
    # 如果找不到包管理器安装的版本，尝试手动查找
    if(NOT NLOHMANN_JSON_FOUND)
        find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp
            PATHS /usr/include /usr/local/include
        )
        if(NLOHMANN_JSON_INCLUDE_DIR)
            target_include_directories(K2_Controler PRIVATE ${NLOHMANN_JSON_INCLUDE_DIR})
            message(STATUS "Found nlohmann/json at: ${NLOHMANN_JSON_INCLUDE_DIR}")
        else()
            message(WARNING "nlohmann/json not found. Please install: sudo apt-get install libnlohmann-json3-dev")
        endif()
    endif()
    
    # # 查找并链接MQTT库 (如果需要)
    # find_package(PahoMqttCpp QUIET)
    # if(PahoMqttCpp_FOUND)
    #     target_link_libraries(K2_Controler PRIVATE PahoMqttCpp)
    # endif()
endif()'

# 链接系统库
find_package(Threads REQUIRED)
target_link_libraries(K2_Controler PRIVATE Threads::Threads)

# 安装目标
install(TARGETS K2_Controler DESTINATION bin)

# 可选的测试程序
option(BUILD_TESTS "Build test programs" OFF)
if(BUILD_TESTS)
    # WebSocket 测试程序
    add_executable(websocket_test tests/websocket_test.cpp ${SOURCES} src/remote/websocket_client.cpp)
    target_include_directories(websocket_test PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/remote
        ${PROJECT_SOURCE_DIR}/include/protocols
        ${PROJECT_SOURCE_DIR}/include/devices
        ${PROJECT_SOURCE_DIR}/include/utils
        ${PROJECT_SOURCE_DIR}/config
    )
    target_link_libraries(websocket_test PRIVATE Threads::Threads)
    
    if(ENABLE_REMOTE_CONTROL)
        target_link_libraries(websocket_test PRIVATE Boost::system)
        if(NLOHMANN_JSON_FOUND)
            target_link_libraries(websocket_test PRIVATE ${NLOHMANN_JSON_LIBRARIES})
            target_include_directories(websocket_test PRIVATE ${NLOHMANN_JSON_INCLUDE_DIRS})
        endif()
    endif()
    
    set_target_properties(websocket_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
    )
endif()