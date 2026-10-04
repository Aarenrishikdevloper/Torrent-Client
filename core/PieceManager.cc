#include "../includes/PieceManager.hpp"

#include <netinet/in.h>

#include "utils.hpp"
#include "../includes/TorrentParser.hpp"
#include "../bencoder/bencoder.hpp"
#include "thread"
#ifndef AMOUNT_HASH_SYMBOLS
# define  AMOUNT_HASH_SYMBOLS 37
#endif

namespace fs = std::filesystem;
//constructor
PieceManager::PieceManager(const TorrentParser &tfp, const std::filesystem::path &downLoadPath):tfp(tfp), downLoadPath(downLoadPath), Pieces(intializePieces()) {
    //we open one file here for a single file torrent
    if (tfp.isSingleFile()) {
        const SingleFile &singleFile = tfp.getSingleFile();
        std::string fileName = singleFile.name;
        //prevent accidental directory traversal  /
        //nested paths for a single-file torrents
        std::replace(
            fileName.begin(),
            fileName.end(),
            '/',
            '_'
            );
        const std::filesystem::path downloadFilePath = downLoadPath / fileName;
        fs::create_directories(downLoadPath);
        downloadFile.open(
            downloadFilePath,
            std::ios::binary | std::ios::out | std::ios::in
            );
        if (!downloadFile.is_open()) {
            //the file may not exist create if first
            std::ofstream createFile(
                downloadFilePath, std::ios::binary);
            if (!createFile.is_open()) {
                throw std::runtime_error("Cannot create download file");
            }
            createFile.close();
            downloadFile.open(
                downloadFilePath,
                std::ios::binary | std::ios::out | std::ios::in
                );
        }
        if (!downloadFile.is_open()) {
            throw std::runtime_error("Cannot open download file");
        }
    }

}
//destructor
PieceManager::~PieceManager() {
    if (downloadFile.is_open()) {
        downloadFile.close();
    }
}
//splite the concentand SHA-1 piece hashes
static std::vector<std::string>splitHashPieces(const std::string&pieces) {
    std::vector<std::string> result;
    //every bittorent piece has exactly 20 bytes(SHA-1)
    if (pieces.length() % 20 !=0) {
        throw std::runtime_error("Invalid piece length");
    }
    const std::size_t piecesCount = pieces.length() / 20;
    result.reserve(piecesCount);
    for (std::size_t i = 0; i < piecesCount; ++i) {
        result.push_back(
            pieces.substr(i * 20, 20)
            );
    }
    return result;
}
//intialise pieces
std::vector<std::unique_ptr<Piece> > PieceManager::intializePieces() {
    std::vector<std::unique_ptr<Piece>> result;

    //split the concatenated 20 byte SHA-1 hashes
    const std::vector<std::string> pieceHashes = splitHashPieces(tfp.getPieces());
    totalpeices = pieceHashes.size();
    const std::uint64_t piecesLength = tfp.getPieceLenght();
    // calculate the total torrent size

    if (tfp.isSingleFile()) {
        const SingleFile &singleFile = tfp.getSingleFile();
        totalbytes =singleFile.lenght;

    }
    else {
        const MultiFile &multiFile = tfp.getMultiFile();
        for (const bencode::data&fileData: multiFile.files) {
            //each element is length:integer and path::list
            const bencode::dict&fileDict = std::get<bencode::dict>(fileData);
            const std::uint64_t fileLenght = static_cast<std::uint64_t>(std::get<long long>(fileDict.at("length")));
            totalbytes += fileLenght;
        }
    }
    result.reserve(pieceHashes.size());
    //create every piece
    for (std::size_t i =0; i<pieceHashes.size(); ++i) {
        //normally every piece has pieceLengh
        std::uint64_t currentPieceslength = piecesLength;
        //global offset of this piece
        const std::uint64_t pieceOffset = i * piecesLength;
        //the last piece may be samller
        if (pieceOffset + piecesLength >totalbytes) {
            currentPieceslength = totalbytes - pieceOffset;
        }
        //no of 16KiB block required by piece
        const int blockCount = static_cast<int>((currentPieceslength + BLOCK_SIZE -1)/BLOCK_SIZE);
        const bool isLastPiece = (i== totalpeices -1);
        result.push_back(
            std::make_unique<Piece>(
                blockCount,
                static_cast<long long>(currentPieceslength),
                pieceHashes[i],
                isLastPiece
            )
        );

    }
    return result;


}
//add peer bitfield
void PieceManager::addPeerBitField(const std::string &peerPeerId, const std::string &payload) {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<bool> bits;
    //one byte contains eight piece avalaibility bits
    bits.resize(payload.size()*8);
    for (std::size_t i = 0; i < payload.size(); ++i) {
        const unsigned char byte = static_cast<unsigned char>(payload[i]);
        for (int j =0; j < 8; ++j) {
            const size_t bitPostion = i*8+j;
            //bittorent sends the most significant bits first
            bits[bitPostion] =(byte >> (7-j)) & 1;
        }
    }
    //the final byte can contain padding bytes
    //we do not need  them do not repesenting real piece
    if (bits.size() > totalpeices) {
        bits.resize(totalpeices);
    }
    peerBitField[peerPeerId] = std::move(bits);

}
//check whether a torrent is complete
bool PieceManager::isComplete() {
    std::lock_guard<std::mutex> lock(mutex);
    return totalDownload  == totalpeices;
}
//request a block from a peer
const std::string PieceManager::requestPiece(const std::string &peerPeerId) {
    std::lock_guard<std::mutex> lock(mutex);
    //make sure we actually received a bitfield from this peer
    auto peerIterator  = peerBitField.find(peerPeerId);
    if (peerIterator == peerBitField.end()) {
        throw std::runtime_error("NO Biffield avalaible for this perr");
    }
    const std::vector<bool>&bitfield = peerIterator->second;

    /*  Look for a piece that.
      - 1 we still need.
      - 2 has a block ready to request .
      -3 The peer posses
      */
    for (std::size_t i=0; i < bitfield.size(); ++i) {
        if (Pieces[i] == nullptr) {
            continue;
        }
        if (!Pieces[i]->haveBlockToRequest()) {
            continue;
        }
        if (i >= bitfield.size() || !bitfield[i]) {
            //this peer does not have this piece
            continue;
        }
        //get 4 bytes = block offset
        //get 4 bytes = block length
        const std::string blockInfo = Pieces[i]->requestBlock();
        //complete request payload 4 bytes = block index 4 bytes =  block offset 4 bytes =  block length
        return intToBytes(
            htonl(
                static_cast<int>(i)
                )


            )+ blockInfo;

    }
   throw std::runtime_error("NO piece from this peer");
}
//recieve a piece block
void PieceManager::blockRecievd(int index, int begin, const std::string &blockStr) {
    //validate piece index before accessing pieces{index
    if (index < 0 || static_cast<std::size_t>(index) >= Pieces.size()) {
        throw std::runtime_error("Invalid block index");
    }
    std::string dataTOfile;

    {   //protect pieceManger state
        std::lock_guard<std::mutex> lock(mutex);
        Piece* ptr = Pieces[index].get();
        //nullptr means piece is already completed by another connection
        if (ptr == nullptr) {
            return;
        }
        //store the received block
        ptr->fillData(begin, blockStr);
        //if piece is not complete
        if (!ptr->isFull()) {
            return;
        }
        //combine all block into one complete piece
        ptr->fillDataToStr(dataTOfile);
        //verif the sha-1 hash
        if (!ptr->isHashChecked(dataTOfile)) {
            std::cout << "Hash checked failed" << std::endl;
            //request the block again
            ptr->resetAllBlockstoMissing();
            return;
        }

     //the piece now   completly verified  nullptr means the pice is finished
        Pieces[index] = nullptr;
        downloadBytes += dataTOfile.size();
        ++totalDownload;

    }
    //we write to disk after releasing mutex as dsk i/o are slow  do not want to hold piece mutex
     WriteDataToFile(index, dataTOfile);
}
void PieceManager::WriteDataToFile(int index, const std::string &dataTOfile) {
    //only one thread should perform file  writea at a time
    std::lock_guard<std::mutex> lock(mutexWrite);
    //rcvery piece except tha fianl piece normally start at index * piceLenght
     std::uint64_t pieceLenght =tfp.getPieceLenght();
     std::uint64_t globalOffset = static_cast<uint64_t>(index)*pieceLenght;
     //sinble bitttorent file
    if (tfp.isSingleFile()) {
        //move to the global byte poaition
        downloadFile.seekp(static_cast<std::streamoff>(globalOffset));
        if (!downloadFile) {
            throw std::runtime_error("Failed to seek file");
        }
        downloadFile.write(
            dataTOfile.data(),
            static_cast<std::streamsize>(dataTOfile.size())
            );
        if (!downloadFile) {
            throw std::runtime_error("Failed to write to file");
        }
        downloadFile.flush();
        return;
    }
    //multifile torrent
    const MultiFile& multiflie = tfp.getMultiFile();
    //dataoffset tells us how much of this piece has already beem written
    std::size_t dataOffset = 0;
    //this represents the global offset where the  current file begins
    std::uint64_t  currentFileOffset =0;
    //go through every  file in the torrent
    for (const bencode::data& fileData : multiflie.files) {
        //each element is adictionary
        const bencode::dict&fileDict = std::get<bencode::dict>(fileData);
        //get file lenght
        const std::uint64_t fileLenght = static_cast<std::uint64_t>(std::get<bencode::integer>(fileDict.at("length")));
        //get path list
        const bencode::list&pathList = std::get<bencode::list>(fileDict.at("path"));
        //build folder/file.txt
        fs::path realtivePath;
        for (const bencode::data& pathPart:pathList) {
            realtivePath /= std::get<bencode::string>(pathPart);
        }
        //does this piece reach this file
        const std::uint64_t currentFileEnd =currentFileOffset +fileLenght;
        //the piece starts completely after this file
        if (globalOffset >=currentFileEnd) {
            currentFileOffset = currentFileEnd;
            continue;
        }
        //the complete piece has been written
        if (dataOffset >= dataTOfile.size()) {
            break;
        }
        //calculate position inside current file
        std::uint64_t offsetInsideFile = 0;
        if (globalOffset > currentFileOffset) {
            offsetInsideFile = globalOffset - currentFileOffset;
        }
        //how many bites can fit in this filter
        const std::uint64_t avalaibleINFile = fileLenght - offsetInsideFile;
         const std::size_t remainingPiece = dataTOfile.size() - dataOffset;
        const std::size_t bytesTOwrite = static_cast<std::size_t>(
            std::min<std::uint64_t>(avalaibleINFile, remainingPiece)
            );
        //contruct actual output file
        const fs::path ouputPath = downLoadPath/multiflie.dirName/realtivePath;
        //create directories
        if (!ouputPath.parent_path().empty()) {
            fs::create_directories(ouputPath.parent_path());
        }
        //open the file
        std::fstream outputFile(
              ouputPath,
              std::ios::binary | std::ios::in | std::ios::out
            );
        //if file does not exist create it it first
        if (!outputFile.is_open()) {
            std::ofstream createFile(
                 ouputPath,
                 std::ios::binary
                );
            if (!createFile.is_open()) {
                throw std::runtime_error("Failed to create file");
            }
            createFile.close();
            outputFile.open(
                ouputPath,
                std::ios::binary | std::ios::in | std::ios::out
                );
        }
        if (!outputFile.is_open()) {
            throw std::runtime_error("Failed to open file");
        }
        //move to the correct position on this file
        outputFile.seekp(
            static_cast<std::streamoff>(offsetInsideFile)
            );
        if (!outputFile) {
            throw std::runtime_error("Failed to seek file");
        }
        //write this portion of the piece
        outputFile.write(
             dataTOfile.data() + dataOffset,
              static_cast<std::streamsize>(bytesTOwrite)
            );
        if (!outputFile) {
            throw std::runtime_error("Failed to write to file");
        }
        outputFile.flush();
        outputFile.close();
        //move to the nex part of piece
        dataOffset += bytesTOwrite;
        //move to the next file global position
        currentFileOffset =currentFileEnd;
        //the entire piece has been written
        if (dataOffset == dataTOfile.size()) break;
        //the piece continues crossed the file boundary continue with next file
        globalOffset = currentFileOffset;
    }
    //verify complete piece has been written
    if (dataOffset != dataTOfile.size()) {
        throw std::runtime_error("Not all piece data was writtren");
    }

}
//handle have message
void PieceManager::addToBitField(const std::string &peerPeerId, const std::string &payload) {
    //lock the Piecemanager mutex
    std::lock_guard<std::mutex> lock(mutex);
    //find the bitfield belonging to this particular peer
    auto iterator = peerBitField.find(peerPeerId);
    //if we do not hyave a bitfield registred for this peer
    if (iterator == peerBitField.end()) {
        std::cerr << "Peer peer " << peerPeerId << " not found" << std::endl;
        return;
    }
    //the payload of a have message contains a piece index
    const int bitPosition = getIntFromStr(payload);
    //verifying the piece index is valid
    if (bitPosition <0 || static_cast<std::size_t>(bitPosition) >= totalpeices) {
        std::cout << "Invalid peer payload " << payload << std::endl;
    }
    //iterator-> second is the vector<bool> belonging to this peer
    std::vector<bool>&bitfield = iterator->second;
    //the peer has the piece therefore set the piece bit to true
    bitfield[bitPosition] = true;

}
static constexpr float BYTES_PER_MB = 1'048'576.0f;

DowloadStats PieceManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex);
    DowloadStats stats;
    stats.downloadBytes =downloadBytes;
    stats.totalbytes = totalbytes;
    stats.completedPices =totalDownload;
    stats.totalPics = totalpeices;
    stats.complete = totalbytes > 0 && totalDownload == totalpeices;
    //speed:recompute at most once per speed the smooth it
    const auto now = std::chrono::steady_clock::now();
    const double dt = std::chrono::duration<double>(now - lastSpeedTime).count();
    if (dt >= 1.0) {
        const double delta = static_cast<double>(downloadBytes) - static_cast<double>(lastSpeedbytes);
        const double instant = delta /dt / BYTES_PER_MB;
        smothedSpeed = (smothedSpeed == 0.0)?instant:0.7*smothedSpeed+0.3*instant;
        lastSpeedbytes = downloadBytes;
        lastSpeedTime = now;
    }
    stats.speedMBps = smothedSpeed;
    return stats;
}






