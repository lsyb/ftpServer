#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
/*
struct FTPCommand
{
  int m_nTokenID;
  char *m_pszName;
  BOOL m_bHasArguments;
  char *m_pszDescription;

};

static FTPCommand commandList[] = 
{
  {TOK_ABOR,  "ABOR", FALSE,  "Abort transfer: ABOR"}, 
  {TOK_BYE, "BYE",  FALSE,  "Logout or break the connection: BYE"},
  {TOK_CDUP,  "CDUP", FALSE,  "Change to parent directory: CDUP"},
  {TOK_CWD, "CWD",  TRUE, "Change working directory: CWD [directory-name]"},
  {TOK_DELE,  "DELE", TRUE ,  "Delete file: DELE file-name"},
  {TOK_DIR, "DIR",  FALSE,  "Get directory listing: DIR [path-name]"},
  {TOK_HELP,  "HELP",  FALSE, "Show help: HELP [command]"},
  {TOK_LIST,  "LIST", FALSE,  "Get directory listing: LIST [path-name]"}, 
  {TOK_MKD, "MKD",  TRUE, "Make directory: MKD path-name"},
  {TOK_NOOP,  "NOOP", FALSE,  "Do nothing: NOOP"},
  {TOK_PASS,  "PASS", TRUE, "Supply a user password: PASS password"},
  {TOK_PASV,  "PASV", FALSE,  "Set server in passive mode: PASV"},
  {TOK_PORT,  "PORT", TRUE, "Specify the client port number: PORT a0,a1,a2,a3,a4,a5"},
  {TOK_PWD, "PWD",  FALSE,  "Get current directory: PWD"},
  {TOK_QUIT,  "QUIT",  FALSE, "Logout or break the connection: QUIT"},
  {TOK_REST,  "REST", TRUE, "Set restart transfer marker: REST marker"},
  {TOK_RETR,  "RETR", TRUE, "Get file: RETR file-name"},
  {TOK_RMD, "RMD",  TRUE, "Remove directory: RMD path-name"},
  {TOK_RNFR,  "RNFR", TRUE, "Specify old path name of file to be renamed: RNFR file-name"},
  {TOK_RNTO,  "RNTO", TRUE, "Specify new path name of file to be renamed: RNTO file-name"},
  {TOK_SIZE,  "SIZE", TRUE, "Get filesize: SIZE file-name"},
  {TOK_STOR,  "STOR", TRUE, "Store file: STOR file-name"},
  {TOK_SYST,  "SYST", FALSE,  "Get operating system type: SYST"},
  {TOK_TYPE,  "TYPE", TRUE, "Set filetype: TYPE [A | I]"},
  {TOK_USER,  "USER", TRUE, "Supply a username: USER username"},
  {TOK_ERROR, "",   FALSE,  ""},

};
*/

int main(int argc,char** argv)
{
  int ret=0;
  int socketHandle=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
  if(socketHandle<0)
  {
    std::cout<<"err : "<<strerror(errno)<<std::endl;
    return -1;
  }

  struct sockaddr_in addr;
  memset(&addr,0,sizeof(addr));
  //addr.sin_len=sizeof(struct sockaddr_in);
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=inet_addr(argv[1]);
  addr.sin_port=htons(atoi(argv[2]));
  ret=bind(socketHandle,(struct sockaddr*)&addr,sizeof(addr));
  if(ret<0)
  {
    std::cout<<"err : "<<strerror(errno)<<std::endl;
    return -1;
  }

  ret=listen(socketHandle,5);
  if(ret<0)
  {
    std::cout<<"err : "<<strerror(errno)<<std::endl;
    return -1;
  }
  struct sockaddr_in newAddr;
  socklen_t length=sizeof(newAddr);
  int newSocketHandle=accept(socketHandle,(struct sockaddr*)&newAddr,&length);
  std::cout<<"newSocketHandle : "<<newSocketHandle<<std::endl;
  std::cout<<"address : "<<inet_ntoa(newAddr.sin_addr);
  std::cout<<"port : "<<ntohs(newAddr.sin_port);

  std::cout<<"new connection "<<std::endl;
  char readyMsg[]="220 welcome test ftp\r\n";
  ret=send(newSocketHandle,readyMsg,sizeof(readyMsg)-1,0);
  std::cout<<"send  for init ret : "<<ret<<std::endl;

  while(1)
  {
    char buff[1024];
    memset(buff,0,1024);
    ret=recv(newSocketHandle,buff,1024,0);
    if(ret>0)
    {
      std::cout<<"ret : "<<ret<<std::endl;
      std::cout<<buff<<std::endl;
      std::string req(buff,ret);
      if(req.substr(0,4)=="USER")
      {
        char passwdStr[]="331 specify password  \r\n";
        send(newSocketHandle,passwdStr,sizeof(passwdStr)-1,0);
      }
      if(req.substr(0,4)=="PASS")
      {
        send(newSocketHandle,"230 \r\n",6,0);
      }
      if(req.substr(0,4)=="SYST")
      {
        char systStr[]="215 UNIX\r\n";
        send(newSocketHandle,systStr,sizeof(systStr)-1,0);
      }
      if(req.substr(0,4)=="PWD ")
      {
        char resp[]="257 / what the fuck\r\n";
        send(newSocketHandle,resp,sizeof(resp)-1,0);
      }
    }
  }
  close(socketHandle);
  return 0;
}
