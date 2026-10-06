#pragma once
#include <atomic>
#include <string>
#include <thread>
#include "../../../includes/PieceManager.hpp"
#include "../../../includes/TorrentClient.hpp"
#include <streambuf>
#include <fmt/core.h>
#include <iostream>

class TorrentTui {
   private:
    std::unique_ptr<TorrentClient> client;

    std::atomic<PieceManager *> pieceManager{nullptr};
    std::string torrentPath;
    std::string downloadPath = "./Downloads";
    std::atomic<bool>downloading{false};
    std::atomic<bool> finished{false};
    std::atomic<bool>quiteRequested{false};
    std::thread downloadThread;
    int activeInput =0;
    std::size_t torrentCurosr = 0;
    std::size_t downloadCurosr = 0;
    void draw();
    void handleInput(int ch);
    void startDownloading();
    void runDownload();
    void drawProgress(const DowloadStats & stats, int width );
    void moveCursor(int ch);
    void handleTextInput(int ch);
    std::atomic<bool> hasError{false};
    std::mutex errorMutex;
    std::string errorMessage;
    void setError(const std::string & message);
    std::string getError();
    void clearError();
    public:
      explicit  TorrentTui(  );
      ~TorrentTui();
       TorrentTui(const TorrentTui&) = delete;

       TorrentTui& operator=(const TorrentTui&) = delete;
       void run();


};
class  NUllBuffer:public std::streambuf {
protected:
    int overflow(int_type c) override {
        return traits_type::not_eof(c);
    }



};
