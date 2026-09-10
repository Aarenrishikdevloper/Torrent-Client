#include <format>
#include <random>
#include <netdb.h>
#include "../includes/PeerRetriever.hpp"
#include <curl/curl.h>
#include "utils.hpp"
#include <cstring>
#include <arpa/inet.h>
//CURL receives the http traker response in small chunks  and collects all those chunks into string
size_t WriteCallback(void * contents, size_t size, size_t memb, std::string*output) {
    //Calculate how many CURL give us
    const size_t totalSize = size * memb;
    //Add those bytes to our response string
    output->append(
         static_cast<char*>(contents),
         totalSize
        );
    //tell curl that we sucessfully processed all bytes
    return totalSize;
}
//UDP Tracker use a transaction id to matcha response  so we generate 32-bit number
static::uint32_t randomTransactionId() {
    static std::random_device randomDevice;
    return randomDevice();
}

PeerRetriever::PeerRetriever(const std::string &peerId, int port, const TorrentParser &tfp,
                             long long bytesDownloaded) : port(port), fileSize(tfp.getLengthOne()),peerId(peerId) {

      //as soon as  Peerreceiver is created  by torrent client we contact the tracker and retrive the list of peer
     allPeers = retrivePeers(tfp, bytesDownloaded);


}

std::vector<std::pair<std::string, long long> > PeerRetriever::retrivePeers(
    const TorrentParser &tfp, long long bytesDownloaded) {
    std::vector<std::pair<std::string, long long>> peers;
    //tracker index tells us which tracker we are trying
    size_t trackerIndex = 0;
    //keep trying until we get at least one peer
    while (peers.empty()) {
        std::string announce;
        if (trackerIndex == 0) {
            //frist try the main tracker
            announce = tfp.getAnnounce();

        }else if (trackerIndex <= tfp.getAnnounceList_Lenght()) {
            //try tracker from announce list
           announce= tfp.getAnnounceList(trackerIndex-1);
        }else {
            //we have tried all tracker  link then break the loop
            break;
        }
        try {
            if (announce.starts_with("udp://")) {
                peers = retrivePeersUDP(
                     announce,
                     tfp,
                      bytesDownloaded
                    );
            }else {
                //intiating curl
                CURL*curl = curl_easy_init();
                if (curl) {
                    std::string response;
                  //Build the tracker request
                    const std::string query = announce+"?info_hash="+urlEncodeHex(hexDecode(tfp.getInfoHash()))+"&peer_id="+peerId+"&port="+std::to_string(port)+"&uploaded=0"+"downloaded="+std::to_string(bytesDownloaded)+"&left"+std::to_string(fileSize-bytesDownloaded)+"&compact=1";
                    //tell curl where to send the request
                     curl_easy_setopt(curl, CURLOPT_URL, query.c_str());
                     //don't wait forever if the tracker does not respond
                     curl_easy_setopt(
                         curl,
                         CURLOPT_TIMEOUT,
                         15L
                         );
                    //identity for our application
                     curl_easy_setopt(
                         curl,
                         CURLOPT_USERAGENT,
                          "TorrentClient"
                         );
                     //tell curl  which function should receive the response
                     curl_easy_setopt(
                         curl,
                         CURLOPT_WRITEFUNCTION,
                         WriteCallback
                         );
                    //Give writeback our response string
                     curl_easy_setopt(
                         curl,
                         CURLOPT_WRITEDATA,
                         &response
                         );
                    //send the Http result
                    const CURLcode res = curl_easy_perform(curl);
                    //curl object is no longer required
                    curl_easy_cleanup(curl);
                    if (res == CURLE_OK) {
                        peers = decodeResponse(response);
                    }else {
                        std::cerr<< std::format("HTTP tacker request failed");
                    }

                }
            }
        } catch (const std::exception& e) {
            throw std::runtime_error(std::format("Tracker error: {}", e.what()));
        }
        trackerIndex++;
    }
   return  peers;


}

std::vector<std::pair<std::string, long long>> PeerRetriever::decodeResponse(const std::string &response) {
    std::vector<std::pair<std::string, long long>> results;
    try {
        //decode bencode
        auto data  = bencode::decode(response);
        //tracker repose should be a dictionary
        auto dict = std::get<bencode::dict>(data);
        //read announce interval
        interval = std::get<bencode::integer>(dict["interval"]);
        try {
            //normal peer format
            auto peers = std::get<bencode::list>(dict["peers"]);
            for (const auto& peer : peers) {
                auto peerDict = std::get<bencode::dict>(peer);
                //read ip
                const std::string ip = std::get<bencode::string>(peerDict["ip"]);
                //read port
                const long long  peerPort = std::get<bencode::integer>(peerDict["port"]);
                //add peer to our result
                results.emplace_back(
                      ip,
                      peerPort
                    );
            }

        } catch (...) {
             //Compact peer format
             const std::string peers = std::get<bencode::string>(dict["peers"]);
              constexpr  size_t peersize = 6;
            for (size_t i =0 ; i+ peersize <= peers.size(); i+=peersize) {
                //first 4 bytes of ip
                const std::string ipBytes = peers.substr(i,4);
                //Next 2 bytes
                const std::string portBytes = peers.substr(i+4,2);
                //convert binary ip
                //"192.168.1.10"
                const std::string ip = bytesToIpAddress(ipBytes);
                //convert binary port to number
                const long long peerPort = bytesToPort(portBytes);
                //add peer
                results.emplace_back(
                    ip,
                    peerPort
                    );

            }
        }
    } catch (const std::exception& e) {

        throw std::runtime_error(std::format("Could not decode tracker: {}", e.what()));
    }
    return results;
}

