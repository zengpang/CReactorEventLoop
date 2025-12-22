#include <iostream>
#include <WinSock2.h> // Windows Socket API 2.0 头文件，提供网络变成接口
#include <windows.h>  // Windows API 主头文件，提供系统功能
#include <iostream>
#include <vector>     // C++ 动态数组容器
#include <thread>     // C++ 线程支持
#include <atomic>     // C++ 原子操作支持
#include <functional> // C++ 函数对象支持
#include <memory>     // C++ 智能指针支持
#include <mutex>
#pragma comment(lib, "ws2_32.lib") // 编译器指令，自动链接 ws2_32.lib 库
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
                  EventCallback writeCallback = nullptr,
                  EventCallback closeCallback = nullptr)
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
      info.event = event;
      info.readCallback = readCallback;
      info.writeCallback = writeCallback;
      info.closeCallback = closeCallback;

      socketInfos_.push_back(info);
      events_.push_back(event);
      sockets_.push_back(sock);
   }

   // 移除 socket
   void removeSocket(SOCKET sock)
   {
      std::lock_guard<std::mutex> lock(mutex_);
      for (auto it = socketInfos_.begin(); it != socketInfos_.end(); ++it)
      {
         if (it->socket == sock)
         {
            WSACloseEvent(it->event);
            closesocket(sock);
            socketInfos_.erase(it);
            break;
         }
      }

      // 同时从events_和sockets_中移除
      events_.erase(std::remove(events_.begin(), events_.end(),
                                [&](WSAEVENT event)
                                {
                                   for (const auto &info : socketInfos_)
                                   {
                                      if (info.event == event)
                                      {
                                         return false;
                                      }
                                      return true;
                                   }
                                }));
   };

   // 设置默认回调函数
   void setDefaultReadCallback(EventCallback callback) { defaultReadCallback_ = callback; }
   void setDefaultWriteCallback(EventCallback callback) { defaultWriteCallback_ = callback; }
   void setDefaultCloseCallback(EventCallback callback) { defaultCloseCallback_ = callback; }

   // 启动事件循环
   void run()
   {
      if (running_)
         return;
      running_ = true;
      std::cout << "Reactor event loop started" << std::endl;
      while (running_)
      {
         if (events_.empty())
         {
            Sleep(100); // 没有事件时短暂休眠
            continue;
         }
         DWORD result = WSAWaitForMultipleEvents(
             static_cast<DWORD>(events_.size()),
             events_.data(),
             FALSE,
             1000, // 1秒超时
             FALSE);
         if (result == WSA_WAIT_FAILED)
         {
            std::cerr << "WSAWaitForMultipleEvents failed:" << WSAGetLastError() << std::endl;
            continue;
         }
         if (result == WSA_WAIT_TIMEOUT)
         {
            continue;
         }

         DWORD eventIndex = result * WSA_WAIT_EVENT_0;
         if (eventIndex == events_.size())
         {
            continue;
         }

         WSAEVENT triggeredEvent = events_[eventIndex];
         SocketInfo *info = nullptr;

         // 找到对应的事件信息
         {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto &socketInfo : socketInfos_)
            {
               if (socketInfo.event == triggeredEvent)
               {
                  info = &socketInfo;
                  break;
               }
            }
         }

         if (!info)
            continue;

         WSANETWORKEVENTS netEvents;
         if (WSAEnumNetworkEvents(info->socket, triggeredEvent, &netEvents) == SOCKET_ERROR)
         {
            std::cerr << "WSAEnumNetWorkEvents failed:" << WSAGetLastError() << std::endl;
            continue;
         }

         // 处理各种网络事件
         if (netEvents.lNetworkEvents & FD_READ)
         {
            handleEvent(info, FD_READ, netEvents.iErrorCode[FD_READ_BIT]);
         }
         if (netEvents.lNetworkEvents & FD_WRITE)
         {
            handleEvent(info, FD_WRITE, netEvents.iErrorCode[FD_WRITE_BIT]);
         }
         if (netEvents.lNetworkEvents & FD_CLOSE)
         {
            handleEvent(info, FD_CLOSE, netEvents.iErrorCode[FD_CLOSE_BIT]);
         }
         if (netEvents.lNetworkEvents & FD_ACCEPT)
         {
            handleEvent(info, FD_ACCEPT, netEvents.iErrorCode[FD_ACCEPT_BIT]);
         }
      }
      std::cout << "Reactor event loop stopped" << std::endl;
   }
   void stop()
   {
      running_ = false;
   }
   bool isRunning() const
   {
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
         return;
      }
      try
      {
         switch (eventType)
         {
         case FD_READ:
         {
            if (info->readCallback)
            {
               info->readCallback(info->socket, errorCode);
            }
            else if (defaultReadCallback_)
            {
               defaultCloseCallback_(info->socket, errorCode);
            }
            break;
         }
         case FD_WRITE:
         {
            if (info->writeCallback)
            {
               info->readCallback(info->socket, errorCode);
            }
            else if (defaultWriteCallback_)
            {
               defaultWriteCallback_(info->socket, errorCode);
            };
            break;
         }
         case FD_CLOSE:
         {
            if (info->closeCallback)
            {
               info->closeCallback(info->socket, errorCode);
            }
            else if (defaultCloseCallback_)
            {
               defaultCloseCallback_(info->socket, errorCode);
            }
            removeSocket(info->socket);
            break;
         }
         case FD_ACCEPT:
         {
            // 接收新连接
            SOCKET clientSocket = accept(info->socket, NULL, NULL);
            if (clientSocket != INVALID_SOCKET)
            {
               std::cout << "New Connection accepted: " << clientSocket << std::endl;
               addSocket(clientSocket);
            };
            break;
         }
         }
      }
      catch (const std::exception &e)
      {
         std::cerr << "Error in event handler: " << e.what() << std::endl;
      }
   };
};
// 示例试用:简单的Echo 服务器
class EchoServer
{
public:
   EchoServer() : serverSocket_(INVALID_ATOM) {};
   ~EchoServer()
   {
      stop();
   }
   void start(int port = 8080)
   {
      // 创建服务器socket
      serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (serverSocket_ == INVALID_SOCKET)
      {
         throw std::runtime_error("Failed to create socket");
      }

      // 设置地址重用
      int yes = 1;
      if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) == SOCKET_ERROR)
      {
         closesocket(serverSocket_);
         throw std::runtime_error("setsockopt failed");
      }

      // 绑定地址
      sockaddr_in serverAddr{};
      serverAddr.sin_family = AF_INET;
      serverAddr.sin_addr.s_addr = INADDR_ANY;
      serverAddr.sin_port = htons(port);

      if (bind(serverSocket_, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
      {
         closesocket(serverSocket_);
         throw std::runtime_error("bind failed");
      }

      // 开始监听
      if (listen(serverSocket_, SOMAXCONN) == SOCKET_ERROR)
      {
         closesocket(serverSocket_);
         throw std::runtime_error("listen failed");
      }

      std::cout << "Echo server listening on port " << port << std::endl;

      // 设置默认回调
      reactor_.setDefaultReadCallback([this](SOCKET sock, int error)
                                      { handleRead(sock, error); });
   }
   void stop()
   {
      reactor_.stop();
      if (reactorThread_.joinable())
      {
         reactorThread_.join();
      }
      if (serverSocket_ != INVALID_SOCKET)
      {
         closesocket(serverSocket_);
         serverSocket_ = INVALID_SOCKET;
      }
   }

private:
   void handleRead(SOCKET sock, int error)
   {
      if (error != 0)
      {
         std::cerr << "Read error: " << error << std::endl;
         return;
      }
      char buffer[1024];
      int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
      if (bytesReceived > 0)
      {
         buffer[bytesReceived] = '\0';
         std::cout << "Received from " << sock << ": " << buffer << std::endl;

         // Echo 回数据
         send(sock, buffer, bytesReceived, 0);
      }
      else if (bytesReceived == 0)
      {
         std::cout << "Connection closed by client:" << sock << std::endl;
      }
      else
      {
         std::cerr << "recv failed:" << WSAGetLastError() << std::endl;
      }
   }
   SOCKET serverSocket_;
   ReactorEventLoop reactor_;
   std::thread reactorThread_;
};
int main(int, char **)
{
   try
   {
      std::cout << "Starting Reactor Event Loop Example..." << std::endl;
      EchoServer server;
      server.start(8080);

      std::cout << "Server started.Press Enter to stop..." << std::endl;
      std::cin.get();

      server.stop();
      std::cout << "Server stopped." << std::endl;
   }
   catch (const std::exception &e)
   {
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;
   }
   return 0;
}
