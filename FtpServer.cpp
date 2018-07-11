#include "FtpServer.h"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <iostream>
#include <string.h>


struct FTPCommand
{
    FtpServer::FTPCommandID m_commandID;
    const char *m_commandName;
    bool m_bCommandHasParam;
    const char *m_commandDescription;
    
};

static FTPCommand commandList[] =
{
    {FtpServer::FTPCommandID::FTPCommand_ABOR,  "ABOR", false,  "Abort transfer: ABOR"},
    {FtpServer::FTPCommandID::FTPCommand_BYE, "BYE",  false,  "Logout or break the connection: BYE"},
    {FtpServer::FTPCommandID::FTPCommand_CDUP,  "CDUP", false,  "Change to parent directory: CDUP"},
    {FtpServer::FTPCommandID::FTPCommand_CWD, "CWD",  true, "Change working directory: CWD [directory-name]"},
    {FtpServer::FTPCommandID::FTPCommand_DELE,  "DELE", true ,  "Delete file: DELE file-name"},
    {FtpServer::FTPCommandID::FTPCommand_DIR, "DIR",  false,  "Get directory listing: DIR [path-name]"},
    {FtpServer::FTPCommandID::FTPCommand_HELP,  "HELP",  false, "Show help: HELP [command]"},
    {FtpServer::FTPCommandID::FTPCommand_LIST,  "LIST", false,  "Get directory listing: LIST [path-name]"},
    {FtpServer::FTPCommandID::FTPCommand_MKD, "MKD",  true, "Make directory: MKD path-name"},
    {FtpServer::FTPCommandID::FTPCommand_NOOP,  "NOOP", false,  "Do nothing: NOOP"},
    {FtpServer::FTPCommandID::FTPCommand_PASS,  "PASS", true, "Supply a user password: PASS password"},
    {FtpServer::FTPCommandID::FTPCommand_PASV,  "PASV", false,  "Set server in passive mode: PASV"},
    {FtpServer::FTPCommandID::FTPCommand_PORT,  "PORT", true, "Specify the client port number: PORT a0,a1,a2,a3,a4,a5"},
    {FtpServer::FTPCommandID::FTPCommand_PWD, "PWD",  false,  "Get current directory: PWD"},
    {FtpServer::FTPCommandID::FTPCommand_QUIT,  "QUIT",  false, "Logout or break the connection: QUIT"},
    {FtpServer::FTPCommandID::FTPCommand_REST,  "REST", true, "Set restart transfer marker: REST marker"},
    {FtpServer::FTPCommandID::FTPCommand_RETR,  "RETR", true, "Get file: RETR file-name"},
    {FtpServer::FTPCommandID::FTPCommand_RMD, "RMD",  true, "Remove directory: RMD path-name"},
    {FtpServer::FTPCommandID::FTPCommand_RNFR,  "RNFR", true, "Specify old path name of file to be renamed: RNFR file-name"},
    {FtpServer::FTPCommandID::FTPCommand_RNTO,  "RNTO", true, "Specify new path name of file to be renamed: RNTO file-name"},
    {FtpServer::FTPCommandID::FTPCommand_SIZE,  "SIZE", true, "Get filesize: SIZE file-name"},
    {FtpServer::FTPCommandID::FTPCommand_STOR,  "STOR", true, "Store file: STOR file-name"},
    {FtpServer::FTPCommandID::FTPCommand_SYST,  "SYST", false,  "Get operating system type: SYST"},
    {FtpServer::FTPCommandID::FTPCommand_TYPE,  "TYPE", true, "Set filetype: TYPE [A | I]"},
    {FtpServer::FTPCommandID::FTPCommand_USER,  "USER", true, "Supply a username: USER username"},
};

FtpServer::FtpServer()
{
    
}

FtpServer::~FtpServer()
{
}

int FtpServer::startServer(std::string ipAddress,int port,SocketDelegate::SocketVersion ipVersion)
{
    m_bIsRunning=true;
    m_ipAddress=ipAddress;
    m_ipVersion=ipVersion;
    m_controlListenSocket=m_socketDelegate.createNewUnicastStreamSocket(ipAddress,port,m_ipVersion,true);
    m_controllPort=port;
    if(m_controlListenSocket<0)
        return -1;
    
    m_socketDelegate.setnewConnectionCallback(m_controlListenSocket,std::bind(&FtpServer::onNewControlClient,this,std::placeholders::_1,std::placeholders::_2));
    m_socketDelegate.setErrCallback(m_controlListenSocket,std::bind(&FtpServer::onControlErr,this,std::placeholders::_1));
    m_socketDelegate.start();
    m_workThread=new std::thread(std::bind(&FtpServer::workLoop,this));
    return 0;
}