std::vector<std::pair<std::string, long long>> PeerRetriever::retrivePeersUDP(const std::string &announce, const TorrentParser &tfp, long long bytesDownloaded) {
    std::vector<std::pair<std::string, long long>> results;
    //parse the UDP tracker URL
    //remove udp::// leaving tracker.example.com:6969/announce
    std::string address = announce.substr(6);
    //find "/announce"
    const size_t slashPosition = address.find('/');
    if (slashPosition != std::string::npos) {
        //keep only tracker.example.com:6969
        address = address.substr(0,slashPosition);
    }
    const size_t colonposition = address.find(':');
    if (colonposition == std::string::npos) {
        throw std::runtime_error("Invalid UDP URL");
    }
    std::string hostname = address.substr(0, colonposition);
    const std::string trackerPort = address.substr(colonposition+1);
    //resolve tracker hostname
    //convert tracer.example.com into an IP address that socket can use
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    addrinfo* servinfo = nullptr;
    if (getaddrinfo(hostname.c_str(), trackerPort.c_str(),&hints, &servinfo) != 0) {
        throw std::runtime_error("Coild not resolve UDP tracker");
    }
    //create UDP socket
    const int socketFd = socket(
        AF_INET,
        SOCK_DGRAM,
         0
        );
     if (socketFd < 0) {
         freeaddrinfo(servinfo);
         throw std::runtime_error("Could not create UDP socket");
     }

    //set timeout don't wait forever for the tracker
     timeval timeout;
     timeout.tv_sec = 15;
     timeout.tv_usec = 0;
     setsockopt(
         socketFd,
          SOL_SOCKET,
          SO_RCVTIMEO,
          &timeout,
          sizeof(timeout)
         );
    //create Connection request
    std::uint8_t connectRequest[16]{};
    //this is the fixed protocol id required by the bit torrent UDP tracker
    const std::uint64_t protocolID = 0x41727101980ULL;
    const std::uint64_t protocolNetwork = htobe64(protocolID);
    std::memcpy(
        connectRequest,
        &protocolNetwork,
        8
        );
    //Action 0 means connect
    const std::uint32_t connectAction = htonl(0);
    std::memcpy(
        connectRequest + 8,
        &connectAction,
        4
        );
    //generate a random transaction Id  the tracker will return the same Id in its response
    const std::uint32_t transactionID =  randomTransactionId();
    const std::uint32_t transactionNetwork = htonl(transactionID);
    std::memcpy(
        connectRequest + 12,
        &transactionNetwork,
        4
        );
    //send connect request
    sendto(
        socketFd,
        connectRequest,
        sizeof(connectRequest),
        0,
        servinfo->ai_addr,
        servinfo->ai_addrlen
        );
    //receive connect response
    std::uint8_t connectResponse[32]{};
    const ssize_t recieved  = recvfrom(
        socketFd,
        connectResponse,
        sizeof(connectResponse),
        0,
        nullptr,
        nullptr
        );
    if (recieved < 0) {
        close(socketFd);
        freeaddrinfo(servinfo);
        throw std::runtime_error("UDP connect recvfrom failed (timeout or error): "
                                 + std::string(strerror(errno)));
    }
    if (recieved < 16) {
        close(socketFd);
        freeaddrinfo(servinfo);
        throw std::runtime_error("Inavlid UDP tracker  connect response");
    }
    //check response action
    std::uint32_t responseAction;
    std::memcpy(
        &responseAction,
        connectResponse,
        4
        );
      responseAction = ntohl(responseAction);
        //response action must be 0
       if (responseAction != 0) {
           close(socketFd);
           freeaddrinfo(servinfo);
           throw std::runtime_error("Inavlid UDP tracker  connect response");
       }
    //check transaction ID
    std::uint32_t responseTransactionID;
    std::memcpy(
        &responseTransactionID,
        connectResponse +4,
         4
        );
    responseTransactionID = ntohl(responseTransactionID);
    //male sure this response belong to our request
    if (responseTransactionID != transactionID) {
        freeaddrinfo(servinfo);
        throw std::runtime_error(" UDP tracker  transaction id mismatch");
    }
    //Get Connection Id
    //the tracker gives us a connection ID
    std::uint64_t connectionId;
    std::memcpy(
        &connectionId,
        connectResponse + 8,
        8
        );
    //Create announce request
    std::uint8_t announceRequest[98]{};
    //connection ID
    std::memcpy(
         announceRequest,
         &connectionId,
         8
        );
    //action
    // 1 mensa: announce
    const std::uint32_t announceAction = htonl(1);
    std::memcpy(
        announceRequest + 8,
         &announceAction,
         4
        );
    //Transaction ID
    const std::uint32_t announceTransaction =randomTransactionId();
    const std::uint32_t announceTransactionNetwork = htonl(announceTransaction);
    std::memcpy(
        announceRequest + 12,
        &announceTransactionNetwork,
        4

        );
    //info hash
    const std::string infoHash = hexDecode(
        tfp.getInfoHash());
     std::memcpy(
         announceRequest + 16,
          infoHash.data(),
          20
         );
    //Peer ID
    std::memcpy(
        announceRequest + 36,
        peerId.data(),
        std::min(
            peerId.size(),static_cast<size_t>(20))
        );
    //downloaded
    const std::uint64_t dowbnloaded = htobe64(
        static_cast<std::uint64_t>(bytesDownloaded));
     std::memcpy(
         announceRequest + 56,
         &dowbnloaded,
         8
         );
    //how many bytes are still required
    const std::uint64_t leftBytes = htobe64(static_cast<std::uint64_t>(fileSize - bytesDownloaded));
    std::memcpy(
        announceRequest + 64,
        &leftBytes,
        8
        );
    //uploaded
    const std::uint64_t uploaded = htobe64(0);
    std::memcpy(
        announceRequest + 72,
        &uploaded,
        8
        );
    //Event 0 means no special event
    const std::uint32_t event = htonl(0);
    std::memcpy(
         announceRequest + 80,
         &event,
         4
        );
    //ip address
    const std::uint32_t ip = htonl(0);
    std::memcpy(
        announceRequest + 84,
        &ip,
        4
        );
    //random key
    const std::uint32_t key = htonl(randomTransactionId());
    std::memcpy(
        announceRequest +88,
        &key,
        4
        );
    //no of peers wanted
    const std::int32_t numWant =htonl(-1);
    std::memcpy(
        announceRequest + 92,
        &numWant,
        4
        );
    //listening port
    const std::uint16_t networkPort = htons(
        static_cast<uint16_t>(port));
    std::memcpy(
        announceRequest + 96,
        &networkPort,
        2);
    //send announce request
    sendto(
        socketFd,
        announceRequest,
        sizeof(announceRequest),
        0,
        servinfo->ai_addr,
        servinfo->ai_addrlen
        );
    //receive announce response
    std::int8_t response[4096]{};
    const ssize_t responseSize = recvfrom(
         socketFd,
         response,
         sizeof(response),
         0,
         nullptr,
         nullptr
        );
    //closing the socket
    close(socketFd);
    //we are finished with the address information
     freeaddrinfo(servinfo);

    if (responseSize < 20) {

        throw std::runtime_error("Invalid UDP tracker  announce response");
    }
    //check announce response
    std::uint32_t action;
    std::memcpy(
        &action,
        response,
        4
        );
    action = ntohl(action);
    //action 1 mean announce response
    if (action != 1) {
        throw std::runtime_error("Invalid UDP tracker  announce failed");
    }
    //check transaction id
    std::uint32_t responseTransactionId;
    std::memcpy(
        &responseTransactionId,
        response + 4,
        4
        );
    responseTransactionId =  ntohl(responseTransactionId);
    if (responseTransactionId != announceTransaction) {
        close(socketFd);       // ✅ add this
        freeaddrinfo(servinfo);
        throw std::runtime_error("UDP tracker  transaction id mismatch");
    }
    //read tracker interval
    std::uint32_t trackerInterval;
    std::memcpy(
        &trackerInterval,
         response + 8,
         4
        );
        interval = ntohl(trackerInterval);
    //extract peers
    constexpr  size_t peerSize = 6;
    for (size_t position = 20; position + peerSize <= static_cast<size_t>(responseSize); position += peerSize) {
        //read IP address
        char ipAddress[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET,response + position, ipAddress, sizeof(ipAddress));
        //read port
        std::uint16_t peerport;
        std::memcpy(
            &peerport,
             response + position + 4,
              2
            );
        peerport = ntohs(peerport);
        //add peer to result
        results.emplace_back(
            ipAddress,
            peerport
            );
    }
    return results;
}
//get peers
std::vector<std::pair<std::string, long long>> PeerRetriever::getPeers() const {
    return  allPeers;
}

long long PeerRetriever::getInterval() const {
    return interval;
}
