cmake_minimum_required(VERSION 3.16)
project(fourier_aurora_sdk VERSION 2.0.0 LANGUAGES CXX)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/fftai/fourier_aurora_sdk" CACHE PATH
        "Default install prefix for fourier_aurora_sdk" FORCE)
endif()

option(FOURIER_AURORA_SDK_BUILD_PYTHON "Build Python bindings" ON)
set(FOURIER_AURORA_SDK_BUILD_TESTS_DEFAULT OFF)
if(SKBUILD)
    set(FOURIER_AURORA_SDK_BUILD_EXAMPLES_DEFAULT OFF)
else()
    set(FOURIER_AURORA_SDK_BUILD_EXAMPLES_DEFAULT ON)
endif()
option(FOURIER_AURORA_SDK_BUILD_TESTS "Build tests" ${FOURIER_AURORA_SDK_BUILD_TESTS_DEFAULT})
option(FOURIER_AURORA_SDK_BUILD_EXAMPLES "Build C++ examples" ${FOURIER_AURORA_SDK_BUILD_EXAMPLES_DEFAULT})
option(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT "Build the fourierdds transport" ON)
set(FOURIER_AURORA_SDK_PACKAGE_COMMIT "" CACHE STRING
    "Optional Git commit embedded in package artifact versions")
if(FOURIER_AURORA_SDK_PACKAGE_COMMIT)
    string(LENGTH "${FOURIER_AURORA_SDK_PACKAGE_COMMIT}" _sdk_commit_length)
    if(NOT FOURIER_AURORA_SDK_PACKAGE_COMMIT MATCHES "^[0-9a-fA-F]+$" OR
       NOT _sdk_commit_length EQUAL 8)
        message(FATAL_ERROR
            "FOURIER_AURORA_SDK_PACKAGE_COMMIT must be an eight-character Git commit")
    endif()
endif()
set(FOURIER_AURORA_SDK_PYTHON_PACKAGE_DIR "fourier_aurora_client" CACHE STRING
    "Python package destination used by wheel builds")
set(FOURIERDDS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../fourierdds" CACHE PATH
    "Optional fourierdds source checkout used before installed headers")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(fourier_aurora_sdk SHARED
    src/aurora_client.cpp
    src/connection_options.cpp
    src/transport/mock_transport.cpp
)
set_target_properties(fourier_aurora_sdk PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    VERSION ${PROJECT_VERSION}
    SOVERSION 2
    INSTALL_RPATH "$ORIGIN;/opt/fftai/fourier_dds_msgs/lib;/usr/local/lib"
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(fourier_aurora_sdk PRIVATE "-Wl,--disable-new-dtags")
endif()

add_library(fourier_aurora_sdk::fourier_aurora_sdk ALIAS fourier_aurora_sdk)

target_include_directories(fourier_aurora_sdk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT)
    find_package(fourier_dds REQUIRED)
    target_sources(fourier_aurora_sdk PRIVATE src/transport/fourierdds_transport.cpp)
    target_compile_definitions(fourier_aurora_sdk PUBLIC FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT)
    if(EXISTS "${FOURIERDDS_SOURCE_DIR}/include/fourier_dds/dds_client.hpp")
        target_include_directories(fourier_aurora_sdk PRIVATE "${FOURIERDDS_SOURCE_DIR}/include")
    endif()
    target_include_directories(fourier_aurora_sdk PUBLIC ${fourier_dds_INCLUDE_DIRS})
    target_link_directories(fourier_aurora_sdk PUBLIC /usr/local/lib ${fourier_dds_LIBRARY_DIRS})
    target_link_libraries(fourier_aurora_sdk PUBLIC
        AuroraState
        RobotMotionState
        RobotControlGroupState
        EstimatorData
        ErrorCodes
        RobotMotionCmd
        RobotControlGroupCmd
        SubControllerCmd
        SetInt
        TracerHeader
        Header
        Time
        UUID
        fastcdr
        fastdds
    )
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/fourier_aurora_sdk.conf.in"
    "${CMAKE_CURRENT_BINARY_DIR}/packaging/fourier_aurora_sdk.conf"
    @ONLY
)

