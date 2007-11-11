// tunnel.cpp
// Author: Allen Porter <allen@thebends.org>

#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <sysexits.h>
#include <sys/types.h>

#include <iostream>
#include <map>

#include <ythread/callback-inl.h>

#include "tunnel.h"
#include "tcpserver.h"

using namespace std;

namespace btunnel {

typedef map<int, int> SockMap;

class TcpTunnel : public Tunnel {
 public:
  TcpTunnel(yhttpserver::Select* select,
            struct in_addr remote_addr,
            uint16_t remote_port,
            uint16_t local_port) : select_(select) {
    ConnectionCallback* accept_callback =
      ythread::NewCallback(this, &TcpTunnel::ClientConnected);
    server_ = new TCPServer(select, local_port, accept_callback);

    // Setup remote address
    bzero(&remote_, sizeof(struct sockaddr_in));
    remote_.sin_family = AF_INET;
    memcpy(&remote_.sin_addr, &remote_addr, sizeof(struct in_addr));
    remote_.sin_addr.s_addr = remote_addr.s_addr;
    remote_.sin_port = htons(remote_port);
  }

  virtual ~TcpTunnel() {
    Stop();
  }

  virtual void Start() {
    server_->Start();
  }

  void ClientConnected(struct Connection* client) {
    cout << "Accepted connection from " << inet_ntoa(client->addr.sin_addr)
         << endl;
    assert(pairs_.count(client->sock) == 0);

    // We've received a client connection; Establish a new connection to the
    // remote address
    int remote_sock;
    if ((remote_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      close(client->sock);
      err(EX_OSERR, "socket()");
    }
/*
    if ((fcntl(remote_sock, F_SETFL, O_NONBLOCK)) == -1) {
      err(EX_OSERR, "fcntl()");
    }
*/
    cout << "Openining new remote connection" << endl;
    if (connect(remote_sock, (const struct sockaddr*)&remote_,
                sizeof(struct sockaddr_in)) != 0) {
      err(EX_OSERR, "connect()");
    }
    cout << "Opened." << endl;
    // Setup socket pairs pairings
    SetForward(client->sock);
    SetForward(remote_sock);
    pairs_[remote_sock] = client->sock;
    pairs_[client->sock] = remote_sock;
  }

  void SetForward(int sock) {
    yhttpserver::Select::AcceptCallback* cb =
        ythread::NewCallback(this, &TcpTunnel::Read);
    select_->AddFd(sock, cb);
  }

  void Read(int sock) {
    SockMap::const_iterator iter = pairs_.find(sock);
    assert(iter != pairs_.end());
    int sock_pair = iter->second;

    char buf[BUFSIZ];
    ssize_t nread = read(sock, buf, BUFSIZ);
    if (nread == -1) {
      err(EX_OSERR, "read()");
      return;
    } else if (nread == 0) {
      cerr << "Connection closed on read" << endl;
      Close(sock);
      Close(sock_pair);
      return;
    }
    ssize_t nwrote = write(sock_pair, buf, nread);
    if (nwrote == -1) {
      err(EX_OSERR, "write()");
      return;
    } else if (nwrote == 0) {
      cerr << "Connection closed on write" << endl;
      Close(sock);
      Close(sock_pair);
      return;
    }
    assert(nwrote == nread);
  }

  void Close(int sock) {
    select_->RemoveFd(sock);
    close(sock);
    pairs_.erase(sock);
  }

  virtual void Stop() {
    server_->Stop();
  }

  struct sockaddr_in remote_;
  yhttpserver::Select* select_;
  TCPServer* server_;
  SockMap pairs_;
};

Tunnel* NewTcpTunnel(yhttpserver::Select* select,
                     struct in_addr addr,
                     uint16_t remote_port,
                     uint16_t local_port) {
  return new TcpTunnel(select, addr, remote_port, local_port);
}

}  // namespace btunnel
