#pragma once
#include <mutex>
#include <string>
#include <utility>
#include <vector>

class PeerQueue {
private:
    //max number of peers allowed in the queue
     static  const int maxPeerqueueSize = 300;
     //index of next peer to retrive
     size_t i =0;
    //protect the queue when accesed by multiple threads
     std::mutex mutexPeerqueue;
    //avalaible peers
     std::vector<std::pair<std::string, long long>> peerQueue;
    //peers that have previously caused errors
    std::vector<std::pair<std::string, long long>> dummyPeers;
    //Time when a peer was added
    std::chrono::time_point<std::chrono::steady_clock> lastPushTime = std::chrono::steady_clock::now();
public:
    PeerQueue() = default;
    ~PeerQueue() = default;
    //get the next available peer
    std::pair<std::string, long long> getPeers();
    //add a peer to the queue
    void push_back(const std::pair<std::string, long long>& newPeer);
    //mark a peer as bad/unusable peer
    void reportBadPeers( const std::pair<std::string, long long>& peer );
    //check whether the queue contains available peers
    bool hasFreePeers();
    //return the number of available peer
    size_t size();






};
