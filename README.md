# BitTorrent Client

![C++23](https://img.shields.io/badge/C%2B%2B-23-blue) ![ncurses](https://img.shields.io/badge/TUI-ncurses-green) ![Platform](https://img.shields.io/badge/tested%20on-WSL-lightgrey)

A multithreaded BitTorrent client written in C++ — built from scratch to gain in-depth knowledge of networking and peer-to-peer protocols. Provides a command-line interface and a terminal user interface (TUI) built with ncurses.

---

## Project goals

The primary aim is to gain hands-on experience with low-level networking and the BitTorrent protocol (BEP 3) in C++ — including multi-threaded peer connections, piece scheduling, and wire-protocol message framing. The dual CLI/TUI architecture keeps the core logic cleanly separated in `libtorrent_core.a`, shared by both front-ends.

---

## Architecture



The client is built around a shared core engine  that handles all backend work: metadata parsing, tracker communication, peer connections, and piece management. Two front-ends, a CLI and an ncurses TUI, sit on top of it and only deal with input and display.

```text
                    +----------------------+
                    |      .torrent file   |
                    |  (announce URL,      |
                    |   info_hash, pieces) |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    |   Parse Metadata     |
                    |  - tracker URL(s)    |
                    |  - piece hashes      |
                    |  - file layout       |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    |  Peer Discovery      |
                    |  (Tracker Announce)  |
                    +----------+-----------+
                               |
                 +-------------+-------------+
                 |                           |
                 v                           v
        +-----------------+         +-----------------+
        |  HTTP Tracker   |         |   UDP Tracker   |
        |  GET /announce  |         |  1. connect     |
        |  ?info_hash=..  |         |  2. announce    |
        |  &peer_id=..    |         |  (binary proto) |
        |  &port=..       |         |                 |
        +--------+--------+         +--------+--------+
                 |                           |
                 +-------------+-------------+
                               |
                               v
                    +----------------------+
                    |   Peer List          |
                    |  [ip:port, ip:port]  |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    |  Connect to Peers    |
                    |  (TCP)               |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    |  Handshake           |
                    |  - protocol string   |
                    |  - info_hash         |
                    |  - peer_id           |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    |  Exchange Messages   |
                    |  - bitfield          |
                    |  - interested        |
                    |  - unchoke           |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    |  Piece Selection     |
                    |  (e.g. rarest-first) |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    |  Request Blocks      |
                    |  (request piece,     |
                    |   offset, length)    |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    |  Receive Blocks      |
                    |  (piece messages)    |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    |  Verify Piece        |
                    |  SHA-1 vs metadata   |
                    +----------+-----------+
                               |
                  +------------+------------+
                  |                         |
              hash OK                   hash FAIL
                  |                         |
                  v                         v
        +------------------+      +------------------+
        |  Write to Disk   |      |  Discard piece,  |
        +--------+---------+      |  re-request      |
                 |                +---------+--------+
                 |                          |
                 |                          +----> back to Piece Selection
                 v
        +------------------+
        | All pieces done? |
        +--------+---------+
                 |
         +-------+-------+
         |               |
        no              yes
         |               |
         v               v
   back to Piece   +----------------+
   Selection       |  Download      |
                   |  Complete      |
                   +----------------+
```

**Key components:**

- **CLI / TUI front-ends** — `main.cc` parses arguments and launches the client either as a plain command-line utility or as an ncurses TUI
- **TorrentClient** (`TorrentClient.cc`) — orchestrates all subsystems
- **Peer discovery** — `PeerRetrieval.cc` announces to trackers; `PeerQueue.cc` manages the peer pool; `PeerConnection.cc` handles per-peer I/O threads
- **Piece assembly** — `PieceManager.cc` schedules, verifies (SHA-1), and writes pieces; `Piece.cc` holds block state
- **Torrent metadata** — `TorenParser.cc` decodes `.torrent` files via `bencoder.hpp`; `utils.cc` handles hashing

---

## How to build

Requires a C++23-capable compiler (GCC 13+ or Clang 16+ recommended). Install dependencies first (on Debian/Ubuntu, including WSL):

```bash
sudo apt install cmake libcurl4-openssl-dev libssl-dev libncurses-dev
```

Then build everything:

```bash
cmake -B build/ -S . && make -C build/ -j$(nproc)
```

The binary will appear in `build/`:

| Binary | Description |
|---|---|
| `torrent-cli` | Command-line interface, plus the ncurses TUI via `--tui` |

> Tested on WSL (Windows Subsystem for Linux).

---

## Command-line usage

```text
torrent-cli [OPTION...]

  -t, --torrent arg    Location of .torrent file
  -d, --directory arg  Directory where files are saved
      --tui            Launch terminal user interface
  -h, --help           Print usage
```

Examples:

```bash
# Plain command-line download
torrent-cli -t ./file.torrent -d ~/Downloads


```

---

## Libraries

| Library | Purpose |
|---|---|
| [ncurses](https://invisible-island.net/ncurses/) | Terminal user interface (TUI) |
| [libcurl](https://curl.se/libcurl/) | HTTP requests to trackers for peer retrieval |
| [Crypto++](https://cryptopp.com/) | SHA-1 hashing for piece verification |
| [bencode.hpp](https://github.com/jimporter/bencode.hpp) | Lightweight bencoded data parser and generator |
| [spdlog](https://github.com/gabime/spdlog) | Fast, header-only C++ logging library |
| [cxxopts](https://github.com/jarro2783/cxxopts) | Lightweight C++ command-line option parser |

---

## Roadmap

- [x] Torrent file parsing (bencode)
- [x] Tracker communication (HTTP announce)
- [x] Peer connection and wire protocol
- [x] Multi-threaded piece downloading
- [x] SHA-1 piece verification
- [x] CLI interface
- [x] TUI (ncurses)
- [ ] Seeding
