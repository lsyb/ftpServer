#include "FtpServer.h"
#include <iostream>
#include <map>
#include <fstream>
#include <mutex>
#include <unistd.h>


FtpServer s;
std::map<int,std::fstream*> id2fs;
std::map<int,std::string> id2filename;
std::map<int,int> filesize;
std::map<int,std::string> id2username;
std::mutex id2fsLock;
std::mutex id2filenameLock;
std::mutex id2usernameLock;


void onBeginIO(int id,std::string name);
void onEndIO(int id);
void onIO(int id,std::string& str,int length);
void onLogin(int id,std::string username,std::string password)
{
  std::cout<<"user : "<<username<<" password : "<<password<<" login"<<std::endl;
  id2usernameLock.lock();
  id2username.insert(std::pair<int,std::string>(id,username));
  id2usernameLock.unlock();
  s.setStartPutCallback(onBeginIO);
  s.setEndPutCallback(onEndIO);
  s.setDoingPutCallback(onIO);
}

void onLogout(int id)
{
  id2usernameLock.lock();
  std::string username=id2username[id];
  id2usernameLock.unlock();

  std::cout<<"user : "<<username<<" log out"<<std::endl;
}

void onBeginIO(int id,std::string name)
{
  std::fstream* fs=new std::fstream(name,std::fstream::out|std::fstream::binary);
  std::cout<<"fs : " <<(int64_t)fs<<std::endl;
  std::cout<<" id : "<<id<<" ,create file : "<<name<<std::endl;
  id2fsLock.lock();
  id2fs.insert(std::pair<int,std::fstream*>(id,fs));
  filesize.insert(std::pair<int,int>(id,0));
  id2fsLock.unlock();
  id2filename.insert(std::pair<int,std::string>(id,name));
  id2filenameLock.unlock();
}

void onEndIO(int id)
{
  std::cout<<"onEndIO"<<std::endl;
  id2filenameLock.lock();
  std::cout<<"id : "<<id<<" ,close file : "<<id2filename[id]<<std::endl;
  id2filename.erase(id);
  id2filenameLock.unlock();

  id2fsLock.lock();
  std::cout<<"fs : "<<(uint64_t)id2fs[id]<<std::endl;
  id2fs[id]->close();
  delete id2fs[id];
  id2fs.erase(id);
  filesize.erase(id);
  id2fsLock.unlock();
  std::cout<<"fstream closed"<<std::endl;
}

void onIO(int id,std::string& str,int length)
{
  std::cout<<"onIO"<<std::endl;
  std::cout<<"write size : "<<str.size()<<std::endl;
  id2fsLock.lock();
  filesize[id]+=str.size();
  std::cout<<"total write size : "<<filesize[id]<<std::endl;
  id2fs[id]->write(str.data(),str.size());
  id2fsLock.unlock();
}


int main(int argc,char** argv)
{
  s.setsessionOpenCallback(onLogin);
  s.setsessionCloseCallback(onLogout);
  s.startServer(argv[1],atoi(argv[2]),SocketDelegate::SocketVersion::SocketVersion_IPV4);
  int count=0;
  while(1)
  {
    //count++;
    //std::cout<<"count : "<<count<<std::endl;
    sleep(1);
    /*
    {
      if(count>30)
      {
        s.shutdownServer();
        break;
      }
    }
    */
  }
  return 0;
}
