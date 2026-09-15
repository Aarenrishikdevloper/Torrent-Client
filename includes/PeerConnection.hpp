#pragma once
#include <string>

#include "Message.hpp"
#include "../includes/PeerQueue.hpp"

class PeerConnection {
private:
    // -1 means that there is  currently no connection
    int sockfd = -1;
    //information required for the Bitorrent handsake
    std::string infoHash;
    std::string peerId;
    //information about the current remote peer
    std::pair<std::string, long long> peer;
    //peer id received from the remote peer during handshake
    std::string peerPerrId;
    //initially choke is true till we received unchoke message
    bool choke = true;
    //bitfield received from the peer
    std::string bitfield;
    //Queue containing peers that we can connect to
    PeerQueue * peers;
    //create the 68 byte  Bittorrent handshake
    std::string createHandsake()const;
    //perform the Bittorrent handsake
    void performHandsake();
    //process a Message received form the peer
    void handleMessage(const Message &message );
    //establish tcp connection with the current peer
    void establishConnection();
   //receive and decode  one Bittorent Message
    Message receiveMessage();

public:
    // constructor
  PeerConnection(const std::string infoHash, const std::string&peerId, PeerQueue* peers);
  ~PeerConnection();
    //all peer connection owns a socket  and its therefore
    //neither copyable  nor movable
    PeerConnection(const PeerConnection&) = delete;
    PeerConnection& operator=(const PeerConnection&) = delete;
    PeerConnection(const PeerConnection&&) = delete;
    PeerConnection& operator=(const PeerConnection&&) = delete;
    void start(); // start connection to peers and communication with them


};
