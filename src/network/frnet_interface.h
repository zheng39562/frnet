/**********************************************************
 *  \file frnet_interface.h
 *  \brief
 *  \note	注意事项： 
 * 
 * \version 
 * * \author zheng39562@163.com
**********************************************************/
#ifndef _frnet_interface_H
#define _frnet_interface_H

#include "frnet_define.h"
#include "frpublic/pub_memory.h"
#include "frpublic/pub_log.h"

namespace frnet{

	// log config
	const std::string& log_key();
	void set_log_config(const std::string& log_key, frpublic::eLogLevel log_level);

	// enum 
	enum eNetSendResult{
		eNetSendResult_Ok = 0,
		eNetSendResult_PacketBigger,
		eNetSendResult_CacheIsFull,
		eNetSendResult_Disconnect,
		eNetSendResult_NotFound,
		eNetSendResult_SendFailed,
		eNetSendResult_End,
	};

	enum eNetErrorNo{
		eNetErrorNo_Invalid_Begin = 0,
		eNetErrorNo_Invalid_End
	};

	struct NetError{
		Socket sockfd;
		eNetErrorNo err_no;
	};

	// class NetListen 
	class NetInterface;
	class NetListen{
		public:
			friend class NetInterface;
		public:
			NetListen()=default;
			virtual ~NetListen()=default;
		protected:
			// param[out] read_size : 
			//	delete date size when function finish. Set 0 If you do not want delete any data.
			//
			// retval : close client if return is false.
			virtual bool OnReceive(Socket sockfd, const frpublic::BinaryMemory& binary, size_t& read_size)=0;

			virtual void OnConnect(Socket sockfd)=0;
			virtual void OnDisconnect(Socket sockfd)=0;

			virtual void OnClose()=0;

			// include all error : read, write, disconnect and so on.
			virtual void OnError(const NetError& net_error)=0;
	};
	
	// class NetInterface 
	class NetInterface{
		public:
			NetInterface(NetListen* listen);
			virtual ~NetInterface();
		public:
			// configure must set before start.
			// Is you want use configure in a running object.You must set and restart it.
			// Confiure :
			//	* set read/write cache.
			virtual bool Start(const std::string& ip, Port port)=0;
			virtual bool Stop()=0;

			inline size_t set_read_cache_size(size_t read_cache_size){ read_cache_size_ = read_cache_size; }
			inline size_t read_cache_size()const{ return read_cache_size_; }

			inline size_t set_write_cache_size(size_t write_cache_size){ write_cache_size_ = write_cache_size; }
			inline size_t write_cache_size()const{ return write_cache_size_; }
		protected:
			inline bool OnReceive(Socket sockfd, const frpublic::BinaryMemory& binary, size_t& read_size){ return listen_->OnReceive(sockfd, binary, read_size); }
			inline void OnConnect(Socket sockfd){ return listen_->OnConnect(sockfd); }
			inline void OnDisconnect(Socket sockfd){ return listen_->OnDisconnect(sockfd); }

			inline void OnClose(){ return listen_->OnClose(); }

			// include all error : read, write, disconnect and so on.
			inline void OnError(const NetError& net_error){ return listen_->OnError(net_error); }
		private:
			size_t write_cache_size_;
			size_t read_cache_size_;
			NetListen* listen_;
	};

	// class NetClient
	class NetClient : public NetInterface{
		public:
			NetClient(NetListen* listen);
			virtual ~NetClient();
		public:
			virtual eNetSendResult Send(const frpublic::BinaryMemoryPtr& binary)=0;
	};

	// class NetServer
	class NetServer : public NetInterface{
		public:
			NetServer(NetListen* listen);
			virtual ~NetServer();
		public:
			virtual bool Disconnect(Socket sockfd)=0;

			virtual eNetSendResult Send(Socket sockfd, const frpublic::BinaryMemoryPtr& binary)=0;

			// set num before start.
			inline void set_work_thread_num(int32_t work_thread_num){ work_thread_num_ = work_thread_num; }
			inline int32_t work_thread_num()const { return work_thread_num_; }
		protected:
			inline int32_t max_listen_num()const { return max_listen_num_; }
		private:
			int32_t max_listen_num_;
			int32_t work_thread_num_;
	};
	
}

// Create Function
extern "C"{
	frnet::NetClient* CreateNetClient(frnet::NetListen* listen);
	frnet::NetServer* CreateNetServer(frnet::NetListen* listen);
}

#endif 

