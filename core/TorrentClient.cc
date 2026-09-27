#include "../includes/TorrentClient.hpp"
#include "curl/curl.h"
#include "../includes/PeerConnection.hpp"
#include  "../includes/PeerRetriever.hpp"

#ifndef  PORT
#define PORT 8080
#endif

#ifndef  PEER_ID
#define PEER_ID "-PC0001-123456789012"
#endif
//max no of PeerConnection
#ifndef  THREAD_NUM
#define THREAD_NUM 20
#endif

TorrentClient::TorrentClient(const char *torrentPath, const char *downloadPath, bool trackPrgress):trackProgress(trackProgress),
 //parse the .torrent file
 tfp(torrentPath),
  //create pieceManger
   pieceManager(tfp, downloadPath)
{
//intialize libcurl globally
    curl_global_init(CURL_GLOBAL_DEFAULT);
}
TorrentClient::~TorrentClient() {
    for (auto & thr : threads) {
        thr.join();
    }
    connections.clear();
    threads.clear();
    curl_global_cleanup();
}


void TorrentClient::run() {
    //create peerRetrievel and communicate with the bittorent track
    PeerRetriever p(
          std::string(PEER_ID),
          PORT,
          tfp,
          0
        );
    //get peers retuned by the tracker
    std::vector<std::pair<std::string,long long>> peers(p.getPeers());


    //put every discovered peer into peersQueue
    for (auto peer : peers) {
        peersQueue.push_back(peer);
    }
    //start progress tracking thread only for CLI
    if (trackProgress) {
        std::thread thread([&]() {
            try {
                pieceManager.trackProgress();
            } catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;
            }catch(...) {}
        });
        //store the thread so that we can join it later
        threads.push_back(std::move(thread));
    }
    //start Peerconnection thread
    //check whether we recieve  any peers
    bool PeersExist = peers.size();

    //start up to THREAD_NUM download threads
    for (size_t i = 0; i < THREAD_NUM; i++) {
        //create a Peerconnection object and immediately start connecting to peer and receiving message and passing blocks to  piecemanger
        auto pc = std::make_shared<PeerConnection> (
            tfp.getInfoHash(),
            std::string(PEER_ID),
            &peersQueue,
            &pieceManager
            );
        connections.push_back(pc);
        std::thread thread([pc]() {
            try {
                pc->start();
            } catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;

            }catch(...) {}
        });
        threads.push_back(std::move(thread));
    }
    //preodically refresh peer
    //rember  when we last connected the tracker
     auto lastUpdate  = std::chrono::steady_clock::now();
    //keep runing until the download is complete
    while (true) {
        //check wheter Piecemanger has complete
        if (pieceManager.isComplete()) {
            break;
        }
        //calculate how maany millisecounds have passed
        //since the last tracker update
        auto diff = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - lastUpdate).count();
        //refresh peer tracker announce interval has expired or there are no usable /free peers in PeersQueue
        if ((diff / 1000) > p.getInterval() || !peersQueue.hasFreePeers()) {
            //update the last  time update
            lastUpdate = std::chrono::steady_clock::now();
            //contact the tracker again
            p = PeerRetriever(
                std::string(PEER_ID),
                PORT,
                tfp,
                0
                );
                //get the newly discovered peers
            peers = p.getPeers();
            //add the  new peers to the shared peers to the shared queue
            //perrconnection threads can now pick them up
            for (const auto& peer : peers) {
                peersQueue.push_back(peer);
            }

        }
        //do not continiously  hammer the CPU While waiting
        //the main torrentclient sleeps for 7 secounds
        std::this_thread::sleep_for(std::chrono::seconds(7));
    }
    //wait for all threads to finish
    for (auto&thr : threads) {
      if (thr.joinable()) {
          thr.join();
      }
    }
    // clear the destrutor won't try
    threads.clear();
    connections.clear();

}
//return the size of the file
long long TorrentClient::getFileSize() const noexcept {
    return tfp.getLengthOne();
}
//Return the name of the torrent single file
const std::string &TorrentClient::getFileName() {
    if (tfp.isSingleFile()) {
        return  tfp.getSingleFile().name;
    }
    return tfp.getMultiFile().dirName;
}
