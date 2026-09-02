cmake_minimum_required(VERSION 3.28)
project(bittorent_client)

set(CMAKE_CXX_STANDARD 26)

# CMakeLists.txt
add_executable(torrent_test
        test/torentparser_test.cc
        core/TorenParser.cc
        core/utils.cc

)
target_include_directories(torrent_test PRIVATE includes)

find_package(cryptopp QUIET)
if(cryptopp_FOUND)
    target_link_libraries(torrent_test PRIVATE cryptopp::cryptopp)
else()
    # fallback: link by name
    target_link_libraries(torrent_test PRIVATE cryptopp)
endif()

find_package(fmt QUIET)
if(fmt_FOUND)
    target_link_libraries(torrent_test PRIVATE fmt::fmt)
else()
    target_link_libraries(torrent_test PRIVATE fmt)
endif()