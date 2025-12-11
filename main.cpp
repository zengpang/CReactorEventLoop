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
          // ≥ı ºªØ Winsock
          WSADATA wsaData;
          if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0)
          {
            throw std::runtime_error("WSAStartup failed");
          }
       }
};
int main(int, char**){
    std::cout << "Hello, from CReactorEventLoop!\n";
}
