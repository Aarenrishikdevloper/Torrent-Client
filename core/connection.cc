#include  "../includes/connection.hpp"

#include <stdexcept>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <chrono>
#include <cstring>

#include "utils.hpp"

int createConnection(const std::string &ip, const long long port) {
    //create a TCP IPv4 socket
    int sockfd = socket(AF_INET, SOCK_STREAM,0);
    if (sockfd == -1) {
        throw std::runtime_error("Error creating socket");
    }
    //create the server adress structure
    sockaddr_in serverAddr = {};
    //specify ipV4
     serverAddr.sin_family = AF_INET;
    //convert the port from host byte order to network byte order
    serverAddr.sin_port = htons(port);
    //convert the ip address from text to binary from
     if (inet_pton(AF_INET,ip.c_str(), &serverAddr.sin_addr) <=0) {
         throw std::runtime_error("Error creating inet_pton");
     }
     //Put the Socket into NON_Blocking mode
     int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("Error getting flags for socket connection");
    }
    //add O_NONBLOCK to the existing flags
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        throw std::runtime_error("Error setting flags for socket connection");
    }
    //attempt to connect to peer
    int connectResult = connect(sockfd, reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr));
  if (connectResult == -1) {
      if (errno != EINPROGRESS)
          throw std::runtime_error("Error connecting to server");
      //write up to  5 sec for the connection to complete
      fd_set writeset;
      FD_ZERO(&writeset);
      FD_SET(sockfd, &writeset);
      timeval timeout{5,0};
      int selectResult = select(sockfd + 1, nullptr, &writeset, nullptr, &timeout);
      if (selectResult == -1) {
          throw std::runtime_error("select() error: " + std::string(strerror(errno)));
      }
      if (selectResult == 0) {
          throw std::runtime_error("Connection time out");
      }
      //check whether the connection actually suceeded
      int socerr = 0;
      socklen_t errlen = sizeof(socerr);
      if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &socerr, &errlen) == -1 || socerr != 0) {
          throw std::runtime_error("Connection failed: " + std::string(strerror(socerr)));
      }
  }


    //connection has completed
    //change the Socket to blocking mode
    flags &= ~O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        throw std::runtime_error("Error setting flags to blocking mode");
    }
    //return the socket file descriptor
    //to communicate with bittorent peer
    return sockfd;
}

void sentData(const int sockfd, const std::string &msg) {
    //set a 3 sec timeout for sending
    timeval timeout{};
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    //SO_SNDTIMEO controls how long send can wait
    if (setsockopt(
        sockfd,
        SOL_SOCKET,
        SO_SNDTIMEO,
        &timeout,
        sizeof(timeout)) == -1) {
        throw std::runtime_error("setsockopt timeout");
    }
    //sent string as raw bytes looping until all bytes are sent
    size_t totalSent  =0;
    while (totalSent < msg.size()) {
        ssize_t sent  = send(sockfd, msg.c_str()+totalSent, msg.size()-totalSent,0);
        if (sent == -1) {
            throw std::runtime_error("Error sending data");
        }
        totalSent += sent;
    }
}
static  void recVData(int sockfd, char*buffer, int size) {
    int byteRecived = 0;
    int SumRecived = 0;
    //start our own 5 sec timeout clock
    auto startTime =std::chrono::steady_clock::now();
    //continue unitl the requested no of bytes has been recieved
    while (SumRecived < size) {
        //calculate how much time has passed
        auto diff = std::chrono::steady_clock::now() - startTime;
        //convert ellpased time into miilisecounds
        if (std::chrono::duration<double, std::milli>(diff).count() > 5000) {
            throw std::runtime_error("read timeout");
        }
        //receive data
        byteRecived = recv(
            sockfd,
            buffer +SumRecived,
            size - SumRecived,
            0
            );
        if (byteRecived == -1) {
            throw std::runtime_error("recv() error: " + std::string(strerror(errno)));
        }
        if (byteRecived == 0) {
            throw std::runtime_error("Connection closed by peer");
        }
        // Socket timeout expired — retry, our manual clock controls the real timeout
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            continue;
            throw std::runtime_error("recv() error: " + std::string(strerror(errno)));
        }
        //add newly add data to the total
        SumRecived += byteRecived;
    }

}

std::string reciveData(int soketfd, int size) {

    //case 1 we do not know the size of the incoming message
   if (!size) {
       //read and decode the 4-byte length
       char prefixBuf[4];
       recVData(soketfd, prefixBuf, 4);
       const std::string prefix(prefixBuf,4);
       size = getIntFromStr(prefix);
       if (size == 0) return prefix;
       //receive the payload
       std::unique_ptr<char[]> payload;
       try {
           // ReSharper disable once CppJoinDeclarationAndAssignment
           payload = std::make_unique<char[]>(size);
       } catch (const std::bad_alloc&) {
           throw std::runtime_error("Error allocating memory");
       }
       recVData(soketfd, payload.get(), size);
       //return prefix + payload as one string
       std::string result(prefix);
       result.append(payload.get(), size);
       return result;

   }

    std::string result(size, '\0');
    recVData(soketfd, result.data(), size);
    return result;
}
