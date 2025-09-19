# root CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(AuroraCreateUserTask)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -luuid")

set(robot_name "gr2t2v2")
set(CMAKE_INSTALL_PREFIX "/usr/local/Aurora") ## path to Aurora installation
add_subdirectory(simple_loco_task)
add_subdirectory(movejoint_task)
add_subdirectory(dds_comm_task)


# install runner config
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/config/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/config/sys_config/${robot_name}
        FILES_MATCHING
        PATTERN "*.yaml"
)

