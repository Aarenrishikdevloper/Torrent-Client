#pragma once
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#define  BLOCK_SIZE 16384
class Piece {
 private:
     //eblockStatus representing  the status of block
     enum class eBlockstatus:int {
          missing = 0,
          pending = 1,
         retrieved = 7,

     };
     //block represent one block inside a piece
     struct  Block {
         //offset of this block inside the piece
         int offset;
         //no of bytes contained in this block
         int length;
         //current status of this block
         eBlockstatus status;
         //actual bytes received for this block
         std::string data;
         //time at which this block was received
         std::chrono::time_point<std::chrono::steady_clock> timeRequested;

     };
     //blocks contains all blocks belonging to this piece
     std::vector<std::unique_ptr<Block>> blocks;
     //store SHA-1 hash expected for this piece
     const std::string hash;
    //create the blocks belonging to this piece
    std::vector<std::unique_ptr<Block>> SetBlocks(int blockCount, long long totalLength, bool isLastPiece);
    //determine whether a block can be requested
    inline  bool  isReadyToRequest(const Block* ptr);
    public:
      //create a piece
       explicit Piece(
           int blockCount,
            long long totalLength,
           const std::string hash,
            bool isLastPiece
           );
       //no special destruction is needed because blokcs are managed by std::unique_ptr
       ~Piece() = default;
      //disable copy and move operation
      Piece() = delete;
    Piece operator=(const Piece & other) = delete;
    Piece operator=(Piece & other) = delete;
    Piece(Piece && other) = delete;
    Piece & operator=(Piece && other) = delete;
    //return true when every block has been sucessfully removed
    bool isFull() const;
    //check whether the downloaded Piece data has the expected
    bool isHashChecked(const std::string&dataToFile);

    //check whether the downloaded Pice data has the expected  SHA-1 hash
    bool isHashChecked(const std::string&dataToFile) const;
    //finds a block that is ready to be requested
    const std::string requestBlock();
    //called when a PIECE message is recieved from a peer
    void fillData(int begin, const std::string&data);
    //haveBlockTORequest() returns true if there is at least one block that can be requested
    bool haveBlockToRequest();
    //reset the piece's blocks
    void resetAllBlockstoMissing();
    //convert all retrieval blocks into one contiguous string
    void fillDataToStr(std::string& dataToFile);

 };
