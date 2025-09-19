file(GLOB_RECURSE DDS_COMM_TASK_SOURCE_FILES 
"${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" 
"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

find_package(aurora REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(fourier_hardware REQUIRED)
find_package(grx_sot REQUIRED)
find_package(fourier_dds REQUIRED)

add_library(runner_task_DdsCommTask SHARED
    ${DDS_COMM_TASK_SOURCE_FILES}
)

target_include_directories(runner_task_DdsCommTask
PUBLIC 
${CMAKE_CURRENT_SOURCE_DIR}/include/
${aurora_INCLUDE_DIRS}
${EIGEN3_INCLUDE_DIRS}
${fourier_hardware_INCLUDE_DIRS}
${grx_sot_INCLUDE_DIRS}
${fourier_dds_INCLUDE_DIRS}
)
target_link_directories(runner_task_DdsCommTask 
PUBLIC 
    ${fourier_dds_LIBRARY_DIRS}
)

file(GLOB FOURIER_DDS_LIBS "${fourier_dds_LIBRARY_DIRS}/*.so")
target_link_libraries(runner_task_DdsCommTask 
PUBLIC 
    ${aurora_LIBRARIES}  
    ${FOURIER_DDS_LIBS}

)


install(TARGETS runner_task_DdsCommTask
        LIBRARY DESTINATION  ${CMAKE_INSTALL_PREFIX}/lib/user_task_lib/${robot_name})
