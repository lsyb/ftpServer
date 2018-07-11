#include "SocketDelegate.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <poll.h>



//#define DEBUG_SOCKET_DELEGATE

#ifdef DEBUG_SOCKET_DELEGATE
#define DSD_printf printf
#else
#define DSD_printf ;
#endif

SocketDelegate::SocketDelegate()
  :m_bisRunning(false),
  m_pollThread(nullptr)
{

}

SocketDelegate::~SocketDelegate()
{

}

void SocketDelegate::start()
{
  m_bisRunning=true;
  m_pollThread=new std::thread(&SocketDelegate::doPoll,this);
}

void SocketDelegate::shutdown()
{
  m_bisRunning=false;
  m_pollThread->join();
  delete m_pollThread;
  m_socket2newConnectionCallbackLock.lock();
  for(std::map<int,newConnectionCallback>::iterator listenIter=m_socket2newConnectionCallback.begin();listenIter!=m_socket2newConnectionCallback.end();listenIter++)
  {
    close(listenIter->first); 
  }
  m_socket2newConnectionCallbackLock.unlock();

  m_socket2ConnectCallbackLock.lock();
  for(std::map<int,connectCallback>::iterator conIter=m_socket2ConnectCallback.begin();conIter!=m_socket2ConnectCallback.end();conIter++)
  {
    close(conIter->first);  
  }
  m_socket2ConnectCallbackLock.unlock();
}

int SocketDelegate::createNewUnicastStreamSocket(std::string ipAddress,int port,SocketDelegate::SocketVersion ver,bool bIsListened)
{
  int socketHandle=-1;
  SocketInfo info;
  switch(ver)
  {
    case SocketVersion::SocketVersion_IPV4:
      {
        socketHandle=socket(PF_INET,SOCK_STREAM,0);
        if(socketHandle<0)
        {
          printf("socket failed\n");
          return -1;
        }
        int ret=setNonblock(socketHandle);
        if(ret<0)
        {
          printf("setNonblock failed\n");
          return -1;
        }
        
        sockaddr_in addr;
        addr.sin_family=AF_INET;
        ret=inet_pton(AF_INET,ipAddress.data(),&addr.sin_addr);
        if(ret<0)
        {
          printf("inet_pton failed\n");
          close(socketHandle);
          return -1;
        }
        addr.sin_port=htons(port);
        info.socketHandle=socketHandle;
        info.version=SocketVersion::SocketVersion_IPV4;
        info.addr4=addr;

        if(bIsListened)
        {
          ret=bind(socketHandle,(const struct sockaddr*)&info.addr4,sizeof(sockaddr_in));
          if(ret<0)
          {
            printf("bind failed\n");
            close(socketHandle);
            return -1;
          }
          ret=listen(socketHandle,5);
          if(ret<0)
          {
            printf("listen failed\n");
            close(socketHandle);
            return -1;
          }
        }
        else
        {
          ret=connect(socketHandle,(const struct sockaddr*)&info.addr4,sizeof(sockaddr_in));
          if(ret<0 && errno!=EINPROGRESS)
          {
            close(socketHandle);
            return -1;
          }
        }
      }
      break;
    case SocketVersion::SocketVersion_IPV6:
      {
        socketHandle=socket(PF_INET6,SOCK_STREAM,0);
        if(socketHandle<0)
          return -1;

        int fcntlFlag;
        fcntlFlag = fcntl(socketHandle, F_GETFL, 0 );
        if(fcntlFlag<0)
          return -1;
        else
        {
          fcntlFlag|= O_NONBLOCK;
          if(fcntl(socketHandle, F_SETFL, fcntlFlag) < 0 ) 
            return -1;
        }

        sockaddr_in6 addr;
        addr.sin6_family=AF_INET6;
        int ret=inet_pton(AF_INET6,ipAddress.data(),&addr.sin6_addr);
        if(ret<0)
        {
          close(socketHandle);
          return -1;
        }
        addr.sin6_port=htons(port);
        info.socketHandle=socketHandle;
        info.version=SocketVersion::SocketVersion_IPV6;
        info.addr6=addr;


        if(bIsListened)
        {
          ret=bind(socketHandle,(const struct sockaddr*)&info.addr6,sizeof(sockaddr_in6));
          if(ret<0)
          {
            close(socketHandle);
            return -1;
          }
          ret=listen(socketHandle,5);
          if(ret<0)
          {
            close(socketHandle);
            return -1;
          }
         }
        else
        {
          ret=connect(socketHandle,(const struct sockaddr*)&info.addr6,sizeof(sockaddr_in6));
          if(ret<0 && errno!=EINPROGRESS)
          {
            close(socketHandle);
            return -1;
          }
        }
      }
      break;
  }

  return socketHandle;
}


int SocketDelegate::sendSocket(std::string str)
{
}

