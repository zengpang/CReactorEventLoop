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
   void addSocket(SOCKET sock, 
      EventCallback readCallback = nullptr,
      EventCallback writeCallback=nullptr,
      EventCallback closeCallback=nullptr
   )
   {
      std::lock_guard<std::mutex> lock(mutex_);
      WSAEVENT event = WSACreateEvent();
      if (event == WSA_INVALID_EVENT)
      {
         throw std::runtime_error("WSACreateEvent failed");
      }

      // 注册事件
      if (WSAEventSelect(sock, event, FD_READ | FD_WRITE | FD_ACCEPT) == SOCKET_ERROR)
      {
         WSACloseEvent(event);
         throw std::runtime_error("WSACreateEvent failed");
      }
      SocketInfo info;
      info.socket = sock;
      info.event=event;
      info.readCallback=readCallback;
      info.writeCallback=writeCallback;
      info.closeCallback=closeCallback;

      socketInfos_.push_back(info);
      events_.push_back(event);
      sockets_.push_back(sock);
   }

   //移除 socket
   void removeSocket(SOCKET sock)
   {
      std::lock_guard<std::mutex> lock(mutex_);
      for(auto it=socketInfos_.begin();it!=socketInfos_.end();++it)
      {
         if(it->socket==sock)
         {
            WSACloseEvent(it->event);
            closesocket(sock);
            socketInfos_.erase(it);
            break;
         }
      }

      //同时从events_和sockets_中移除
      events_.erase(std::remove(events_.begin(),events_.end(),
       [&](WSAEVENT event){
         for(const auto& info:socketInfos_)
         {
            if(info.event==event)
            {
               return false;
            }
            return true;
         }
       }
     ))
   }
   void stop()
   {
      running_=false;
   }
   bool isRunning() const{
      return running_;
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
         if (eventType == FD_CLOSE || errorCode != 0)
         {
            throw std::runtime_error("WSACreateEvent failed");
         }

         // 注册事件
         if (WSAEventSelect(sock))
         {
         }
      }
   }
};
//示例试用:简单的Echo 服务器
class EchoServer{
   public:
     EchoServer():serverSocket_(INVALID_ATOM){};
     ~EchoServer()
     {

     }
     void start(int port=8080)
     {
       //创建服务器socket
       serverS
     }
}
int main(int, char **)
{
   std::cout << "Hello, from CReactorEventLoop!\n";
}
