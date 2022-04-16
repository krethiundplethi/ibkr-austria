cmake_minimum_required (VERSION 3.0)

project (ibkr)

include_directories(source include)

set(CMAKE_BUILD_TYPE Debug)


add_executable(ibkr ibkr.cpp source/ibkr_parser.cpp source/currency.cpp source/ledger.cpp source/tranche.cpp source/security.cpp)
