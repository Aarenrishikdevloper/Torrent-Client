#include <iostream>
#include <string>


#include "../includes/TorrentParser.hpp"
#include "../includes/PeerRetriever.hpp"

int main()
{
    try
    {
        // Path to your test torrent
        const std::string torrentPath = "test/test.iso.torrent";

        // Create torrent parser
        TorrentParser torrent(torrentPath.c_str());

        std::cout << "Torrent loaded successfully\n";

        std::cout << "Tracker: "
                  << torrent.getAnnounce() << '\n';


        std::cout << "File size: "
                  << torrent.getLengthOne() << '\n';

        // Generate a 20-byte BitTorrent peer ID.
        // This is only for testing.
        const std::string peerId =
            "-PC0001-123456789012";

        // Port on which our client claims it is listening.
        const int port = 6881;

        // We haven't downloaded anything yet.
        const long long bytesDownloaded = 0;

        // Create PeerRetriever.
        //
        // The constructor will contact the tracker.
        PeerRetriever retriever(
            peerId,
            port,
            torrent,
            bytesDownloaded
        );

        // Get peers returned by tracker.
        const auto& peers = retriever.getPeers();

        std::cout << "\nPeers received: "
                  << peers.size() << "\n";

        // Display peers
        for (const auto& peer : peers)
        {
            std::cout << "IP: "
                      << peer.first
                      << "  Port: "
                      << peer.second
                      << '\n';
        }

        std::cout << "\nTracker interval: "
                  << retriever.getInterval()
                  << " seconds\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "TEST FAILED: "
                  << e.what()
                  << '\n';

        return 1;
    }
}
