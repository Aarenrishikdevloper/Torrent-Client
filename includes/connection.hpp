#pragma once

#include <string>
//create an TCP connection to an IP address and port
int createConnection(const std::string &ip, const long  long port );
//sent data through an already connected Socket
void sentData(const int sockfd, const std::string& msg);
//receive data from a socket
std::string reciveData(int soketfd, int size);