int FtpServer::shutdownServer()
{
    m_socketDelegate.shutdown();
    m_bIsRunning=false;
    m_workThread->join();
    delete m_workThread;
    return 0;
}



void FtpServer::setsessionOpenCallback(FtpServer::sessionOpenCallback f)
{
    m_sessionOpenCallback=f;
}

void FtpServer::setsessionCloseCallback(FtpServer::sessionCloseCallback f)
{
    m_sessionCloseCallback=f;
}

void FtpServer::setsessionErrCallback(FtpServer::sessionErrCallback f)
{
    m_sessionErrCallback=f;
}

void FtpServer::setStartPutCallback(FtpServer::beginIOCallback f)
{
    m_putBeginIOCallback=f;
}
void FtpServer::setEndPutCallback(FtpServer::endIOCallback f)
{
    m_putEndIOCallback=f;
}

void FtpServer::setDoingPutCallback(FtpServer::IOCallback f)
{
    m_putIOCallback=f;
}

void FtpServer::setErrPutCallback(FtpServer::errIOCallback f)
{
    m_putErrIOCallback=f;
}



void FtpServer::onNewControlClient(int listenSocket,int clientSocket)
{
    std::cout<<"new control client : "<<clientSocket<<std::endl;
    m_socket2RequestLock.lock();
    m_socket2ResponseLock.lock();
    m_socket2Request.insert(std::pair<int,std::list<FtpCommandRequest> >(clientSocket,std::list<FtpCommandRequest>()));
    m_socket2Response.insert(std::pair<int,std::list<std::string> >(clientSocket,std::list<std::string>()));
    m_socket2Response[clientSocket].push_back("220 CloudCamera Ftp Server\r\n");
    m_socket2ResponseLock.unlock();
    m_socket2RequestLock.unlock();
    
    m_socket2UserPasswdLock.lock();
    m_socket2UserandPasswd.insert(std::pair<int,UsernameAndPassword>(clientSocket,UsernameAndPassword()));
    m_socket2UserPasswdLock.unlock();
    
    m_socketDelegate.setSendCallback(clientSocket,std::bind(&FtpServer::onControlClientSend,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    m_socketDelegate.setRecvCallback(clientSocket,std::bind(&FtpServer::onControlClientRecv,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    m_socketDelegate.setCloseCallback(clientSocket,std::bind(&FtpServer::onControlClientClose,this,std::placeholders::_1));
    m_socketDelegate.setErrCallback(clientSocket,std::bind(&FtpServer::onControlClientErr,this,std::placeholders::_1));
}

void FtpServer::onNewDataClient(int listenSocket,int clientSocket)
{
    std::cout<<"new data client : "<<clientSocket<<std::endl;
    m_controldataSocketLock.lock();
    int controlSocket=m_data2controlSocket[listenSocket];
    m_data2controlSocket.erase(listenSocket);
    m_data2controlSocket.insert(std::pair<int,int>(clientSocket,controlSocket));
    m_control2dataSocket[controlSocket]=clientSocket;
    m_controldataSocketLock.unlock();
    
    std::cout<<"set send callback"<<std::endl;
    m_socketDelegate.setSendCallback(clientSocket,std::bind(&FtpServer::onDataClientSend,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    std::cout<<"set recv callback"<<std::endl;
    m_socketDelegate.setRecvCallback(clientSocket,std::bind(&FtpServer::onDataClientRecv,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    std::cout<<"set err callback"<<std::endl;
    m_socketDelegate.setCloseCallback(clientSocket,std::bind(&FtpServer::onDataClientClose,this,std::placeholders::_1));
    std::cout<<"set close callback"<<std::endl;
    m_socketDelegate.setErrCallback(clientSocket,std::bind(&FtpServer::onDataClientErr,this,std::placeholders::_1));
    std::cout<<"close data listen socket"<<std::endl;
    
    m_socketDelegate.clearListenSocket(listenSocket);
}

void FtpServer::onControlClientSend(int socketHandle,std::string& str,int& length)
{
    if(m_socket2Response[socketHandle].size()>0)
    {
        //std::cout<<"onSend "<<std::endl;
        str=m_socket2Response[socketHandle].front();
        length=str.size();
        m_socket2Response[socketHandle].pop_front();
    }
}

void FtpServer::onControlClientRecv(int socketHandle,std::string& cmdStr,int& length)
{
    //  std::cout<<"onRecv"<<std::endl;
    std::string::size_type cmdCodeStart=0;
    std::string::size_type cmdCodeEnd=0;
    cmdCodeEnd=cmdStr.find(' ',cmdCodeStart);
    if(cmdCodeEnd==std::string::npos)
        cmdCodeEnd=cmdStr.find("\r\n",cmdCodeStart);
    std::string cmdCodeStr=cmdStr.substr(cmdCodeStart,cmdCodeEnd-cmdCodeStart);
    for(int n=0;n<cmdCodeStr.size();n++)
    {
        if(cmdCodeStr[n]>='a' && cmdCodeStr[n]<='z')
            cmdCodeStr[n]-=32;
    }
    FTPCommandID cmdID=FTPCommandID::FTPCommand_NOTSUPPORT;
    std::string cmdParam;
    for(int n=0;n<sizeof(commandList)/sizeof(FTPCommand);n++)
    {
        if(cmdCodeStr!=commandList[n].m_commandName)
            continue;
        cmdID=commandList[n].m_commandID;
        if(commandList[n].m_bCommandHasParam)
        {
            int paramStart=cmdCodeEnd;
            while(paramStart<cmdStr.size())
            {
                if(cmdStr[paramStart]!=' ')
                    break;
                paramStart++;
            }
            int paramEnd=cmdStr.find("\r\n",paramStart);
            cmdParam=cmdStr.substr(paramStart,paramEnd-paramStart);
        }
    }
    FtpCommandRequest req;
    req.cmdName=cmdCodeStr;
    req.cmdID=cmdID;
    req.param=cmdParam;
    m_socket2RequestLock.lock();
    m_socket2Request[socketHandle].push_back(req);
    m_socket2RequestLock.unlock();
}

void FtpServer::onDataClientSend(int socketHandle,std::string& str,int& length)
{
  std::cout<<"ftpSend"<<std::endl;
  m_controldataSocketLock.lock();
  int controlSocket=m_data2controlSocket[socketHandle];
  m_controldataSocketLock.unlock();

  SocketOperationType opType=SocketOperationType::SOT_EMPTY;
  m_socket2OperationTypeLock.lock();
  if(m_socket2OperationType.find(controlSocket)!=m_socket2OperationType.end())
    opType=m_socket2OperationType[controlSocket];
  if(opType!=SocketOperationType::SOT_EMPTY)
  {
    switch(opType)
    {
      default:
        break;
    }
  }
  m_socket2OperationTypeLock.unlock();

}

void FtpServer::onDataClientRecv(int socketHandle,std::string& str,int& length)
{
  std::cout<<"ftpRecv ,size : "<<str.size()<<std::endl;
  m_controldataSocketLock.lock();
  int controlSocket=m_data2controlSocket[socketHandle];
  m_controldataSocketLock.unlock();

  SocketOperationType opType;
  m_socket2OperationTypeLock.lock();
  if(m_socket2OperationType.find(controlSocket)!=m_socket2OperationType.end())
    opType=m_socket2OperationType[controlSocket];
  if(opType!=SocketOperationType::SOT_EMPTY)
  {
    switch(opType)
    {
      case SocketOperationType::SOT_PUT:
        if(m_putIOCallback)
          m_putIOCallback(controlSocket,str,length);
        break;
      default:
        break;
    }
  }
  m_socket2OperationTypeLock.unlock();
}

//ftp server err
void FtpServer::onControlErr(int socketHandle)
{
  m_socketDelegate.clearListenSocket(socketHandle);
  std::cout<<"control server err : "<<socketHandle<<std::endl;
}

//ftp client err
void FtpServer::onControlClientErr(int socketHandle)
{
  m_socketDelegate.clearConnectedSocket(socketHandle);
  if(m_sessionErrCallback)
    m_sessionErrCallback(socketHandle);
  std::cout<<"control client err : "<<socketHandle<<std::endl;
}

//ftp data err
void FtpServer::onDataErr(int socketHandle)
{
  m_socketDelegate.clearListenSocket(socketHandle);
  clearDataSocket(socketHandle);
  std::cout<<"data sever err : "<<socketHandle<<std::endl;

}

//ftp data client err
void FtpServer::onDataClientErr(int socketHandle)
{
  std::cout<<"data client err"<<std::endl;
  m_socketDelegate.clearConnectedSocket(socketHandle);
  m_controldataSocketLock.lock();
  int controlSocket=m_data2controlSocket[socketHandle];
  m_control2dataSocket.erase(controlSocket);
  m_data2controlSocket.erase(socketHandle);
  m_controldataSocketLock.unlock();

  SocketOperationType opType=SocketOperationType::SOT_EMPTY;
  m_socket2OperationTypeLock.lock();
  if(m_socket2OperationType.find(controlSocket)!=m_socket2OperationType.end())
    opType=m_socket2OperationType[controlSocket];
  if(opType!=SocketOperationType::SOT_EMPTY)
  {
    m_socket2OperationType.erase(controlSocket);
    switch(opType)
    {
      case SocketOperationType::SOT_PUT:
        if(m_putErrIOCallback)
          m_putErrIOCallback(controlSocket);
        break;
      default:
        break;
    }
  }
  m_socket2OperationTypeLock.unlock();

  std::cout<<"data client err : "<<socketHandle<<std::endl;
}

void FtpServer::onControlClientClose(int socketHandle)
{
  m_socketDelegate.clearConnectedSocket(socketHandle);
  clearControlSocket(socketHandle);
  if(m_sessionCloseCallback)
    m_sessionCloseCallback(socketHandle);
  std::cout<<"control client close : "<<socketHandle<<std::endl;
}

void FtpServer::onDataClientClose(int socketHandle)
{
  m_socketDelegate.clearConnectedSocket(socketHandle);
  m_controldataSocketLock.lock();
  int controlSocket=m_data2controlSocket[socketHandle];
  m_control2dataSocket.erase(controlSocket);
  m_data2controlSocket.erase(socketHandle);
  m_controldataSocketLock.unlock();

  SocketOperationType opType=SocketOperationType::SOT_EMPTY;
  m_socket2OperationTypeLock.lock();
  if(m_socket2OperationType.find(controlSocket)!=m_socket2OperationType.end())
    opType=m_socket2OperationType[controlSocket];
  if(opType!=SocketOperationType::SOT_EMPTY)
  {
    m_socket2OperationType.erase(controlSocket);
    switch(opType)
    {
      case SocketOperationType::SOT_PUT:
        m_socket2ResponseLock.lock(); 
        m_socket2Response[controlSocket].push_back("226\r\n");
        m_socket2ResponseLock.unlock(); 
        if(m_putEndIOCallback)
          m_putEndIOCallback(controlSocket);
        break;
      case SocketOperationType::SOT_GET:
        break;
    }
  }
  m_socket2OperationTypeLock.unlock();
  std::cout<<"data client close : "<<socketHandle<<std::endl;
}

void FtpServer::clearControlSocket(int socketHandle)
{
  m_socket2UserPasswdLock.lock();
  m_socket2UserandPasswd.erase(socketHandle);
  m_socket2UserPasswdLock.unlock();

  m_socket2RequestLock.lock();
  m_socket2Request.erase(socketHandle);
  m_socket2RequestLock.unlock();

  m_socket2ResponseLock.lock();
  m_socket2Response.erase(socketHandle);
  m_socket2ResponseLock.unlock();

}

void FtpServer::clearDataSocket(int socketHandle)
{
  m_controldataSocketLock.lock();
  int controlSocket=m_data2controlSocket[socketHandle];
  m_control2dataSocket.erase(controlSocket);
  m_data2controlSocket.erase(socketHandle);
  m_controldataSocketLock.unlock();
}

void FtpServer::workLoop()
{
  while(m_bIsRunning)
  {
    m_socket2RequestLock.lock();
    m_socket2ResponseLock.lock();
    for(std::map<int,std::list<FtpCommandRequest> >::iterator reqIter=m_socket2Request.begin();reqIter!=m_socket2Request.end();reqIter++)
    {
      if(reqIter->second.size()==0)
        continue;
      FtpCommandRequest req=reqIter->second.front();
      reqIter->second.pop_front();
      FTPCommandID cmdID=req.cmdID;
      switch(cmdID)
      {
        case FTPCommandID::FTPCommand_USER:
          std::cout<<"user : "<<req.param<<std::endl;
          m_socket2UserPasswdLock.lock();
          m_socket2UserandPasswd[reqIter->first].username=req.param;
          m_socket2UserPasswdLock.unlock();
          m_socket2Response[reqIter->first].push_back("331 specify password \r\n");
          break;
        case FTPCommandID::FTPCommand_PASS:
          std::cout<<"password : "<<req.param<<std::endl;
          m_socket2UserPasswdLock.lock();
          m_socket2UserandPasswd[reqIter->first].password=req.param;
          if(m_sessionOpenCallback)
            m_sessionOpenCallback(reqIter->first,m_socket2UserandPasswd[reqIter->first].username,m_socket2UserandPasswd[reqIter->first].password);
          m_socket2UserPasswdLock.unlock();
          m_socket2Response[reqIter->first].push_back("230 \r\n");
          break;
        case FTPCommandID::FTPCommand_PWD:
          std::cout<<"pwd : "<<req.param<<std::endl;
          m_socket2Response[reqIter->first].push_back("257 / current dir \r\n");
          break;
        case FTPCommandID::FTPCommand_SYST:
          std::cout<<"syst : "<<req.param<<std::endl;
          m_socket2Response[reqIter->first].push_back("215 UNIX \r\n");
          break;
        case FTPCommandID::FTPCommand_MKD:
          std::cout<<"mkd : "<<req.param<<std::endl;
          m_socket2Response[reqIter->first].push_back("257 / mkd dir\r\n");
          break;
        case FTPCommandID::FTPCommand_CWD:
          std::cout<<"cwd : "<<req.param<<std::endl;
          m_socket2Response[reqIter->first].push_back("200 / cwd dir\r\n");
          break;
        case FTPCommandID::FTPCommand_TYPE:
          std::cout<<"type : "<<req.param<<" , size : "<<req.param.size()<<std::endl;
          m_socket2Response[reqIter->first].push_back("200\r\n");
          break;
        case FTPCommandID::FTPCommand_STOR:
          {
            std::cout<<"stor : "<<req.param<<std::endl;
            m_socket2OperationTypeLock.lock();
            m_socket2OperationType.insert(std::pair<int,SocketOperationType>(reqIter->first,SocketOperationType::SOT_PUT));
            m_socket2OperationType.insert(std::pair<int,SocketOperationType>(34,SocketOperationType::SOT_GET));
            m_socket2OperationTypeLock.unlock();

            if (m_putBeginIOCallback)
              m_putBeginIOCallback(reqIter->first,req.param);
            m_socket2Response[reqIter->first].push_back("125\r\n");

          }
          break;
        case FTPCommandID::FTPCommand_QUIT:
          std::cout<<"quit : "<<req.param<<std::endl;
          m_socket2Response[reqIter->first].push_back("221\r\n");
          break;
        case FTPCommandID::FTPCommand_PASV:
          {
            std::cout<<"pasv : "<<req.param<<" ,id : "<<reqIter->first<<std::endl;
            std::string pasvResponse="227 Entering Passive Mode(";
            std::string tmpIpaddress=m_ipAddress;
            int dotPos=0;
            while(1)
            {
              dotPos=tmpIpaddress.find('.',dotPos);
              if(dotPos==std::string::npos)
                break;
              tmpIpaddress.replace(dotPos,1,",");
            }
            int port=0;
            while(1)
            {
              port=random()%65535;
              std::cout<<"search port ,current : "<<port<<std::endl;
              int newDataSocket=m_socketDelegate.createNewUnicastStreamSocket(m_ipAddress,port,m_ipVersion,true);
              std::cout<<"newDataSocket : "<<newDataSocket<<std::endl;
              if(newDataSocket<0)
                continue;
              m_socketDelegate.setnewConnectionCallback(newDataSocket,std::bind(&FtpServer::onNewDataClient,this,std::placeholders::_1,std::placeholders::_2));
              m_controldataSocketLock.lock();
              m_control2dataSocket.insert(std::pair<int,int>(reqIter->first,newDataSocket));
              m_data2controlSocket.insert(std::pair<int,int>(newDataSocket,reqIter->first));
              m_controldataSocketLock.unlock();
              std::cout<<"success to create data socket"<<std::endl;
              break;
            }
            char portStr[100];
            memset(portStr,0,100);
            sprintf(portStr,",%d,%d).\r\n",port/256,port%256);
            pasvResponse.append(tmpIpaddress);
            pasvResponse.append(portStr);
            std::cout<<"pasvResponse : "<<pasvResponse<<std::endl;
            m_socket2Response[reqIter->first].push_back(pasvResponse);
          }
          break;
        default:
          std::cout<<"not support command : "<<req.cmdName<<std::endl;
          m_socket2Response[reqIter->first].push_back("502\r\n");
          break;
      }
    }
    m_socket2RequestLock.unlock();
    m_socket2ResponseLock.unlock();
  }

}
