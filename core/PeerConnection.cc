#include  "../includes/PeerConnection.hpp"

#include <stdexcept>
#include <unistd.h>
#include <bits/this_thread_sleep.h>
#include <iostream>
#include "utils.hpp"
#include "../includes/connection.hpp"
#include "../includes/PieceManager.hpp"

PeerConnection::PeerConnection(const std::string infoHash,  const std::string &peerId, PeerQueue *peers,PieceManager*pieceManager):infoHash(infoHash),peerId(peerId),peers(peers),pieceManager(pieceManager) {
     if (peers == nullptr) {
         throw std::invalid_argument("Null peers");
     }
     if (pieceManager == nullptr) {
         throw std::invalid_argument("Null pieceManager");
     }
}
//close the tcp socket when peerConnection is destroyed
PeerConnection::~PeerConnection() {
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
}

void PeerConnection::establishConnection()  {
    //get the ip adressa and port from peerQueue and initiate the connection
    sockfd = createConnection(peer.first, peer.second);
    //once TCP is established perform the Bittorrent protocol hasndsake
    performHandsake();
    //tell the peer we are interested in the piece they have
    const Message Interested(eMeesageType::Interested, "");
    sentData(sockfd, Interested.getMessageStr());

}

std::string PeerConnection::createHandsake() const {
    //bittorent protocol identifier
    std::string protocl = "BitTorrent protocol";
    //the BitTorrent  handshake  contains 8 reserved bytes
    //protocol extension
    const std::string reseved(8 , '\0');
    //the first byte contains  the length of BitTorrent protocol
    const std::uint8_t protocolLength = static_cast<std::uint8_t>(protocl.length());
    std::string handsake;
    //1 byte: protocol string length
    handsake += static_cast<char>(protocolLength);
    //19 bytes; "BitTorrent protocol"
    handsake += protocl;
    //8 bytes reserved field
     handsake += reseved;
    //20 bytes torrent info hash
    handsake += hexDecode(infoHash);
    //20 bytes:our peer Id
     handsake += peerId;
    if (handsake.length() != 68) {
        throw std::invalid_argument("Invalid Bittorent handsake length");
    }
    return handsake;
}

void PeerConnection::performHandsake() {
    //create our handshake
    const std::string handsake = createHandsake();
    //sent the 68 byte handsake to remote peer
    sentData(sockfd, handsake);
    //a bittorent handshake is exactly is 68 bytes
    const std::string response = reciveData(sockfd, 68);
    //make sure we actually receive a complete handsake
    if (response.length() != 68) {
        throw std::runtime_error("Incomplete handshake");
    }
    //validate protocol
    if (handsake.substr(0,20)!= response.substr(0,20)) {
        throw std::runtime_error("Invalid Bittorent protocl");
    }
    //validate info hash
    if (handsake.substr(28,20)!= response.substr(28,20)) {
        throw std::runtime_error("Peer info does not match");

    }
    peerPerrId= response.substr(48,20);
    std::cout << "handshake Sucessfully";
}

void PeerConnection::start() {
    while (!pieceManager->isComplete()) {
        //get the next available peer
        peer = peers->getPeers();
        //no peer is currently avalaible
        if (peer.first.empty()) {
            //wait for PeerRetriver to add more peer
            std::this_thread::sleep_for(std::chrono::seconds(7));
            continue;
        }
        try {
            //connect to the peer and perform handshake
            establishConnection();
            //once connected continuously  receive  Bittorrent  message
            while (!pieceManager->isComplete()) {
                   Message message = receiveMessage();
                   handleMessage(message);
            }
        } catch (const std::runtime_error &e) {
           if (!pieceManager->isComplete()) {
               std::cerr<<e.what()<<std::endl;
           }
            //report the bad peer and add it to bad peer list
            peers->reportBadPeers(peer);


        }
        //close the connection
        if (sockfd >=0) {
            close(sockfd);
            sockfd = -1;
        }

    }
}

Message PeerConnection::receiveMessage() {
    //getting the raw message
    const std::string rawMessage = reciveData(sockfd, 0);
    //convert the raw bytes into Message object
    return  Message(rawMessage);

}

void PeerConnection::handleMessage(const Message &message) {
    //process the message according to the Bittorent Id
    switch (message.geMessageType()) {
        case eMeesageType::KeepAlive: {
            //nothing to do just tell connection is open
            std::cout << "KeepAlive" << std::endl;
            break;
        }
        case eMeesageType::Choke: {
            //the peer does not allow us to request pieces
             choke = true;
             std::cout << "Choke" << std::endl;
            break;
        }
        case eMeesageType::Unchoke: {
            //the peer allow us to request pieces
             choke = false;
            std::cout << "Unchoke" << std::endl;
            if (!pieceManager->isComplete()) {
                requestPiece();
            }
            break;
        }

        case eMeesageType::Have: {
            std::cout << "Have" << std::endl;
            //HAVE PAYLOAD
            const std::string&paylaoad = message.getPayload();
            if (paylaoad.length() !=4) {
                throw std::invalid_argument("Invalid HAVE Message");
            }
            //update piecemanger knoeledge  about this  piece
            pieceManager->addToBitField(peerPerrId, paylaoad);
            break;
        }
        case eMeesageType::Bitfield:{
       std::cout << "Bitfield" << std::endl;
            //bitfield contains the peeers complete piece avalaibility
            bitfield = message.getPayload();
             //give the complete bitfiled to piecemanger
             pieceManager->addPeerBitField(peerPerrId, bitfield);
       break;
        }
            case eMeesageType::Piece: {
            std::cout << "Piece" << std::endl;
            const std::string&paylaoad = message.getPayload();
             //4bytes  piece index 4 bytes begin therefore at least 8 bit
             if (paylaoad.length()  < 8) {
                 throw std::invalid_argument("Invalid PIECE Message");
             }
               //first 4bytes is piece index
                const int pieceIndex = getIntFromStr(paylaoad.substr(0,4));
                 //Next 4 bytes is block offset
                  const int begin =  getIntFromStr(paylaoad.substr(4,4));
                //everything after the first 8 bytes is actual block data
                const std::string block = paylaoad.substr(8);
               //give the recive block to piece
            pieceManager->blockRecievd(pieceIndex,begin,block);
            //if the torent is not finished and peer not choke immediatly ask another block
            if (!choke && !pieceManager->isComplete()) {
                requestPiece();
            }
            break;
        }
        case eMeesageType::Cancel: {
            std::cout << "Cancel" << std::endl;
            break;
        }
    }

}

void PeerConnection::requestPiece() {
    try {
        //ask pieceManger which piece / block we should request
        std::string request = Message(eMeesageType::Request, pieceManager->requestPiece(peerPerrId)).getMessageStr();
        //send the complete request message to the connected peer
        sentData(sockfd, request);
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
    }

}
