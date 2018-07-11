#ifndef SOCKETDELEGATE_54ACB222_AA47_4196_A404_95EA9195D904
#define SOCKETDELEGATE_54ACB222_AA47_4196_A404_95EA9195D904

#include <string>
#include <functional>
#include <arpa/inet.h>
#include <map>
#include <mutex>
#include <thread>
#include <list>

class SocketDelegate
{
  public:
    enum class SocketVersion
    {
      SocketVersion_IPV4,
      SocketVersion_IPV6
    };
    enum class SocketStatus
    {
      SocketStatus_Error,
      SocketStatus_Listening,
      SocketStatus_Connecting,
      SocketStatus_Connected
    };
    struct SocketInfo
    {
      int socketHandle;
      //SocketStatus status;
      SocketVersion version;
      union
      {
        sockaddr_in addr4;
        sockaddr_in6 addr6;
      };
    };

    typedef std::function<void(int,std::string&,int&)> IOCallback;
    typedef std::function<void(int)> connectCallback;
    typedef std::function<void(int,int)> newConnectionCallback;
    typedef std::function<void(int)> socketErrCallback;
    typedef std::function<void(int)> socketCloseCallback;
  public:
    SocketDelegate();
    ~SocketDelegate();
  public:
    void start();
    void shutdown();
    int createNewUnicastStreamSocket(std::string ipAddres,int port,SocketVersion ver=SocketVersion::SocketVersion_IPV4,bool bIsListened=false);
    int sendSocket(std::string str);
    int recvSocket(std::string str);
    void clearListenSocket(int socketHandle);
    void clearConnectedSocket(int socketHandle);

    void setnewConnectionCallback(int socketHandle,newConnectionCallback f);
    void setConnectCallback(int socketHandle,connectCallback f);
    void setSendCallback(int socketHandle,IOCallback f);
    void setRecvCallback(int socketHandle,IOCallback f);
    void setCloseCallback(int socketHandle,socketCloseCallback f);
    void setErrCallback(int socketHandle,socketErrCallback f);
    
  private:
    int doPoll();
    int doPollListenSocket();
    int doPollConnectedSocket();
    int setNonblock(int socket);
  private:
    std::map<int,newConnectionCallback> m_socket2newConnectionCallback;
    std::map<int,connectCallback> m_socket2ConnectCallback;
    std::map<int,IOCallback> m_socket2SendCallback;
    std::map<int,IOCallback> m_socket2RecvCallback;
    std::map<int,socketErrCallback> m_socket2ErrCallback;
    std::map<int,socketErrCallback> m_socket2CloseCallback;
    std::list<int> m_connectedSocket;

    std::mutex m_socket2newConnectionCallbackLock;
    std::mutex m_socket2ConnectCallbackLock;
    std::mutex m_socket2SendCallbackLock;
    std::mutex m_socket2RecvCallbackLock;
    std::mutex m_socket2ErrCallbackLock;
    std::mutex m_socket2CloseCallbackLock;
    std::mutex m_connectedSocketLock;

    bool m_bisRunning;
    std::thread* m_pollThread;
};

#endif

