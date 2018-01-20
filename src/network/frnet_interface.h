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
	// log config {{{1
	const std::string& log_key();
	void set_log_config(const std::string& log_key, frpublic::eLogLevel log_level);
	//}}}1

	// enum {{{1
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
	// }}}1

	// class NetInterface {{{1
	class NetInterface{
		public:
			NetInterface();
			NetInterface(size_t min_cache_size, size_t max_cache_size);
			virtual ~NetInterface();
		public:
			virtual bool Start(const std::string& ip, Port port)=0;
			virtual bool Stop()=0;

			virtual void OnClose()=0;

			// include all error : read, write, disconnect and so on.
			virtual void OnError(const NetError& net_error)=0;

			inline size_t set_min_cache_size(size_t min_cache_size){ min_cache_size_ = min_cache_size; }
			inline size_t min_cache_size()const{ return min_cache_size_; }

			inline size_t set_max_cache_size(size_t max_cache_size){ max_cache_size_ = max_cache_size; }
			inline size_t max_cache_size()const{ return max_cache_size_; }
		private:
			size_t min_cache_size_;
			size_t max_cache_size_;
	};
	//}}}1

	// class AstractClient {{{1
	class AstractClient : public NetInterface{
		public:
			AstractClient();
			AstractClient(size_t min_cache_size, size_t max_cache_size);
			virtual ~AstractClient();
		public:
			virtual eNetSendResult Send(const frpublic::BinaryMemoryPtr& binary)=0;
		protected:
			// param[out] read_size : 
			//	delete date size when function finish. Set 0 If you do not want delete any data.
			//
			// retval : close client if return is false.
			virtual bool OnReceive(const frpublic::BinaryMemory& binary, size_t& read_size)=0;
	};
	//}}}1

	// class AstractServer {{{1
	class AstractServer : public NetInterface{
		public:
			AstractServer();
			AstractServer(size_t min_cache_size, size_t max_cache_size, int32_t max_listen_num);
			virtual ~AstractServer();
		public:
			virtual bool Disconnect(Socket sockfd)=0;

			virtual eNetSendResult Send(Socket sockfd, const frpublic::BinaryMemoryPtr& binary)=0;
		protected:
			// param[out] read_size : delete date size when function finish. Set 0 If you do not want delete any data.
			//
			// retval : close client if return is false.
			virtual bool OnReceive(Socket sockfd, const frpublic::BinaryMemory& binary, size_t& read_size)=0;

			virtual void OnConnect(Socket sockfd)=0;
			virtual void OnDisconnect(Socket sockfd)=0;

			inline int32_t max_listen_num(){ return max_listen_num_; }
		private:
			int32_t max_listen_num_;
	};
	//}}}1
	
}

#endif 

