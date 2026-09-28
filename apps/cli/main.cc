#include <filesystem>
#include <iostream>
#include "../../cxxopts/cxxopts.hpp"
#include "../../includes/TorrentClient.hpp"

int main (int argc, char ** argv) {
    //check command-line-argument
    if (argc < 2) {
        std::cerr<<"Try:\n"<< "./torrent_client -h"<<std::endl;
        return 1;
    }
    //determine the default directory
    //start the current directory as a fallback
    std::string defaultDownloadPath("/");
    //get the user home variable
    const char* homepath = std::getenv("HOME");
    if (homepath) {
        defaultDownloadPath = std::string(homepath) + "/Downloads/";
        std::filesystem::path directoryPath(defaultDownloadPath);
        //check whether ~/Downloads exist if  not then use current directory
        if (!std::filesystem::exists(directoryPath) ||!std::filesystem::is_directory(directoryPath)) {
            defaultDownloadPath = "./";
        }

    }
    std::string torrentPath;
    std::string downloadPath;
    try {
         //create the command-line options
        cxxopts::Options options("torrent-cli", "BitTorrent Client");
        //define command line options
        // -t torrent file
        options.add_options()(
            "t, torrent", "Location of .torrent file",
             cxxopts::value<std::string>()
            )
        // -d  / --direction

        (
             "d, directory", "Directory where files are saved",
             cxxopts::value<std::string>()
        )
        // -h / --help
        (
            "h, help",
            "(Print Usage)"
            );
        //parse command line argument
         auto result = options.parse(argc, argv);
        //handle help
        if (result.count("help")) {
            std::cerr << options.help() << std::endl;
            return 0;
        }
        downloadPath = result["directory"].as<std::string>();
        //make sure the download path becomes home/usr/ if it was lie home/usr
        if (!downloadPath.empty() && downloadPath.back() != '/') {
            downloadPath.push_back('/');
        }

        //get the .torrent file path
        torrentPath = result["torrent"].as<std::string>();



    } catch (const std::exception& e) {
        std::cerr<< "command line error" << e.what()<<std::endl;
        return 2;

    }
    try {
        //start the Bittorent client
        TorrentClient client(
              torrentPath.c_str(),  downloadPath.c_str()

            );
        client.run();
    } catch (const std::exception& e) {
         std::cerr<< "command line error" << e.what()<<std::endl;
        return 1;
    }
    return 0;


}
