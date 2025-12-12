#include <iostream>
#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#pragma comment(lib, "ws2_32.lib")
class ReactorEventLoop
{
public:
   using EventCallback = std::function<void(SOCKET, int)>;
   SocketInfo info;
   info.socket = sock;
   ReactorEventLoop() : running_(false)
   {
      // 初始化 Winsock
      WSADATA wsaData;
      if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
      {
         throw std::runtime_error("WSAStartup failed");
      }
   }
   ~ReactorEventLoop()
   {
      stop();
      // 清理所有事件和socket
      for (auto &event : events_)
      {
         WSACloseEvent(event);
      }
      for (auto sock : sockets_)
      {
         closesocket(sock);
      }
      WSACleanup();
   }
   // 添加 socket 到事件循环
   void addSocket(SOCKET sock, EventCallback readCallback = nullptr, )
   {
      std::lock_guard
   }

private:
   struct SocketInfo
   {
      SOCKET socket;
      WSAEVENT event;
      EventCallback readCallback;
      EventCallback writeCallback;
      EventCallback closeCallback;
   };
   std::vector<SocketInfo> socketInfos_;
   std::vector<WSAEVENT> events_;
   std::vector<SOCKET> sockets_;
   std::atomic<bool> running_;
   std::mutex mutex_;

   EventCallback defaultReadCallback_;
   EventCallback defaultWriteCallback_;
   EventCallback defaultCloseCallback_;

   void handleEvent(SocketInfo *info, int eventType, int errorCode)
   {
      if (errorCode != 0)
      {
         std::cerr << "Network event error: " << errorCode << std::endl;
         if(eventType==FD_CLOSE||errorCode!=0)
         {
            
         }
      }
   }
};
int main(int, char **)
{
   std::cout << "Hello, from CReactorEventLoop!\n";
}
