#include "../includes/TorrentParser.hpp"
#include <fstream>
#include<format>
#include<iostream>

#include "../includes/utils.hpp"
#include "spdlog/spdlog.h"
//intially put an empty multifile object into variant
//Later if files does not exist we replace it with SingleFile
TorrentParser::TorrentParser(const std::string &filePath):file_(MultiFile{}) {
    //Open the torrent file in binary mode
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
       std::cerr<< std::format("[CRITICAL] Cannot open {}\n", filePath);
        throw std::runtime_error("[CRITICAL] Cannot open torrent file");
    }
    try {
        //Decode the Bencoded torrent file
        auto data  = bencode::decode(file);
        //a .torrent file has a dictionary at its top level
        //Extract the dictionary from the decoded value
        auto dict = std::get<bencode::dict>(data);
        //get the main tracker url
        announce_ = std::get<bencode::string>(dict["announce"]);
        //get the info dictionary
         auto info = std::get<bencode::dict>(dict["info"]);
        //Calculate the torrents info hash
        infoHash_ = sha1(bencode::encode(info));
        //get piece lenght
        pieceLenght_ = static_cast<std::uint64_t>(std::get<bencode::integer>(info.at("piece length")));   \
        //get piece hashes
        pieces_ = std::get<bencode::string>(info.at("pieces"));
        //parse trackers
        parseTracker(dict);

      //parse single-file or multi-file
        parseFiles(info);




    } catch (...) {
    }
}
//Parse Trackers
void TorrentParser::parseTracker(const bencode::dict &torrent) {
    //the main announcement url is always a tracker
    announceList_.push_back(announce_);
    //anounce-list is optional
    const auto it = torrent.find("announce-list");
    if (it == torrent.end()) {
        //no additional tacker
        return;
    }
    const auto& announceList = std::get<bencode::list>(it->second);
    //go through each tracker tier
     for (const auto& tierValue: announceList) {
         const auto& tier = std::get<bencode::list>(tierValue);
         //go through each tracker inside the tier
         for (const auto&trackValue:tier) {
            announceList_.push_back(std::get<bencode::string>(trackValue));
         }
     }

}

void TorrentParser::parseFiles(const bencode::dict &info) {
    //check whether files exist
    const auto it = info.find("files");
    //Multi-file torrent
    if (it != info.end()) {
        auto&multi = std::get<MultiFile>(file_);
        //name is the root
        multi.dirName = std::get<bencode::string>(info.at("name"));
        //store the list of files
        multi.files = std::get<bencode::list>(it->second);
        return;
    }
    //Single-file torrent
    SingleFile single;
    //file name
    single.name = std::get<bencode::string>(info.at("name"));
    //file size in bytes
    single.lenght = static_cast<std::uint64_t>(std::get<bencode::integer>(info.at("length")));
    //replace the MulitFile currently stored in file variant
    file_ = std::move(single);

}
//getters
const std::string &TorrentParser::getAnnounce() const noexcept {
    return announce_;

}
const std::vector<std::string>&TorrentParser::getAnnounceList() const noexcept {
    return announceList_;
}
const std::string &TorrentParser::getInfoHash() const noexcept {
    return infoHash_;
}
const std::uint64_t TorrentParser::getPieceLenght() const noexcept {
    return pieceLenght_;
}
const std::string &TorrentParser::getPieces() const noexcept {
    return pieces_;
}

bool TorrentParser::isSingleFile() const noexcept {
    return std::holds_alternative<SingleFile>(file_);
}
const MultiFile &TorrentParser::getMultiFile() const noexcept {
    return std::get<MultiFile>(file_);
}
const SingleFile &TorrentParser::getSingleFile() const noexcept {
    return std::get<SingleFile>(file_);
}