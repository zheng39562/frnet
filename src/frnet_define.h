/**********************************************************
 *  \file frnet_define.h
 *  \brief
 *  \note	注意事项： 
 * 
 * \version 
 * * \author zheng39562@163.com
**********************************************************/
#ifndef _frnet_define_H
#define _frnet_define_H

// typedef {{{2
//
#include <stdint.h>

typedef int32_t Socket;
typedef uint32_t Port;
typedef uint8_t Byte;

#ifndef __Independent_Log

#define LOG_KEY "network"
#define NET_DEBUG_P(msg) K_DEBUG_P(LOG_KEY, msg)
#define NET_DEBUG_D(msg) K_DEBUG_D(LOG_KEY, msg)
#define NET_DEBUG_I(msg) K_DEBUG_I(LOG_KEY, msg)
#define NET_DEBUG_W(msg) K_DEBUG_W(LOG_KEY, msg)
#define NET_DEBUG_E(msg) K_DEBUG_E(LOG_KEY, msg)
#define NET_DEBUG_C(msg) K_DEBUG_C(LOG_KEY, msg)

#else

#include "rpc/frrpc_log.h"
#define NET_DEBUG_P(msg) RPC_DEBUG_P(msg) 
#define NET_DEBUG_D(msg) RPC_DEBUG_D(msg)
#define NET_DEBUG_I(msg) RPC_DEBUG_I(msg)
#define NET_DEBUG_W(msg) RPC_DEBUG_W(msg)
#define NET_DEBUG_E(msg) RPC_DEBUG_E(msg)
#define NET_DEBUG_C(msg) RPC_DEBUG_C(msg)

#endif

//}}}2

#endif 

