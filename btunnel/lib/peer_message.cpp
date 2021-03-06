// peer_message.cpp
// Author: Allen Porter <allen@thebends.org>

#include "peer_message.h"
#include <err.h>
#include <string>
#include <ynet/buffer.h>
#include <iostream>
#include "encoding.h"
#include "peer.h"

using namespace std;

namespace btunnel {

MessageReader::MessageReader(Peer* peer) : peer_(peer) {
}

MessageReader::~MessageReader() {
}

int MessageReader::Read(int sock, ynet::ReadBuffer* buffer) {
  cout << "MessageReader::Read on " << sock << endl; 
  int8_t type = -1;
  if (!buffer->Read((char*)&type, 1)) {
    return 0;
  }
  int nbytes;
  if (type == REGISTER) {
    nbytes = HandleRegister(sock, buffer);
  } else if (type == UNREGISTER) {
    nbytes = HandleUnregister(sock, buffer);
  } else if (type == FORWARD) {
    nbytes = HandleForward(sock, buffer);
  } else {
    warnx("Invalid message type: 0x%02x", type);
    return -1;
  }
  if (nbytes == 0) {
    // Push back the "type" field
    buffer->Unadvance(1);  // type
    return 0;
  }
  return nbytes;
}

int MessageReader::HandleRegister(int sock, ynet::ReadBuffer* buffer) {
  RegisterRequest request;
  int nbytes = ReadRegister(buffer, &request);
  if (nbytes > 0) {
    if (!peer_->Register(sock, &request)) {
      return -1;
    }
  }
  return nbytes;
}

int MessageReader::HandleUnregister(int sock, ynet::ReadBuffer* buffer) {
  UnregisterRequest request;
  int nbytes = ReadUnregister(buffer, &request);
  if (nbytes > 0) {
    if (!peer_->Unregister(sock, &request)) {
      return -1;
    }
  }
  return nbytes;
}

int MessageReader::HandleForward(int sock, ynet::ReadBuffer* buffer) {
  ForwardRequest request;
  int nbytes = ReadForward(buffer, &request);
  if (nbytes > 0) {
    if (!peer_->Forward(sock, &request)) {
      return -1;
    }
  }
  return nbytes;
}

int ReadRegister(ynet::ReadBuffer* buffer, RegisterRequest* request) {
  int nbytes = 0;
  int32_t service_id;
  if (!buffer->Read((char*)&service_id, sizeof(int32_t))) {
    return 0;
  }
  nbytes += sizeof(int32_t);
  request->service_id = ntohl(service_id);

  int ret = ReadString(buffer, kMaxNameLen, &request->name);
  if (ret <= 0) {
    buffer->Unadvance(nbytes);
    return ret;
  }
  nbytes += ret;

  ret = ReadString(buffer, kMaxTypeLen, &request->type);
  if (ret <= 0) {
    buffer->Unadvance(nbytes);
    return ret;
  }
  nbytes += ret;

  ret = ReadString(buffer, kMaxTextLen, &request->txt_records);
  if (ret < 0) {
    buffer->Unadvance(nbytes);
    return ret;
  }
  nbytes += ret;
  return nbytes;
}

int WriteRegister(ynet::WriteBuffer* buffer, const RegisterRequest& request) {
  int nbytes = 0;
  int32_t service_id = htonl(request.service_id);
  if (!buffer->Write((char*)&service_id, sizeof(int32_t))) {
    return -1;
  }
  nbytes += sizeof(int32_t);
  int ret = WriteString(buffer, kMaxNameLen, request.name);
  if (ret <= 0) {
    return -1;
  }
  nbytes += ret;

  ret = WriteString(buffer, kMaxTypeLen, request.type);
  if (ret <= 0) {
    return -1;
  }
  nbytes += ret;

  ret = WriteString(buffer, kMaxTextLen, request.txt_records);
  if (ret <= 0) {
    return -1;
  }
  nbytes += ret;
  return nbytes;
}

int ReadUnregister(ynet::ReadBuffer* buffer, UnregisterRequest* request) {
  int32_t service_id;
  if (!buffer->Read((char*)&service_id, sizeof(int32_t))) {
    return 0;
  }
  request->service_id = ntohl(service_id);
  return sizeof(int32_t);
}

int WriteUnregister(ynet::WriteBuffer* buffer,
                    const UnregisterRequest& request) {
  int32_t service_id = htonl(request.service_id);
  if (!buffer->Write((char*)&service_id, sizeof(int32_t))) {
    return -1;
  }
  return sizeof(int32_t);
}

int ReadForward(ynet::ReadBuffer* buffer, ForwardRequest* request) {
  int nbytes = 0;
  int32_t service_id;
  if (!buffer->Read((char*)&service_id, sizeof(int32_t))) {
    return 0;
  }
  nbytes += sizeof(int32_t);
  request->service_id = ntohl(service_id);

  int32_t session_id;
  if (!buffer->Read((char*)&session_id, sizeof(int32_t))) {
    buffer->Unadvance(nbytes);
    return 0;
  }
  nbytes += sizeof(int32_t);
  request->session_id = ntohl(session_id);

  int ret = ReadString(buffer, kMaxBufLen, &request->buffer);
  if (ret <= 0) {
    buffer->Unadvance(nbytes);
    return ret;
  }
  nbytes += ret;
  return nbytes;
}

int WriteForward(ynet::WriteBuffer* buffer, const ForwardRequest& request) {
  int nbytes = 0;
  int32_t service_id = htonl(request.service_id);
  if (!buffer->Write((char*)&service_id, sizeof(int32_t))) {
    return -1;
  }
  nbytes += sizeof(int32_t);

  int32_t session_id = htonl(request.session_id);
  if (!buffer->Write((char*)&session_id, sizeof(int32_t))) {
    return -1;
  }
  nbytes += sizeof(int32_t);

  int ret = WriteString(buffer, kMaxBufLen, request.buffer);
  if (ret <= 0) {
    return -1;
  }
  nbytes += ret;
  return nbytes;
}

}  // namespace btunnel
