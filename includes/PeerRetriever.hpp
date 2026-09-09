#pragma once
# include <string>
#include <cstdlib>
#include "TorrentParser.hpp"
class PeerRetriever {
  private:
    //port on which our Bittorent client listens
    int port;
    //Total filesize of the torrent
    long long fileSize;
    //traxker tell us how many secounds to wait
    //before connect it again
    long long interval{};
    //Unique Id of our BitTorrent Client
    std::string peerId;
    //list of a peers received from the tracker
    std::vector<std::pair<std::string, long long>> allPeers;
    //convert the tracker response into a list of peers
    std::vector<std::pair<std::string, long long>> decodeResponse(const std::string& response);
    //get peers from a UDP Tracker
    std::vector<std::pair<std::string, long long>>retrivePeersUDP(const std::string& announce, const TorrentParser& tfp, long long bytesDownloaded);
    public:
      explicit PeerRetriever(const std::string&peerId, int port, const TorrentParser& tfp, long long bytesDownloaded);
      ~PeerRetriever() = default;
     //contact the peers that were retrieved
    const std::vector<std::pair<std::string, long long>> retrivePeers(const TorrentParser& tfp, long long bytesDownloaded);
    //return the peers that were retrieved
    std::vector<std::pair<std::string, long long>> getPeers() const;
    //Return the tracker recomended announce interval
    long long getInterval() const;




};