cmake_minimum_required(VERSION 3.28)
project(bittorent_client)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # ← add this, forces the standard
set(CMAKE_CXX_EXTENSIONS OFF)
include_directories(/home/rishik/.local/include)
link_directories(/home/rishik/.local/lib)
# CMakeLists.txt
add_executable(torrent_cli
        apps/cli/main.cc
        core/TorenParser.cc
        core/utils.cc
        core/PeerRetrieval.cc
        core/PeerQueue.cc
        core/connection.cc
        core/Message.cc
        core/PeerConnection.cc
        core/Piece.cc
        core/PieceManager.cc
        core/TorrentClient.cc

)

target_include_directories(torrent_cli PRIVATE includes)
target_link_libraries(torrent_cli PRIVATE curl)

find_package(cryptopp QUIET)
if(cryptopp_FOUND)
    target_link_libraries(torrent_cli PRIVATE cryptopp::cryptopp)
else()
    # fallback: link by name
    target_link_libraries(torrent_cli PRIVATE cryptopp)
endif()

find_package(fmt QUIET)
if(fmt_FOUND)
    target_link_libraries(torrent_cli PRIVATE fmt::fmt)
else()
    target_link_libraries(torrent_cli PRIVATE fmt)
endif()
