#pragma once
#include <thread>
#include <thread>

#include "PeerConnection.hpp"
#include "PeerQueue.hpp"
#include "PieceManager.hpp"
#include "TorrentParser.hpp"

class TorrentClient {
private:
     bool trackProgress;
     TorrentParser tfp;
      long long interval ;
     long long complete;
    PieceManager pieceManager;
     long long incomplete;
    PeerQueue peersQueue;
    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<PeerConnection>> connections;
    public:
      explicit  TorrentClient( const char*torrentPath, const char*downloadPath, bool trackPrgress =true);
      ~TorrentClient();
      TorrentClient() = delete;
      TorrentClient& operator=( const TorrentClient& other ) = delete;
      TorrentClient(const TorrentClient& other) = delete;
    TorrentClient& operator=( TorrentClient&& other ) = delete;
    TorrentClient(const TorrentClient&& other) = delete;
    void run();
    long long getFileSize()const noexcept;
    const std::string&getFileName();
};
