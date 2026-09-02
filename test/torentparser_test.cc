#include <iostream>
#include <stdexcept>

#include "../includes/TorrentParser.hpp"

int main()
{
    try
    {
        // ----------------------------------------------------
        // Load the torrent file
        // ----------------------------------------------------

        TorrentParser torrent("test/test.iso.torrent");


        // ----------------------------------------------------
        // Basic torrent information
        // ----------------------------------------------------

        std::cout << "===== Torrent Information =====\n";

        std::cout << "Announce: "
                  << torrent.getAnnounce()
                  << '\n';

        std::cout << "Info Hash: "
                  << torrent.getInfoHash()
                  << '\n';

        std::cout << "Piece Length: "
                  << torrent.getPieceLenght()
                  << " bytes\n";


        // ----------------------------------------------------
        // Check torrent type
        // ----------------------------------------------------

        if (torrent.isSingleFile())
        {
            std::cout << "\nTorrent type: Single-file\n";

            const auto& file = torrent.getSingleFile();

            std::cout << "File name: "
                      << file.name
                      << '\n';

            std::cout << "File size: "
                      << file.lenght
                      << " bytes\n";
        }
        else
        {
            std::cout << "\nTorrent type: Multi-file\n";

            const auto& multi = torrent.getMultiFile();

            std::cout << "Directory: "
                      << multi.dirName
                      << '\n';

            std::cout << "Number of files: "
                      << multi.files.size()
                      << '\n';
        }


        // ----------------------------------------------------
        // Trackers
        // ----------------------------------------------------

        std::cout << "\n===== Trackers =====\n";

        const auto& trackers = torrent.getAnnounceList();

        for (std::size_t i = 0; i < trackers.size(); ++i)
        {
            std::cout << i << ": "
                      << trackers[i]
                      << '\n';
        }


        // ----------------------------------------------------
        // Piece information
        // ----------------------------------------------------

        std::cout << "\n===== Pieces =====\n";

        std::cout << "Piece hash data size: "
                  << torrent.getPieces().size()
                  << " bytes\n";

        std::cout << "Number of pieces: "
                  << torrent.getPieces().size() / 20
                  << '\n';


        std::cout << "\nParser test PASSED\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Parser test FAILED: "
                  << e.what()
                  << '\n';

        return 1;
    }
}