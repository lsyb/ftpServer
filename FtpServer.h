#ifndef FTPSERVER_HEADER_21F012A0_FB69_41D4_AEDE_1EE203A82FD0
#define FTPSERVER_HEADER_21F012A0_FB69_41D4_AEDE_1EE203A82FD0


#include <string>
#include <functional>
#include <map>
#include "SocketDelegate.h"
#include <mutex>
#include <list>
#include <thread>

class FtpServer
{
  public:
    enum class FTPCommandID
    {
      FTPCommand_NOTSUPPORT=-1,FTPCommand_ABOR, FTPCommand_BYE, FTPCommand_CDUP,
      FTPCommand_CWD, FTPCommand_DELE, FTPCommand_DIR, FTPCommand_HELP,
      FTPCommand_LIST, FTPCommand_MKD, FTPCommand_NOOP, FTPCommand_PASS,
      FTPCommand_PASV, FTPCommand_PORT, FTPCommand_PWD, FTPCommand_QUIT,
      FTPCommand_REST, FTPCommand_RETR, FTPCommand_RMD, FTPCommand_RNFR,
      FTPCommand_RNTO, FTPCommand_SIZE, FTPCommand_STOR, FTPCommand_SYST,
      FTPCommand_TYPE, FTPCommand_USER
    };

    enum class SocketOperationType
    {
      SOT_EMPTY=0,
      SOT_PUT,
      SOT_GET
    };  

    struct FtpCommandRequest
    {
      FTPCommandID cmdID;
      std::string cmdName;
      std::string param;
    };
    struct UsernameAndPassword
    {
      std::string username;
      std::string password;
    };
  public:
    typedef std::function<void(int id,std::string& fileName)> beginIOCallback;
    typedef std::function<void(int id)> endIOCallback;
    typedef std::function<void(int id,std::string& fileContent,int length)> IOCallback;
    typedef std::function<void(int id)> errIOCallback;
    typedef std::function<void(int id,std::string& userName,std::string& passWord)> sessionOpenCallback;
    typedef std::function<void(int id)> sessionCloseCallback;
    typedef std::function<void(int id)> sessionErrCallback;
  public:
    FtpServer();
    ~FtpServer();
    /*建立一个ftp服务端
     *@param (std::string)ipAddress
     *服务端所在ip
     *@param (int)port
     *ftp服务要监听的端口
     *@return (int)
     *成功建立ftp返回0，否则返回-1
     */
    int startServer(std::string ipAddress,int port,SocketDelegate::SocketVersion ipVersion=SocketDelegate::SocketVersion::SocketVersion_IPV4);

    /*关闭ftp服务端
    */
    int shutdownServer();

    /*
     *设置有用户连接到ftp时的回调
     */
    void setsessionOpenCallback(sessionOpenCallback);

    /*
     *设置用户断开ftp时的回调
     */
    void setsessionCloseCallback(sessionCloseCallback);

    /*
     *设置用户ftp连接出错时的回调
     */
    void setsessionErrCallback(sessionErrCallback);

    /*
     *设置开始上传文件时的回调，用于通知ftp建立文件
     */
    void setStartPutCallback(beginIOCallback);

    /*
     *设置上传文件完成时的回调，用于通知ftp关闭保存文件
     */
    void setEndPutCallback(endIOCallback);

    /*
     *设置上传文件有数据时的回调，用于通知ftp有数据，以便写入文件
     */
    void setDoingPutCallback(IOCallback);

    /*
     *设置上传文件出错时的回调
     */
    void setErrPutCallback(errIOCallback);

    /*
     *和上传类似
     *暂时没用到get方法，未实现

     void setStartGetCallback(int,beginIOCallback);
     void setEndGetCallback(int,beginIOCallback);
     void setDoingGetCallback(int,IOCallback);
     void setErrGetCallback(int,errIOCallback);
     */



  private:
    void onNewControlClient(int listenSocket,int clientSocket);
    void onControlClientSend(int socketHandle,std::string& str,int& length);
    void onControlClientRecv(int socketHandle,std::string& str,int& length);
    void onNewDataClient(int listenSocket,int clientSocket);
    void onDataClientSend(int socketHandle,std::string& str,int& length);
    void onDataClientRecv(int socketHandle,std::string& str,int & length);
    void onControlErr(int socketHandle);
    void onControlClientErr(int socketHandle);
    void onDataErr(int socketHandle);
    void onDataClientErr(int socketHandle);
    void onControlClientClose(int socketHandle);
    void onDataClientClose(int socketHandle);
    void clearControlSocket(int socketHandle);
    void clearDataSocket(int socketHandle);
    void workLoop();

  private:
    sessionOpenCallback m_sessionOpenCallback;
    sessionCloseCallback m_sessionCloseCallback;
    sessionErrCallback m_sessionErrCallback;
    IOCallback m_putIOCallback;
    beginIOCallback m_putBeginIOCallback;
    endIOCallback m_putEndIOCallback;
    errIOCallback m_putErrIOCallback;

    std::string m_ipAddress;
    SocketDelegate::SocketVersion m_ipVersion;
    int m_controlListenSocket;
    int m_controllPort;
    SocketDelegate m_socketDelegate;
    std::map<int,UsernameAndPassword> m_socket2UserandPasswd;
    std::map<int,std::list<FtpCommandRequest> > m_socket2Request;
    std::map<int,std::list<std::string> > m_socket2Response;
    std::map<int,int> m_control2dataSocket;
    std::map<int,int> m_data2controlSocket;

    /*
       std::map<int,IOCallback> m_socket2DoingIOCallback;
       std::map<int,beginIOCallback> m_socket2BeginIOCallback;
       std::map<int,endIOCallback> m_socket2EndIOCallback;
       std::map<int,errIOCallback> m_socket2ErrIOCallback;
       */
    std::map<int,SocketOperationType> m_socket2OperationType;

    std::mutex m_socket2UserPasswdLock;
    std::mutex m_socket2RequestLock;
    std::mutex m_socket2ResponseLock;
    std::mutex m_controldataSocketLock;

    /*
       std::mutex m_socket2DoingIOCallbackLock;
       std::mutex m_socket2BeginIOCallbackLock;
       std::mutex m_socket2EndIOCallbackLock;
       std::mutex m_socket2ErrIOCallbackLock;
       */

    std::mutex m_socket2OperationTypeLock;
    std::mutex m_socket2filenameLock;
    std::thread* m_workThread;
    bool m_bIsRunning;
};

#endif
