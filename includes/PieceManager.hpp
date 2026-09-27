#pragma once
#include <filesystem>
#include <fstream>
#include <mutex>
#include <unordered_map>

#include "Piece.hpp"
#include "TorrentParser.hpp"

class PieceManager {
    private:
      const TorrentParser tfp;
      //directory where the torrent contents will be downloaded
     std::filesystem::path downLoadPath;
     std::size_t totalpeices =0;
     std::uint64_t totalDownload =0;
     //store which pieces are avaliable from each peer
     std::unordered_map<std::string, std::vector<bool>> peerBitField;
     //all pieces belonging to the torrent
      std::vector<std::unique_ptr<Piece>> Pieces;
      //use for single-file torrents
      std::string downloadFilePath;
      std::ofstream downloadFile;
    //start time of download
    std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
    //no of bytes at the previous progress checks
    std::uint64_t lastCheckFileSize = 0;
    //protect PieceManager state
    std::mutex mutex;
    //protect file writing
    std::mutex mutexWrite;
    //create all place objects
    std::vector<std::unique_ptr<Piece>> intializePieces();
    //write a complete piece to disk
    void WriteDataToFile(
        int index,
         const std::string& dataToFile
        );
    //display download progress
    void display(
        const float currentFileSize,
        const double n,
        const int lenghtOfSize

        );
public:
     explicit PieceManager(
         const TorrentParser& tfp,
         const std::filesystem::path& downLoadPath);
     ~PieceManager();
     PieceManager(const PieceManager&) = delete;
     PieceManager& operator=(const PieceManager&) = delete;
    PieceManager(PieceManager&&) = delete;
    PieceManager& operator=(PieceManager&&) = delete;
    //select the intial bitified recived from peer
    void addPeerBitField(const std:: string&peerPeerId, const std::string&payload);
    //select a piece/block  that the specific peer has
    const std::string requestPiece(const std::string&peerPeerId);
    //receives a block from a peer
    void blockRecievd(int index, int begin, const std::string& blockStr);
    //update a peer's bitfield when a have message is recieved
    void addToBitField(const std::string&peerPeerId, const std::string&payload);
    //returns true when the complete torrent has beeen complete
    bool isComplete();

    //display download progress
    void trackProgress();
    //display download  speed;
    void trackSpeed();




};
