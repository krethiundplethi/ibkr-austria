cmake_minimum_required (VERSION 4.0)

project (ibkr)

include_directories(source include)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

add_executable(ibkr ibkr.cpp 
	source/ibkr_parser.cpp source/rate_parser.cpp
	source/pnl.cpp source/pnl_forex.cpp source/pnl_equity.cpp 
	source/currency.cpp source/ledger.cpp source/tranche.cpp source/security.cpp 
)