if(NOT SKBUILD)
    install(TARGETS fourier_aurora_sdk
        EXPORT fourier_aurora_sdkTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/fourier_aurora_sdkConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fourier_aurora_sdkConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/fourier_aurora_sdkConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fourier_aurora_sdk
    )

    install(EXPORT fourier_aurora_sdkTargets
        NAMESPACE fourier_aurora_sdk::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fourier_aurora_sdk
    )
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/fourier_aurora_sdkConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/fourier_aurora_sdkConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fourier_aurora_sdk
    )
    install(FILES examples/cpp/installed_smoke.cpp
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples/cpp
    )
    install(DIRECTORY examples/cpp/usage
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples/cpp
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/packaging/fourier_aurora_sdk.conf"
        DESTINATION /etc/ld.so.conf.d
    )
endif()

if(FOURIER_AURORA_SDK_BUILD_PYTHON)
    set(PYBIND11_FINDPYTHON ON)
    find_package(pybind11 QUIET)
    if(pybind11_FOUND)
        pybind11_add_module(_fourier_aurora_sdk
            python/bindings/module.cpp
            python/bindings/bind_client.cpp
            python/bindings/bind_types.cpp
        )
        target_link_libraries(_fourier_aurora_sdk PRIVATE fourier_aurora_sdk)
        set_target_properties(_fourier_aurora_sdk PROPERTIES
            INSTALL_RPATH "$ORIGIN"
        )
        install(TARGETS _fourier_aurora_sdk fourier_aurora_sdk
            LIBRARY DESTINATION ${FOURIER_AURORA_SDK_PYTHON_PACKAGE_DIR}
            RUNTIME DESTINATION ${FOURIER_AURORA_SDK_PYTHON_PACKAGE_DIR}
        )
    else()
        message(WARNING "pybind11 not found; Python bindings are skipped")
    endif()
endif()

if(FOURIER_AURORA_SDK_BUILD_TESTS)
    enable_testing()
    add_executable(test_aurora_client tests/cpp/test_aurora_client.cpp)
    target_link_libraries(test_aurora_client PRIVATE fourier_aurora_sdk)
    add_test(NAME aurora_client COMMAND test_aurora_client)
endif()

if(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT AND FOURIER_AURORA_SDK_BUILD_EXAMPLES)
    foreach(example_name get_info set_state set_motion set_arm_swing state_client motion_client control_group_client)
        add_executable(${example_name} examples/cpp/usage/${example_name}.cpp)
        target_include_directories(${example_name} PRIVATE examples/cpp/usage)
        target_link_libraries(${example_name} PRIVATE fourier_aurora_sdk)
    endforeach()
    if(NOT SKBUILD)
        install(TARGETS get_info set_state set_motion set_arm_swing state_client motion_client control_group_client
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/fourier_aurora_sdk_examples
        )
    endif()
endif()

if(NOT SKBUILD)
    set(CPACK_PACKAGE_NAME "fourier-aurora-sdk")
    set(CPACK_PACKAGE_VENDOR "Fourier Intelligence")
    set(CPACK_PACKAGE_CONTACT "support@fftai.com")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    if(FOURIER_AURORA_SDK_PACKAGE_COMMIT)
        string(TOLOWER "${FOURIER_AURORA_SDK_PACKAGE_COMMIT}" _sdk_package_commit)
        set(CPACK_DEBIAN_PACKAGE_VERSION
            "${PROJECT_VERSION}+g${_sdk_package_commit}")
    endif()
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Fourier Intelligence")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/fftai/fourier_aurora_sdk")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "fourier_dds (>= 1.2.0), fourier-dds-msgs (>= 0.1.0)")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/postinst"
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/postrm"
    )
    include(CPack)
endif()
