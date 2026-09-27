#include "../includes/Piece.hpp"
#include "../includes/utils.hpp"
#include <algorithm>
#include <netinet/in.h>

#include "../includes/utils.hpp"
//Create a Piece and Initialize it
Piece::Piece(int blockCount, long long totalLength, const std::string hash, bool isLastPiece):blocks(SetBlocks(blockCount, totalLength, isLastPiece)), hash(hash) {

}

std::vector<std::unique_ptr<Piece::Block> > Piece::SetBlocks(int blockCount, long long totalLength, bool isLastPiece) {
    //local vector that will contain all blocks
    std::vector<std::unique_ptr<Block>> blocks;
    //reserve memory for all  blocks so that the vector does not repeatedly reallocate adding block
    blocks.reserve(blockCount);
    //create each block
    for (int offset = 0; offset < blockCount; offset++) {
        //create a block dynamically
        std::unique_ptr<Block> block = std::make_unique<Block>();
        //initially we do not have the block
        block->status = eBlockstatus::missing;
        //calculate the bye offset of this block inside
        block->offset = offset * BLOCK_SIZE;
        //if this is the last block of last piece calculate its actual size
         if (isLastPiece && offset == blockCount - 1) {
             block->length = totalLength % BLOCK_SIZE;
             //if totalLength is exactly divisible by BLOCK_SIZe modulo gives 0
            if (!block->length) block->length = BLOCK_SIZE;


         }
          else {
              //every normal block is 16 KiB
              block->length = BLOCK_SIZE;

          }
        //move the unique_ptr into he vector
         blocks.push_back(std::move(block));
    }
    //return the complete list of blocks
    return blocks;

}
//check whether ever block in this piece has been recived
bool Piece::isFull() const {
    return  std::all_of(
        blocks.begin(),
        blocks.end(),[](const std::unique_ptr<Block>& block) {
            return block.get()->status ==eBlockstatus::retrieved;
        });
}
//determine whether a Block can be requested from a peer
bool Piece::isReadyToRequest(const Block *ptr) {
    return
       //case 1 a block can be requested when it has not been requested
       //case 2 it was requeted previously but request has been pending  for more than 7sec retry
       ptr->status == eBlockstatus::missing   || (ptr->status == eBlockstatus::pending && std::chrono::duration<double ,std::milli>(
           std::chrono::steady_clock::now() - ptr->timeRequested
           ).count() > 7000);

}
//finds a block that ca be requested
const std::string Piece::requestBlock() {
    //search through all blocks
    for (size_t i = 0; i < blocks.size(); i++) {
        //check whether this block can be requested
        if (isReadyToRequest(blocks[i].get())) {
            Block*ptr = blocks[i].get();
            //mark the block as pending
            ptr->status = eBlockstatus::pending;
            //remember when we requested it
            ptr->timeRequested = std::chrono::steady_clock::now();
            //convert offset and length into network byte order
            return intToBytes(htonl(ptr->offset)) + intToBytes(htonl(ptr->length));
        }
    }
    throw std::runtime_error("No Block to request");

}

//filldata called when a PIECE message is received from a peer
void Piece::fillData(int begin, const std::string &data) {
    //search for the Block having the requested offset
    for (size_t i = 0, n=blocks.size(); i<n; i++) {
        if (blocks[i].get()->offset == begin) {
            //we alreay recived this block recieving it again return  error
            if (blocks[i].get()->status == eBlockstatus::retrieved) {
                throw std::runtime_error("Requested block already exists");
            }
            //store the received bytes
            blocks[i].get()->data = data;
            //marked the block as successfully retrieved
            blocks[i].get()->status = eBlockstatus::retrieved;
            //block found and processed and returned
            return;

        }
    }
    //no block has this offset
    throw std::runtime_error("NO such offset exist");
}
//verify the hash
bool Piece::isHashChecked(const std::string &dataToFile) {
    return  hexDecode(sha1(dataToFile)) == hash;
}
//check whether the peice currently has one block
bool Piece::haveBlockToRequest() {
    for (const auto & block : blocks) {
        if (isReadyToRequest(block.get())) return true;
    }
    return false;
}
//reset block to missing
void Piece::resetAllBlockstoMissing() {
    for (const auto & block : blocks) {
        block->status = eBlockstatus::missing;
    }
}
//combine the data from every block into one continuous string
void Piece::fillDataToStr(std::string &dataToFile) {
    for (const auto & block : blocks) {
        dataToFile += block->data;
    }
}
