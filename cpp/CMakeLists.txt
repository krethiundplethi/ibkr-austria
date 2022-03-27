cmake_minimum_required (VERSION 3.0)

project (ibkr)

include_directories(source)

set(CMAKE_BUILD_TYPE Debug)


add_executable(ibkr ibkr.cpp source/currency.cpp source/depot.cpp source/tranche.cpp source/security.cpp)