int SocketDelegate::recvSocket(std::string str)
{

}


void SocketDelegate::clearListenSocket(int socketHandle)
{
  m_socket2newConnectionCallbackLock.lock();
  m_socket2newConnectionCallback.erase(socketHandle);
  m_socket2newConnectionCallbackLock.unlock();
  close(socketHandle);
}

void SocketDelegate::clearConnectedSocket(int socketHandle)
{
  m_connectedSocketLock.lock();
  m_connectedSocket.remove(socketHandle);
  m_connectedSocketLock.unlock();
  
  m_socket2SendCallbackLock.lock();
  m_socket2SendCallback.erase(socketHandle);
  m_socket2SendCallbackLock.unlock();
  
  m_socket2RecvCallbackLock.lock();
  m_socket2RecvCallback.erase(socketHandle);
  m_socket2RecvCallbackLock.unlock();
  
  m_socket2ErrCallbackLock.lock();
  m_socket2ErrCallback.erase(socketHandle);
  m_socket2ErrCallbackLock.unlock();

  m_socket2CloseCallbackLock.lock();
  m_socket2CloseCallback.erase(socketHandle);
  m_socket2CloseCallbackLock.unlock();
  close(socketHandle);
}

void SocketDelegate::setnewConnectionCallback(int socketHandle,newConnectionCallback f)
{
  m_socket2newConnectionCallbackLock.lock();
  m_socket2newConnectionCallback.insert(std::pair<int,newConnectionCallback>(socketHandle,f));
  m_socket2newConnectionCallbackLock.unlock();
}


void SocketDelegate::setConnectCallback(int socketHandle,connectCallback f)
{
  m_socket2ConnectCallbackLock.lock();
  m_socket2ConnectCallback.insert(std::pair<int,connectCallback>(socketHandle,f));
  m_socket2ConnectCallbackLock.unlock();
}

void SocketDelegate::setSendCallback(int socketHandle,IOCallback f)
{
  m_socket2SendCallbackLock.lock();
  m_socket2SendCallback.insert(std::pair<int,IOCallback>(socketHandle,f));
  m_socket2SendCallbackLock.unlock();
}
void SocketDelegate::setRecvCallback(int socketHandle,IOCallback f)
{
  m_socket2RecvCallbackLock.lock();
  m_socket2RecvCallback.insert(std::pair<int,IOCallback>(socketHandle,f));
  m_socket2RecvCallbackLock.unlock();
}


void SocketDelegate::setCloseCallback(int socketHandle,socketCloseCallback f)
{
  m_socket2CloseCallbackLock.lock();
  m_socket2CloseCallback.insert(std::pair<int,socketCloseCallback>(socketHandle,f));
  m_socket2CloseCallbackLock.unlock(); 
}

void SocketDelegate::setErrCallback(int socketHandle,socketErrCallback f)
{
  m_socket2ErrCallbackLock.lock();
  m_socket2ErrCallback.insert(std::pair<int,socketErrCallback>(socketHandle,f));
  m_socket2ErrCallbackLock.unlock();
}
int SocketDelegate::doPoll()
{
  while(m_bisRunning) 
  {
    doPollListenSocket();
    doPollConnectedSocket();
  }
  return 0;
}

int SocketDelegate::doPollListenSocket()
{
  /*update long-term listen socket status*/
  m_socket2newConnectionCallbackLock.lock();
  int listenfdsNum=m_socket2newConnectionCallback.size();
  int listenfdsIndex=0;
  pollfd* listenfds=new pollfd[listenfdsNum];
  for(std::map<int,newConnectionCallback>::iterator listenIter=m_socket2newConnectionCallback.begin();listenIter!=m_socket2newConnectionCallback.end();listenIter++) 
  {
    listenfds[listenfdsIndex].fd=listenIter->first;
    listenfds[listenfdsIndex].events=POLLIN;
    listenfdsIndex++;
  }
  m_socket2newConnectionCallbackLock.unlock();

  int activeListenfdsNum=poll(listenfds,listenfdsNum,200);
  DSD_printf("activeListenfdsNum : %d , listenfdsNum : %d \n",activeListenfdsNum,listenfdsNum);
  if(activeListenfdsNum<=0)
    return -1;

  for(int n=0;n<listenfdsNum;n++)
  {
    DSD_printf("revent : %d\n",listenfds[n].revents);
    if(listenfds[n].revents & POLLERR)
    {
      m_socket2ErrCallbackLock.lock();
      socketErrCallback cb=m_socket2ErrCallback[listenfds[n].fd];
      m_socket2ErrCallbackLock.unlock();
      if(cb)
        cb(listenfds[n].fd);
    }
    else
    {
      int newConnectedSocketHandle=accept(listenfds[n].fd,nullptr,0);
      //setNonblock(newConnectedSocketHandle);
      if(newConnectedSocketHandle<0)
        continue;

      m_connectedSocketLock.lock();
      m_connectedSocket.push_back(newConnectedSocketHandle);
      m_connectedSocketLock.unlock();

      m_socket2newConnectionCallbackLock.lock();
      newConnectionCallback cb=m_socket2newConnectionCallback[listenfds[n].fd];
      m_socket2newConnectionCallbackLock.unlock();

      if(cb)
        cb(listenfds[n].fd,newConnectedSocketHandle);
    }
  }
  delete[] listenfds;
  return 0;
}

