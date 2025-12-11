#include <iostream>
#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#pragma comment(lib,"ws2_32.lib")
class ReactorEventLoop{
    public:
       using EventCallback=std::function<void(SOCKET,int)>;
       ReactorEventLoop():running_(false){
          // 初始化 Winsock
          WSADATA wsaData;
          if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0)
          {
            throw std::runtime_error("WSAStartup failed");
          }
       }
       ~ReactorEventLoop()
       {
          stop();
          // 清理所有事件和socket
          for(auto& event;events_)
          {
            WSA
          }
       }
       //添加 socket 到事件循环
       void addSocket(SOCKET sock,EventCallback readCallback=nullptr,
      ){
         std::lock_guard
       }
};
int main(int, char**){
    std::cout << "Hello, from CReactorEventLoop!\n";
}
