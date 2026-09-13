#include "../includes/Message.hpp"

#include <stdexcept>
#include <netinet/in.h>

#include "../includes/utils.hpp"
//constructor 1:converts a raw bittorent message receive from the network into message object
Message::Message(const std::string &str):lenght(getIntFromStr(str)) {
   // a length of o means this is a keep-alive message
   if (lenght) {
      //the message Id is immediately  stored after  the 4-bytes length prefix

      MessageType = getMessageType(str);
      //extract everything after the message id  as the payload
      payload = getPayloadFromMessage(str);

   } else {
      MessageType = eMeesageType::KeepAlive;
   }
}
//construct a message that we want to send
Message::Message(const eMeesageType e, const std::string &str):lenght(1+str.size()), MessageType(e), payload(str) {


}
//extract the message ID
//the first 4 bytes are the length prefix
eMeesageType Message::getMessageType(const std::string&str)  {
  return static_cast<eMeesageType>(static_cast<unsigned char>(str[4]));
}
//extract the payload from a received message
const std::string Message::getPayloadFromMessage(const std::string &payload) {
   //the first 4 bytes are the length prefix should be equal to the value stored in length
   if (static_cast<int>(payload.size() -4)!=lenght) {
      throw std::invalid_argument("Message payload length mismatch");
   }
   //length includes the 1 byte message ID therefore the actual payload size is length -1
   std::string res(lenght - 1, '\0');
   //payload starts at byte 5
   for (int i =0; i < lenght - 1; i++) {
      res[i] = payload[i+5];
   }
   return res;
}
//convert the message object into the raw bytes that can be sent through  tcp
std::string Message::getMessageStr() const {
   return  intToBytes(htonl(lenght)) + static_cast<char>(MessageType) +payload;
}
//check whether  this message is a Bittorent keep alive
bool Message::isKeepAlive() const {
   return lenght == 0;
}
//return the Bittorent message type
eMeesageType Message::geMessageType() const {
   return MessageType;
}
//return the message payload
 std::string Message::getPayload() const {
   return payload;
}
int Message::getLenght() const {
   return lenght;
}