int SocketDelegate::doPollConnectedSocket()
{
  /*udpate connect socket status*/
  m_connectedSocketLock.lock();
  int connectedfdsNum=m_connectedSocket.size();
  int connectedfdsIndex=0;
  pollfd* connectedfds=new pollfd[connectedfdsNum];

  for(std::list<int>::iterator connectIter=m_connectedSocket.begin();connectIter!=m_connectedSocket.end();connectIter++)
  {
    connectedfds[connectedfdsIndex].fd=*connectIter;
    connectedfds[connectedfdsIndex].events=POLLIN|POLLOUT;
    connectedfdsIndex++;
  }
  m_connectedSocketLock.unlock();


  int activeConnectedfdsNum=poll(connectedfds,connectedfdsNum,200);
  DSD_printf("activeConnectfdsNum : %d , connectedfdsNum : %d \n",activeConnectedfdsNum,connectedfdsNum);
  if(activeConnectedfdsNum<=0)
    return -1;

  for(int n=0;n<connectedfdsNum;n++)
  {
    DSD_printf("fd :%d ,  revents :%d\n",connectedfds[n].fd,connectedfds[n].revents);
    if(connectedfds[n].revents & POLLERR)
    {
      m_socket2ErrCallbackLock.lock();
      socketErrCallback cb=m_socket2ErrCallback[connectedfds[n].fd];
      m_socket2ErrCallbackLock.unlock();
      if(cb)
        cb(connectedfds[n].fd);
      continue;
    }

    m_socket2ConnectCallbackLock.lock();
    connectCallback cb=m_socket2ConnectCallback[connectedfds[n].fd];
    m_socket2ConnectCallbackLock.unlock();
    if(cb)
    {
      cb(connectedfds[n].fd);
      m_socket2ConnectCallbackLock.lock();
      m_socket2ConnectCallback.erase(connectedfds[n].fd);
      m_socket2ConnectCallbackLock.unlock();
    }
    else
    {
      if(connectedfds[n].revents & POLLIN)
      {
        m_socket2RecvCallbackLock.lock(); 
        SocketDelegate::IOCallback cb=m_socket2RecvCallback[connectedfds[n].fd];
        m_socket2RecvCallbackLock.unlock();
        if(cb)
        {
          std::string recvStr;
          int length=0;
          while(1)
          {
            char buff[2048];
            memset(buff,0,2048);
            int recvLength=recv(connectedfds[n].fd,buff,2048,MSG_DONTWAIT);
            if(recvLength<=0)
              break;
            recvStr.append(buff,recvLength);
          }
          length=recvStr.size();
          if(length==0)
            close(connectedfds[n].fd);
          else
            cb(connectedfds[n].fd,recvStr,length);
        }
        else
          DSD_printf("no callback for recv\n");
      }

      if(connectedfds[n].revents & POLLOUT)
      {
        m_socket2SendCallbackLock.lock(); 

        SocketDelegate::IOCallback cb=m_socket2SendCallback[connectedfds[n].fd];
        if(cb)
        {
          std::string sendStr;
          int length=0;
          m_socket2SendCallback[connectedfds[n].fd](connectedfds[n].fd,sendStr,length);
          if(length>0)
            send(connectedfds[n].fd,sendStr.data(),length,MSG_DONTWAIT);
        }
        m_socket2SendCallbackLock.unlock(); 
      }
      if(connectedfds[n].revents & POLLHUP)
      {
        m_socket2CloseCallbackLock.lock();
        socketCloseCallback cb=m_socket2CloseCallback[connectedfds[n].fd];
        m_socket2CloseCallbackLock.unlock();
        if(cb)
          cb(connectedfds[n].fd);
        continue;
      }
    }
  }
  delete[] connectedfds;
  return 0;
}

int SocketDelegate::setNonblock(int socketHandle)
{ 
  int fcntlFlag;
  fcntlFlag = fcntl(socketHandle, F_GETFL, 0 );
  if(fcntlFlag<0)
  {
    close(socketHandle);
    return -1;

  }
  else
  {
    fcntlFlag|= O_NONBLOCK;
    if(fcntl(socketHandle, F_SETFL, fcntlFlag) < 0 ) 
    {
      close(socketHandle);
      return -1;

    }
  }
  return 0;
}
