/**********************************************************
 *  \file frnet_epoll.h
 *  \brief
 *  \note	注意事项： 
 * 
 * \version 
 * * \author zheng39562@163.com
**********************************************************/
#ifndef _frnet_epoll_H
#define _frnet_epoll_H

#ifdef __FRNET_EPOLL

#ifndef WIN32
#include <sys/epoll.h>
#include <unistd.h>
#endif

#include <thread>
#include <atomic>
#include <vector>

#include "frtemplate/lock_queue.hpp"
#include "frpublic/pub_memory.h"
#include "network/frnet_interface.h"

namespace frnet{

	// NetClient_Epoll {{{1
	class NetClient_Epoll : public AstractClient{
		public:
			NetClient_Epoll();
			NetClient_Epoll(size_t _min_cache_size, size_t _max_cache_size);
			virtual ~NetClient_Epoll();
		public:
			virtual bool Start(const std::string& ip, Port port);
			virtual bool Stop();

			virtual eNetSendResult Send(const frpublic::BinaryMemoryPtr& binary);
		private:
			Socket BuildSocket();

			bool TryConnect(Socket sockfd, const std::string& ip, Port port); 

			void ReadProcess();
			void WriteProcess();
		private:
			frtemplate::LockQueue<frpublic::BinaryMemoryPtr> write_queue_;

			frpublic::BinaryMemoryPtr write_binary_;
			// default size of memory : max_cache_size();
			frpublic::BinaryMemory read_binary_;

			Socket sockfd_;
			std::thread read_thread_;
			std::thread write_thread_;

			std::atomic<bool> is_connect_;
	};
	//}}}1

	// NetServer_Epoll {{{1
	class NetServer_Epoll : public AstractServer{
		public:
			enum eEpollEventType{// {{{2
				eEpollEventType_Read = 0,
				eEpollEventType_Write,
				eEpollEventType_Connect,
				eEpollEventType_DisConnect,
				eEpollEventType_Error,
			};// }}}2

			struct SocketCache{// {{{2
				SocketCache(Socket _sockfd, int32_t max_cache_size):sockfd(_sockfd),write_binary(),write_queue(),read_binary(),can_read(true),can_write(true),is_connect(true){
					read_binary.reserve(max_cache_size);
				}

				Socket sockfd;
				frpublic::BinaryMemoryPtr write_binary;
				frtemplate::LockQueue<frpublic::BinaryMemoryPtr> write_queue;
				frpublic::BinaryMemory read_binary;
				bool can_read;
				bool can_write;
				bool is_connect;
			};
			typedef std::shared_ptr<SocketCache> SocketCachePtr;
			// }}}2

			struct EventInfo{// {{{2
				EventInfo():type(eEpollEventType_Error), cache(){}
				EventInfo(eEpollEventType _type, SocketCachePtr _cache):type(_type), cache(_cache){}

				eEpollEventType type;
				SocketCachePtr cache;
			};// }}}2

			struct TcpPacket{// {{{2
				TcpPacket():sockfd(0), write_binary(){}
				TcpPacket(Socket _sockfd, frpublic::BinaryMemoryPtr _binary):sockfd(_sockfd), write_binary(_binary){}

				Socket sockfd;
				frpublic::BinaryMemoryPtr write_binary;
			};
			//}}}2

		public:
			NetServer_Epoll();
			NetServer_Epoll(size_t _min_cache_size, size_t _max_cache_size, int32_t _max_listen_num, size_t _work_thread_num = 4);
			virtual ~NetServer_Epoll();
		public:
			virtual bool Start(const std::string& ip, Port port);
			virtual bool Stop();

			virtual bool Disconnect(Socket sockfd);

			virtual eNetSendResult Send(Socket sockfd, const frpublic::BinaryMemoryPtr& binary);
		private:
			Socket CreateListenSocket();

			void EpollProcess();
			void WorkProcess();

			void RecvMessageFromWriteQueue(int32_t number);

			void PerformEpollEventFromListen(const epoll_event& event);
			void PerformEpollEvent(const epoll_event& event);

			void ClearVariables();

			void Read(SocketCachePtr cache);
			void Write(SocketCachePtr cache);

			inline std::string GetTypeName(eEpollEventType type)const{
				switch(type){
					case eEpollEventType_Read: return "Read";
					case eEpollEventType_Write: return "Write";
					case eEpollEventType_Connect: return "Connect";
					case eEpollEventType_DisConnect: return "Disconnect";
					default : return "unknow error";
				}
			}
		private:
			frtemplate::LockQueue<TcpPacket> write_queue_;
			frtemplate::LockQueue<EventInfo> event_active_queue_;
			std::vector<std::thread> work_threads_;
			std::thread epoll_thread_;
			std::map<Socket, SocketCachePtr> socket_2cache_ptr_;
			Socket listen_sockfd_;
			Socket epoll_sockfd_;
			std::atomic<bool> is_running_;
			size_t work_thread_num_; 
	};
	//}}}1

}

#endif 

#endif 

