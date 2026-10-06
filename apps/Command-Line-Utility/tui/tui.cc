#include "tui.hpp"
#include <ncurses.h>
#include <thread>
#include <TorrentClient.hpp>

namespace {
    constexpr  int COLOR_TITLE = 1;
    constexpr  int COLOR_BORDER=2;
    constexpr  int COLOR_PROGRESS=3;
    constexpr int COLOR_STATUS =4;
    constexpr int COLOR_ERROR=5;

}
TorrentTui::TorrentTui(){}
TorrentTui::~TorrentTui() {
    quiteRequested = true;
    //wait for the doenload thread to finish
    if (downloadThread.joinable()) {
        downloadThread.join();
    }
    pieceManager.store(nullptr);
    client.reset();

    endwin();
}


void TorrentTui::run() {
    NUllBuffer nullbuffer;
    std::streambuf*oldcout = std::cout.rdbuf(&nullbuffer);
    std::streambuf*oldcerr = std::cerr.rdbuf(&nullbuffer);
    //intialize screen
    initscr();
    //allow chracter input without waiting
    cbreak();
    //do not eco type chracter automaticaaly
    noecho();
    //allow chracter key and other arrow key
    keypad(stdscr, TRUE);
    //make gtech non blockin
    timeout(100);
    //hide cursor
    curs_set(0);
    //enable colors
    start_color();
    init_pair(COLOR_TITLE, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_BORDER, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_PROGRESS, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_STATUS, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_ERROR, COLOR_RED, COLOR_BLACK);
    while (!quiteRequested) {
        int ch = getch();
        if (ch != ERR) {
            handleInput(ch);

        }
        draw();

    }
    endwin();
    std::cout.rdbuf(oldcout);
    std::cerr.rdbuf(oldcerr);


}

void TorrentTui::handleInput(int ch) {
    //q is quiet
    if (ch == 'q' || ch == 'Q') {
        quiteRequested = true;
        return;
    }
    if ( ch == '\n' || ch == KEY_ENTER ) {
        if (!downloading && !finished) {
            startDownloading();
        }
        return;
    }
    if (ch == '\t') {
        activeInput =(activeInput + 1) %2;
        if (activeInput == 0) {
            torrentCurosr = torrentPath.size();
        }else {
            downloadCurosr =downloadPath.size();
        }
        return;
    }
    if (ch == KEY_BACKSPACE ||ch == 127  || ch==8 ||(ch >=32 && ch<=126)) {
        handleTextInput(ch);
        return;
    }
    if (ch == KEY_LEFT || ch == KEY_RIGHT) {
        moveCursor(ch);
        return;
    }
    handleTextInput(ch);

}

void TorrentTui::handleTextInput(int ch) {
    std::size_t*cursor;
    std::string*input;
    if (activeInput == 0) {
        input = &torrentPath;
        cursor = &torrentCurosr;
    }else {

        input = &downloadPath;
        cursor = &downloadCurosr;
    }
    if (ch ==KEY_BACKSPACE ||ch == 127 ||ch==8) {
        if (*cursor >0) {
            input->erase(*cursor - 1,1);
            (*cursor)--;
        }
        return;
    }
    //normal chracter
    if (ch >= 32 && ch <= 126) {
        input->insert(*cursor,1, static_cast<char>(ch));
        (*cursor)++;
    }
}

void TorrentTui::startDownloading() {
    if (torrentPath.empty()) {
        return;
    } if (!std::filesystem::exists(torrentPath)) {
        return;
    }
    //prevent multiple download
    if (downloading) {
        return;
    }
    if (downloadThread.joinable()) {
        downloadThread.join();
    }
    clearError();
    downloading = true;
    finished = false;
    //run the downlaod task run in background
    downloadThread = std::thread(&TorrentTui::runDownload, this);

}

void TorrentTui::runDownload() {
    try {
        client = std::make_unique<TorrentClient>(torrentPath.c_str(), downloadPath.c_str(), false);
        pieceManager.store(&client->getPieceManager());
        client->run();
        finished = true;

    } catch (const std::exception &e) {
        finished = false;
        setError(e.what());
        pieceManager.store(nullptr);
        client.reset();

    }catch (...) {
        finished = false;
        setError("Unknown exception");
        pieceManager.store(nullptr);
        client.reset();
    }
    downloading = false;
}

void TorrentTui::moveCursor(int ch) {
    std::size_t*cursor;
    std::string*input;
    if (activeInput == 0) {
        cursor = &torrentCurosr;
        input = &torrentPath;
    }else {
        cursor = &downloadCurosr;
        input = &downloadPath;
    }
    if (ch == KEY_LEFT) {
        if (*cursor >0) {
            --(*cursor);
        }
    }
    if (ch == KEY_RIGHT) {
        if (*cursor < input->size()) {
            ++(*cursor);
        }
    }

}

