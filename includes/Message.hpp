#pragma once
#include <string>

enum class eMeesageType : int {
    KeepAlive = -1,
    Choke = 0,
    Unchoke = 1,
    Interested = 2 ,
    NotInterested = 3 ,
    Have = 4,
    Bitfield = 5,
    Request = 6,
    Piece  = 7,
    Cancel =0,



};
class Message {
private:
    int lenght;
    eMeesageType MessageType;
    std::string payload;
    inline eMeesageType getMessageType(const std::string& payload);
    const std::string getPayloadFromMessage(const std::string& payload);
public:
    explicit Message(const std::string& str);
    explicit Message(const eMeesageType e,  const std::string& str = "");
    ~Message() = default;
     std::string getMessageStr()const;
    bool isKeepAlive()const;
    int getLenght()const;
    eMeesageType geMessageType()const;
     std::string getPayload()const;


};