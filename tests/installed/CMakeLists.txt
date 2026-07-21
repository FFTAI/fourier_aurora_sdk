cmake_minimum_required(VERSION 3.16)
project(fourier_aurora_sdk_installed_consumer LANGUAGES CXX)

if(NOT DEFINED FOURIER_AURORA_SDK_PREFIX)
    message(FATAL_ERROR "FOURIER_AURORA_SDK_PREFIX is required")
endif()

find_package(fourier_aurora_sdk 2.0 CONFIG REQUIRED
    PATHS "${FOURIER_AURORA_SDK_PREFIX}/lib/cmake/fourier_aurora_sdk"
    NO_DEFAULT_PATH
)

add_executable(installed_consumer consumer.cpp)
target_link_libraries(
    installed_consumer
    PRIVATE fourier_aurora_sdk::fourier_aurora_sdk
)
target_compile_features(installed_consumer PRIVATE cxx_std_17)
