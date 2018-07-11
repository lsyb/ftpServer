#include "SocketDelegate.h"
#include <iostream>
#include <stdlib.h>
#include <unistd.h>

SocketDelegate del;

void onConnect(int& s)
{
  std::cout<<"new socket : "<<s<<std::endl;
}
void onSend(std::string& str,int& length)
{
  str.append("hello");
  length=5;
}
void onRecv(std::string& str,int& length)
{
  std::cout<<"recv : "<<str<<std::endl;
}

void onListen(int& s)
{
  std::cout<<"new socket accpet : "<<s<<std::endl;
  del.setSendCallback(s,onSend);
  del.setRecvCallback(s,onRecv);

}
int main()
{
  int s=del.createNewUnicastStreamSocket("0.0.0.0",12345,SocketDelegate::SocketVersion::SocketVersion_IPV4,true);
  if(s>0) 
  {
    del.setListenCallback(s,onListen);
    del.start();
  }
  else
    std::cout<<"failed to create socket"<<std::endl;
  while(1)
  {
    sleep(2);
  }
  return 0;
}
