#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include "../bencoder/bencoder.hpp"

//info about a single file
struct SingleFile {
    std::string name;
    std::uint64_t lenght;
};
//info about a Mulitfile
struct  MultiFile {
    std::string dirName;
    bencode::list files;
};
class TorrentParser {
    public:
    //opens and parse the .torrent file
      explicit TorrentParser(const std::string& filePath);
      //main tracker
      const std::string& getAnnounce() const noexcept;
      //Additional Tracker
    std::string getAnnounceList(size_t i) const noexcept;
     //Torrent identifier
    const std::string& getInfoHash() const noexcept;
    //size of each piece
    const std::uint64_t getPieceLenght() const noexcept;
    //SHA-1 hash of all files
    const std::string& getPieces() const noexcept;
    //Check whether the torrent contains one file
    bool isSingleFile() const noexcept;
    //single-file information
    const SingleFile& getSingleFile() const noexcept;
    //Multi file information
    const MultiFile& getMultiFile() const noexcept;
    long long getLengthOne() const noexcept;
    size_t getAnnounceList_Lenght() const noexcept;
private:
    //parse tracker information

    void parseTracker(const bencode::dict& torrent);
    //Parse Single-file or multi-file information
    void parseFiles(const bencode::dict& info);

    //Torrent Metadata
    std::string announce_;
    std::vector<std::string> announceList_;
    std::string infoHash_;
    std::string pieces_;
    std::uint64_t pieceLenght_;
    //File information
    std::variant<SingleFile, MultiFile> file_;





};