cmake_minimum_required(VERSION 3.16)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" FOURIER_AURORA_SDK_VERSION)
string(STRIP "${FOURIER_AURORA_SDK_VERSION}" FOURIER_AURORA_SDK_VERSION)
if(NOT FOURIER_AURORA_SDK_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR
        "VERSION must contain one major.minor.patch semantic version")
endif()

project(
    fourier_aurora_sdk
    VERSION ${FOURIER_AURORA_SDK_VERSION}
    LANGUAGES CXX
)

if(SKBUILD AND DEFINED SKBUILD_PROJECT_VERSION AND
   NOT SKBUILD_PROJECT_VERSION STREQUAL PROJECT_VERSION)
    message(FATAL_ERROR
        "Python metadata version ${SKBUILD_PROJECT_VERSION} does not match "
        "CMake VERSION ${PROJECT_VERSION}")
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/fftai/fourier_aurora_sdk" CACHE PATH
        "Default install prefix for fourier_aurora_sdk" FORCE)
endif()

option(FOURIER_AURORA_SDK_BUILD_PYTHON "Build Python bindings" ON)
set(FOURIER_AURORA_SDK_BUILD_TESTS_DEFAULT OFF)
if(SKBUILD)
    set(FOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS_DEFAULT OFF)
    set(FOURIER_AURORA_SDK_BUILD_EXAMPLES_DEFAULT OFF)
else()
    set(FOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS_DEFAULT ON)
    set(FOURIER_AURORA_SDK_BUILD_EXAMPLES_DEFAULT ON)
endif()
option(FOURIER_AURORA_SDK_BUILD_TESTS "Build tests" ${FOURIER_AURORA_SDK_BUILD_TESTS_DEFAULT})
option(FOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS
    "Build robot-independent C++ diagnostic clients"
    ${FOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS_DEFAULT})
option(FOURIER_AURORA_SDK_INSTALL_SYSTEM_CLIENT
    "Expose aurora-client and completions in system paths"
    OFF)
option(FOURIER_AURORA_SDK_BUILD_EXAMPLES
    "Build robot-specific C++ recipes"
    ${FOURIER_AURORA_SDK_BUILD_EXAMPLES_DEFAULT})
option(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT "Build the fourierdds transport" ON)
if(SKBUILD AND NOT FOURIER_AURORA_SDK_BUILD_PYTHON)
    message(FATAL_ERROR "Wheel builds require Python bindings")
endif()
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
set(FOURIER_AURORA_SDK_DDS_PREFIX "/opt/fftai" CACHE PATH
    "Prefix containing the pinned FourierDDS and generated message packages")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(FOURIER_AURORA_SDK_SOVERSION 3)

