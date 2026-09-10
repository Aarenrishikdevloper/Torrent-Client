#include "../includes/PeerQueue.hpp"

#include <algorithm>

std::pair<std::string, long long> PeerQueue::getPeers() {
    //lock the mutex while accessing the queue
    //lock guard automatically unlocks when the function exist
    std::lock_guard<std::mutex> locK(mutexPeerqueue);
    //check whether we have consumed all peers
    if (i == peerQueue.size()) {
        //empty peer
        return {"",0};
    }
    //return the current peer and move the index to next peer
    return peerQueue[i++];

}

void PeerQueue::push_back(const std::pair<std::string, long long> &newPeer) {
    //only one thread can modify the queue at a time
    std::lock_guard<std::mutex> locK(mutexPeerqueue);
    //check max queue size if it becomes larger clear both good and bad peer list and reset index
    if (peerQueue.size() > maxPeerqueueSize) {
        peerQueue.clear();
        dummyPeers.clear();
        i=0;

    }
    //calculate how much time has passed since  the last successful last insertion
    auto elapsedTime = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - lastPushTime).count();
    //convert elapsed time into integer
    int millisecounds = static_cast<int>(elapsedTime);
    //convert milliseconds into seconds
    int secounds = millisecounds/1000;
    //check whether this peer already  in dummypeers
    if (std::ranges::find(dummyPeers, newPeer) == dummyPeers.end()) {
        //this peer is not marked as bad  add it to normal peer queue
         peerQueue.emplace_back(newPeer);
         //remember when we successfully added a peer
        lastPushTime = std::chrono::steady_clock::now();

    }
    else if (secounds > 10) {
        //even if marked bad the push was 10 sec ago clear all bad peer
        dummyPeers.clear();
        //add this peer to the normal queue
        peerQueue.emplace_back(newPeer);
        //update the successful push time
        lastPushTime = std::chrono::steady_clock::now();
    }




}

void PeerQueue::reportBadPeers(const std::pair<std::string, long long> &peer) {
    //protect dummyPeers from concurrent access
    std::lock_guard<std::mutex> locK(mutexPeerqueue);
    //store the problematic peer
    dummyPeers.emplace_back(peer);

}

bool PeerQueue::hasFreePeers() {
    //protect aceess to i and peers.size
    std::lock_guard<std::mutex> locK(mutexPeerqueue);
    return i < peerQueue.size();
}

size_t PeerQueue::size() {
    //protect access to vector
    std::lock_guard<std::mutex> locK(mutexPeerqueue);
    return peerQueue.size();
}