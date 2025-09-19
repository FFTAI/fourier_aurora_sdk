file(GLOB_RECURSE SIMPLE_LOCO_TASK_SOURCE_FILES 
"${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" 
"${CMAKE_CURRENT_SOURCE_DIR}/*.c" 
"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
# aurora
find_package(aurora REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(fourier_hardware REQUIRED)
find_package(grx_sot REQUIRED)
find_package(fourier_dds REQUIRED)

# libtorch use as needed
set(Torch_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/libtorch/share/cmake/Torch")
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/libtorch")
find_package(Torch REQUIRED)
set(Torch_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/libtorch/include")
set(Torch_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/libtorch/lib")

# ---------------------------------------
# library
add_library(runner_task_SimpleLocoTask SHARED
    ${SIMPLE_LOCO_TASK_SOURCE_FILES}
)

add_library(GaitGenerator SHARED IMPORTED)
set_target_properties(GaitGenerator PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/libGaitGenerator.so"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/simple_loco_task/include/"
)

# include directories
target_include_directories(runner_task_SimpleLocoTask 
PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/simple_loco_task/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/
    ${aurora_INCLUDE_DIRS}
    ${TORCH_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIRS}
    ${fourier_hardware_INCLUDE_DIRS}
    ${grx_sot_INCLUDE_DIRS}
    ${fourier_dds_INCLUDE_DIRS}
)

# link libraries
target_link_libraries(runner_task_SimpleLocoTask
PUBLIC 
    yaml-cpp 
    ${aurora_LIBRARIES}  
    ${fourier_hardware_LIBRARIES}
    ${grx_sot_LIBRARIES}
    fastdds
    GaitGenerator
)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lib/libGaitGenerator.so
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/user_task_lib/
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

# install library
install(TARGETS runner_task_SimpleLocoTask
        LIBRARY DESTINATION  ${CMAKE_INSTALL_PREFIX}/lib/user_task_lib/${robot_name})
        

# install task config
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/config/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/config/sys_config/${robot_name}/SimpleLocoTask
        FILES_MATCHING
        PATTERN "*.yaml"
        PATTERN "*.pt"
)