add_library(fourier_aurora_sdk SHARED
    src/aurora_client.cpp
    src/connection_options.cpp
    src/transport/fourierdds_transport.cpp
    src/transport/teardown_coordinator.cpp
)
set_target_properties(fourier_aurora_sdk PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    VERSION ${PROJECT_VERSION}
    SOVERSION ${FOURIER_AURORA_SDK_SOVERSION}
    INSTALL_RPATH "$ORIGIN;/opt/fftai/fourier_dds_msgs/lib;/usr/local/lib"
    INSTALL_RPATH_USE_LINK_PATH FALSE
)
if(SKBUILD)
    set_property(
        TARGET fourier_aurora_sdk
        PROPERTY BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

add_library(fourier_aurora_sdk::fourier_aurora_sdk ALIAS fourier_aurora_sdk)

target_include_directories(fourier_aurora_sdk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT)
    find_package(fourier_dds 1.2.0 EXACT CONFIG REQUIRED
        PATHS "${FOURIER_AURORA_SDK_DDS_PREFIX}/lib/cmake/fourier_dds"
        NO_DEFAULT_PATH
    )

    set(_fourier_aurora_sdk_message_root
        "${FOURIER_AURORA_SDK_DDS_PREFIX}/fourier_dds_msgs")
    find_path(FOURIER_AURORA_SDK_MESSAGE_INCLUDE_DIR
        NAMES AuroraState/AuroraState.hpp
        PATHS "${_fourier_aurora_sdk_message_root}/include/fourier_dds_msgs"
        NO_DEFAULT_PATH
    )
    if(NOT FOURIER_AURORA_SDK_MESSAGE_INCLUDE_DIR)
        message(FATAL_ERROR
            "Generated FourierDDS message headers were not found under "
            "${_fourier_aurora_sdk_message_root}")
    endif()

    set(_fourier_aurora_sdk_message_names
        AuroraState
        RobotMotionState
        RobotControlGroupState
        EstimatorData
        ErrorCodes
        RobotMotionCmd
        RobotControlGroupCmd
        MotorCfgCmd
        MotorCfgState
        SubControllerCmd
        SubControllerService
        LeaseInfo
        ManageLease
        SetInt
        TracerHeader
        Header
        Time
        UUID
    )
    set(_fourier_aurora_sdk_message_libraries)
    foreach(_message_name IN LISTS _fourier_aurora_sdk_message_names)
        find_library(_message_library_${_message_name}
            NAMES "${_message_name}"
            PATHS "${_fourier_aurora_sdk_message_root}/lib"
            NO_DEFAULT_PATH
        )
        if(NOT _message_library_${_message_name})
            message(FATAL_ERROR
                "Generated FourierDDS message library ${_message_name} was not found under "
                "${_fourier_aurora_sdk_message_root}/lib")
        endif()
        list(APPEND _fourier_aurora_sdk_message_libraries
            "${_message_library_${_message_name}}")
    endforeach()

    target_compile_definitions(fourier_aurora_sdk PRIVATE
        FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT
    )
    target_include_directories(fourier_aurora_sdk PRIVATE
        "${FOURIER_AURORA_SDK_MESSAGE_INCLUDE_DIR}"
    )
    target_link_libraries(fourier_aurora_sdk PRIVATE
        fourier_dds::fourierdds_core
        ${_fourier_aurora_sdk_message_libraries}
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
    install(FILES
        LICENSE
        README.md
        README_CN.md
        CHANGELOG
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
    )
    install(FILES
        docs/overview.md
        docs/overview_CN.md
        docs/cpp_api.md
        docs/cpp_api_CN.md
        docs/python_api.md
        docs/python_api_CN.md
        docs/examples_and_clients.md
        docs/examples_and_clients_CN.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/docs
    )

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
    if(FOURIER_AURORA_SDK_BUILD_EXAMPLES)
        install(DIRECTORY examples/gr3
            DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
        )
    endif()
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/packaging/fourier_aurora_sdk.conf"
        DESTINATION /etc/ld.so.conf.d
    )
endif()

if(FOURIER_AURORA_SDK_BUILD_PYTHON)
    find_package(Python COMPONENTS Interpreter Development REQUIRED)
    set(_fourier_aurora_sdk_python "${Python_EXECUTABLE}")
    execute_process(
        COMMAND
            "${_fourier_aurora_sdk_python}" -m pybind11 --cmakedir
        RESULT_VARIABLE _fourier_aurora_sdk_pybind11_result
        OUTPUT_VARIABLE _fourier_aurora_sdk_pybind11_dir
        ERROR_VARIABLE _fourier_aurora_sdk_pybind11_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT _fourier_aurora_sdk_pybind11_result EQUAL 0 OR
       NOT EXISTS
           "${_fourier_aurora_sdk_pybind11_dir}/pybind11Config.cmake")
        message(FATAL_ERROR
            "Python-enabled builds require pybind11 in the active interpreter "
            "(${_fourier_aurora_sdk_python}): "
            "${_fourier_aurora_sdk_pybind11_error}")
    endif()
    set(PYBIND11_FINDPYTHON ON)
    find_package(pybind11 2.13.6 CONFIG REQUIRED
        PATHS "${_fourier_aurora_sdk_pybind11_dir}"
        NO_DEFAULT_PATH
    )
    pybind11_add_module(_fourier_aurora_sdk
        python/bindings/module.cpp
        python/bindings/bind_client.cpp
        python/bindings/bind_types.cpp
    )
    target_link_libraries(_fourier_aurora_sdk PRIVATE fourier_aurora_sdk)
    set_target_properties(_fourier_aurora_sdk PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        INSTALL_RPATH_USE_LINK_PATH FALSE
    )
    if(SKBUILD)
        install(TARGETS _fourier_aurora_sdk
            LIBRARY DESTINATION ${FOURIER_AURORA_SDK_PYTHON_PACKAGE_DIR}
            RUNTIME DESTINATION ${FOURIER_AURORA_SDK_PYTHON_PACKAGE_DIR}
        )
        install(FILES "$<TARGET_FILE:fourier_aurora_sdk>"
            DESTINATION ${FOURIER_AURORA_SDK_PYTHON_PACKAGE_DIR}
            RENAME
                "libfourier_aurora_sdk${CMAKE_SHARED_LIBRARY_SUFFIX}.${FOURIER_AURORA_SDK_SOVERSION}"
        )
    endif()
endif()

if(FOURIER_AURORA_SDK_BUILD_TESTS)
    enable_testing()
    target_compile_definitions(
        fourier_aurora_sdk
        PRIVATE FOURIER_AURORA_SDK_TESTING
    )

    add_library(aurora_sdk_test_support STATIC
        tests/support/mock_transport.cpp
    )
    set_target_properties(
        aurora_sdk_test_support
        PROPERTIES POSITION_INDEPENDENT_CODE ON
    )
    target_compile_definitions(
        aurora_sdk_test_support
        PUBLIC FOURIER_AURORA_SDK_TESTING
    )
    target_include_directories(
        aurora_sdk_test_support
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/support
    )
    target_link_libraries(
        aurora_sdk_test_support
        PUBLIC fourier_aurora_sdk
    )

    if(TARGET _fourier_aurora_sdk)
        target_sources(
            _fourier_aurora_sdk
            PRIVATE tests/python/bind_test_fixture.cpp
        )
        target_compile_definitions(
            _fourier_aurora_sdk
            PRIVATE FOURIER_AURORA_SDK_TESTING
        )
        target_include_directories(
            _fourier_aurora_sdk
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/support
        )
        target_link_libraries(
            _fourier_aurora_sdk
            PRIVATE aurora_sdk_test_support
        )

        set(_aurora_sdk_test_python_package
            "${CMAKE_CURRENT_BINARY_DIR}/python/fourier_aurora_client")
        file(MAKE_DIRECTORY "${_aurora_sdk_test_python_package}")
        configure_file(
            python/fourier_aurora_client/__init__.py
            "${_aurora_sdk_test_python_package}/__init__.py"
            COPYONLY
        )
        configure_file(
            python/fourier_aurora_client/client.py
            "${_aurora_sdk_test_python_package}/client.py"
            COPYONLY
        )
        set_target_properties(
            _fourier_aurora_sdk
            PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY
                    "${_aurora_sdk_test_python_package}"
        )
    endif()

    function(add_aurora_sdk_test test_name source)
        add_executable(${test_name} ${source})
        target_compile_definitions(
            ${test_name}
            PRIVATE FOURIER_AURORA_SDK_TESTING
        )
        target_include_directories(
            ${test_name}
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/support
        )
        target_link_libraries(
            ${test_name}
            PRIVATE aurora_sdk_test_support
        )
        add_test(NAME ${test_name} COMMAND ${test_name})
        set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
    endfunction()

    add_aurora_sdk_test(
        client_contract
        tests/cpp/test_client_contract.cpp
    )
    add_aurora_sdk_test(
        client_concurrency
        tests/cpp/test_client_concurrency.cpp
    )
    add_aurora_sdk_test(
        lease_contract
        tests/cpp/test_lease_contract.cpp
    )
    add_aurora_sdk_test(
        teardown_coordinator
        tests/cpp/test_teardown_coordinator.cpp
    )
    add_aurora_sdk_test(
        destructor_fallback
        tests/cpp/test_destructor_fallback.cpp
    )

    if(NOT FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT)
        add_aurora_sdk_test(
            dds_disabled
            tests/cpp/test_dds_disabled.cpp
        )
    else()
        add_aurora_sdk_test(
            dds_transport_loopback
            tests/dds/test_dds_transport_loopback.cpp
        )
        target_include_directories(
            dds_transport_loopback
            PRIVATE "${FOURIER_AURORA_SDK_MESSAGE_INCLUDE_DIR}"
        )
        target_link_libraries(
            dds_transport_loopback
            PRIVATE
                fourier_dds::fourierdds_core
                ${_fourier_aurora_sdk_message_libraries}
        )
        set_tests_properties(
            dds_transport_loopback
            PROPERTIES TIMEOUT 40
        )

        add_executable(
            dds_wheel_peer
            tests/dds/dds_wheel_peer.cpp
        )
        target_include_directories(
            dds_wheel_peer
            PRIVATE "${FOURIER_AURORA_SDK_MESSAGE_INCLUDE_DIR}"
        )
        target_link_libraries(
            dds_wheel_peer
            PRIVATE
                fourier_dds::fourierdds_core
                ${_fourier_aurora_sdk_message_libraries}
        )
    endif()
endif()

if(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT AND
   FOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS)
    add_executable(aurora_client examples/client/aurora_client.cpp)
    set_target_properties(aurora_client PROPERTIES OUTPUT_NAME aurora-client)
    target_compile_definitions(aurora_client PRIVATE
        FOURIER_AURORA_SDK_VERSION="${PROJECT_VERSION}"
    )
    target_include_directories(aurora_client PRIVATE examples/client)
    target_link_libraries(aurora_client PRIVATE fourier_aurora_sdk)
    if(NOT SKBUILD)
        install(TARGETS aurora_client
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
        if(FOURIER_AURORA_SDK_INSTALL_SYSTEM_CLIENT)
            file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/packaging")
            file(CREATE_LINK
                "/opt/fftai/fourier_aurora_sdk/bin/aurora-client"
                "${CMAKE_CURRENT_BINARY_DIR}/packaging/aurora-client"
                SYMBOLIC
            )
            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/packaging/aurora-client"
                DESTINATION /usr/bin
            )
            set(_aurora_client_completion_root "/usr/share")
        else()
            set(_aurora_client_completion_root
                "${CMAKE_INSTALL_DATAROOTDIR}")
        endif()
        install(FILES completion/aurora-client.bash
            DESTINATION
                "${_aurora_client_completion_root}/bash-completion/completions"
            RENAME aurora-client
        )
        install(FILES completion/_aurora-client
            DESTINATION "${_aurora_client_completion_root}/zsh/site-functions"
        )
        install(FILES completion/aurora-client.fish
            DESTINATION
                "${_aurora_client_completion_root}/fish/vendor_completions.d"
        )
    endif()
    if(FOURIER_AURORA_SDK_BUILD_TESTS)
        find_program(FOURIER_AURORA_SDK_BASH_EXECUTABLE bash)
        if(FOURIER_AURORA_SDK_BASH_EXECUTABLE)
            add_test(
                NAME unified_client_cli
                COMMAND
                    "${FOURIER_AURORA_SDK_BASH_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/client/test_unified_client_cli.sh"
                    "$<TARGET_FILE:aurora_client>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/completion"
            )
            set_tests_properties(unified_client_cli PROPERTIES TIMEOUT 15)
        endif()
    endif()
endif()

if(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT AND FOURIER_AURORA_SDK_BUILD_EXAMPLES)
    foreach(recipe_name get_info walk_forward swing_arms aurora_state_callback)
        add_executable(
            gr3_${recipe_name}
            examples/gr3/cpp/${recipe_name}.cpp
        )
        target_link_libraries(
            gr3_${recipe_name}
            PRIVATE fourier_aurora_sdk
        )
    endforeach()
endif()

if(NOT SKBUILD)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
        set(_aurora_sdk_debian_architecture "amd64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
        set(_aurora_sdk_debian_architecture "arm64")
    else()
        message(FATAL_ERROR
            "Debian packaging supports only amd64 and arm64, got "
            "${CMAKE_SYSTEM_PROCESSOR}")
    endif()

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
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE
        "${_aurora_sdk_debian_architecture}")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/fftai/fourier_aurora_sdk")
    set(CPACK_SET_DESTDIR ON)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "fourier_dds (= 1.2.0-1), fourier-dds-msgs (= 0.2.0-1)")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/postinst"
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/postrm"
    )
    include(CPack)
endif()
