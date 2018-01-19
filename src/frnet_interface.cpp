/**********************************************************
 *  \file frnet_interface.cpp
 *  \brief
 *  \note	注意事项： 
 * 
 * \version 
 * * \author zheng39562@163.com
**********************************************************/
#include "frnet_interface.h"

// namespace frnet{{{1
namespace frnet{

//====================================================================================================
NetInterface::NetInterface()
	:min_cache_size_(1 * 1024), 
	 max_cache_size_(64 * 1024)
{}

NetInterface::NetInterface(size_t min_cache_size, size_t max_cache_size)
	:min_cache_size_(min_cache_size), 
	 max_cache_size_(max_cache_size)
{}

NetInterface::~NetInterface(){ }


//====================================================================================================
AstractClient::AstractClient()
	:NetInterface()
{}

AstractClient::AstractClient(size_t min_cache_size, size_t max_cache_size)
	:NetInterface(min_cache_size, max_cache_size)
{}
AstractClient::~AstractClient(){}


//====================================================================================================
AstractServer::AstractServer()
	:NetInterface(), max_listen_num_(65535)
{}

AstractServer::AstractServer(size_t min_cache_size, size_t max_cache_size, int32_t max_listen_num)
	 :NetInterface(min_cache_size, max_cache_size), 
	  max_listen_num_(max_listen_num)
{}

AstractServer::~AstractServer(){}

}
// }}}1

/*

//TCP Creator : boost::asio {{{2
#ifdef __TCP_USE_ASIO

#include "frnet_tcp_asio.h"

// use boost::asio
frnet::AstractClient* TcpClientInstance(){ return new frnet::TcpClient_Asio(); }
frnet::AstractServer* TcpServerInstance(){ return new frnet::TcpServer_Asio(); }

//}}}2

//TCP Creator : code self(linux epoll.) {{{2
#elif __TCP_USE_SELF
frnet::AstractClient* TcpClientInstance(){ return new frnet::TcpClient_Epoll(); }
frnet::AstractServer* TcpServerInstance(){ return new frnet::TcpServer_Epoll(); }

//// }}}2

#else
#endif
*/


