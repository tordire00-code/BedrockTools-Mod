cmake_minimum_required(VERSION 3.21)
project(NoFallDamageMod VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Requires a LeviLauncher/LeviLamina Android SDK package that exports
# LeviLaminaConfig.cmake and the matching pl/ headers and libraries.
find_package(LeviLamina REQUIRED)

add_library(nofalldamage SHARED src/main.cpp)
target_link_libraries(nofalldamage PRIVATE LeviLamina::LeviLamina log)
