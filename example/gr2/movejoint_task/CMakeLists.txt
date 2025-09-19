file(GLOB_RECURSE MOVEJOINT_TASK_SOURCE_FILES 
"${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" 
"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

find_package(aurora REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(fourier_hardware REQUIRED)
find_package(grx_sot REQUIRED)
find_package(fourier_dds REQUIRED)

add_library(runner_task_MoveJointTask SHARED
    ${MOVEJOINT_TASK_SOURCE_FILES}
)

target_include_directories(runner_task_MoveJointTask
PUBLIC 
${CMAKE_CURRENT_SOURCE_DIR}/include/
${aurora_INCLUDE_DIRS}
${EIGEN3_INCLUDE_DIRS}
${fourier_hardware_INCLUDE_DIRS}
${grx_sot_INCLUDE_DIRS}
${fourier_dds_INCLUDE_DIRS}
)
target_link_libraries(runner_task_MoveJointTask 
PUBLIC 
    ${aurora_LIBRARIES}  
)


install(TARGETS runner_task_MoveJointTask
        LIBRARY DESTINATION  ${CMAKE_INSTALL_PREFIX}/lib/user_task_lib/${robot_name})