void TorrentTui::draw() {
    erase();
    int height;
     int width;
    getmaxyx(stdscr, height, width);
    //do not draw  if terminal is small
    if (width < 70 || height < 25) {
        mvprintw(
            0,
            0,
            "Terminal too small please resize"
            );
          refresh();
        return;
    }
    //outer boder
    attron(COLOR_PAIR(COLOR_BORDER));
    box(
        stdscr,
        0,
        0
        );
    attroff(COLOR_PAIR(COLOR_BORDER));
    attron(COLOR_PAIR(COLOR_TITLE) | A_BOLD);
    mvprintw(
        2,
        (width-15) /2,
         "TORRENT CLIENT"
        );
    attroff(COLOR_PAIR(COLOR_TITLE) | A_BOLD);
    mvprintw(
          5,
          4,
          "Torrent File"
        );
     mvprintw(6,4, "> ");
    if (activeInput == 0) {
        attron(A_REVERSE);

    }
    mvprintw(
        6,
        6,
        "%-*s",
        width-12,
        torrentPath.c_str()
        );
    if (activeInput == 0) {
        attroff(A_REVERSE);
    }
    mvprintw(
         9,
         4,
         "Download Destination"

        );
    mvprintw(
        10,4 , "> "
        );
    if (activeInput == 1) {
        attron(A_REVERSE);
    }
    mvprintw(
        10,
        6,
        "%-*s",
        width-12,
        downloadPath.c_str()
        );
    if (activeInput == 1) {
        attroff(A_REVERSE);
    }
    mvprintw(
         13,
         5,
         "[ENTER] Start"
        );
    mvprintw(
        13,
        28,
        "[Q] Quit"
        );
    mvhline(15,3, ACS_HLINE, width -6);
    DowloadStats stats {};
    PieceManager*pm = pieceManager.load();
    if (pm) stats = pm->getStats();
    attron(A_BOLD);
    mvprintw(17,4, "Status:");
    attroff(A_BOLD);
    if (downloading) {
        attron(COLOR_PAIR(COLOR_STATUS));
        mvprintw(
              17,13,"Downloading"
            );
        attroff(COLOR_PAIR(COLOR_STATUS));
    }
    else if (finished) {
       attron(COLOR_PAIR(COLOR_PROGRESS)| A_BOLD);
        mvprintw(
            17,13,"Download finished");
        attroff(COLOR_PAIR(COLOR_PROGRESS)| A_BOLD);
    } else if (hasError) {
        attron(COLOR_PAIR(COLOR_ERROR)| A_BOLD);
        mvprintw(17, 13,"Error");
        attroff(COLOR_PAIR(COLOR_ERROR)| A_BOLD);
        std::string message = getError();
        int maxlen = width - 8;
        if (static_cast<int>(message.size()) > maxlen) {
            message = message.substr(0, maxlen - 3);
        }
        attron(COLOR_PAIR(COLOR_ERROR)| A_BOLD);
        mvprintw(18, 4, "%s", message.c_str());
        attroff(COLOR_PAIR(COLOR_ERROR)| A_BOLD);
    }
    else {
        attron(COLOR_PAIR(COLOR_PROGRESS)| A_BOLD);
        mvprintw(17,13, "Waiting");
        attroff(COLOR_PAIR(COLOR_PROGRESS)| A_BOLD);
    }
    if (stats.downloadBytes > 0 || stats.complete) {
        drawProgress(stats, width);
    }
    mvprintw(
        height-2,4,"Press Q to quit"
         );

  refresh();

}

void TorrentTui::drawProgress(const DowloadStats &stats, int width) {
    double progress =0.0;
    if (stats.totalbytes > 0) {
        progress = static_cast<double>(stats.downloadBytes) / static_cast<double>(stats.totalbytes);
    }
    if (progress < 0.0) {
        progress = 0.0;
    }
    if (progress > 1.0) {
        progress = 1.0;
    }
    const int barWidth =  width - 12;
    const int filled = static_cast<int>(barWidth * progress);
    mvprintw(
        20,4,"["
        );
    attron(COLOR_PAIR(COLOR_PROGRESS));
    for (int i=0; i<filled; i++) {
        addch('#');
    }
    attroff(COLOR_PAIR(COLOR_PROGRESS));
    for (int i=filled; i<barWidth; i++) {
        addch('-');
    }
    addch(']');
    mvprintw(
        22,4,"Progress: %3d%%",
        static_cast<int>(progress * 100)
        );
    mvprintw(
        23,4,"Downloaded: %llu / %llu", static_cast<unsigned long long>(stats.downloadBytes), static_cast<unsigned long long>(stats.totalbytes)
        );
    mvprintw(
        24,4,"Speed: %.2f MB/s", stats.speedMBps
        );
    mvprintw(
        25,4,"Pieces: %zu / %zu", stats.completedPices, stats.totalPics
        );
}

void TorrentTui::setError(const std::string &message) {
    {
        std::lock_guard<std::mutex> lock(errorMutex);
        errorMessage = message;
    }
    hasError = true;
}
std::string TorrentTui::getError() {
    std::lock_guard<std::mutex> lock(errorMutex);
    return errorMessage;
}
void TorrentTui::clearError() {
    hasError = false;
    std::lock_guard<std::mutex> lock(errorMutex);
    errorMessage.clear();
}