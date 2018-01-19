/**********************************************************
 *  \file frnet_tcp_asio.h
 *  \brief
 *  \note	注意事项： 
 * 
 * \version 
 * * \author zheng39562@163.com
**********************************************************/
#ifndef _frnet_tcp_asio_H
#define _frnet_tcp_asio_H

#ifdef __NET_USE_ASIO

#include "frnet_interface.h"

// I want use rpc on OS of window.And asio is a stable, simple library.
namespace frnet{
	class TcpClient_Asio : public NetorkInterface{
		public:
			TcpClient_Asio();
			virtual ~TcpClient_Asio();
	};

	class TcpServer_Asio : public NetorkInterface{
		public:
			TcpServer_Asio();
			virtual ~TcpServer_Asio();
	};
}

#endif

#endif 

