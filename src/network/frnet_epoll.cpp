/**********************************************************
 *  \file frnet_epoll.cpp
 *  \brief
 *  \note	注意事项： 
 * 
 * \version 
 * * \author zheng39562@163.com
**********************************************************/
#ifdef __FRNET_EPOLL

#ifdef WIN32
#include <WS2tcpip.h>
#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#endif

#include <memory.h>

#include "frpublic/pub_tool.h"
#include "frnet_epoll.h"

using namespace std;
using namespace frpublic;

namespace frnet{

// Class NetClient_Epoll {{{1

// NetClient_Epoll {{{2
NetClient_Epoll::NetClient_Epoll(NetListen* listen)
	:NetClient(listen),
	 write_queue_(),
	 write_binary_(),
	 read_binary_(),
	 sockfd_(0),
	 read_thread_(),
	 write_thread_(),
	 is_connect_(false)
{
	read_binary_.reserve(write_cache_size());
}// }}}2

NetClient_Epoll::~NetClient_Epoll(){// {{{2
	Stop();
}// }}}2

bool NetClient_Epoll::Start(const std::string& ip, Port port){// {{{2
	sockfd_ = BuildSocket();
	is_connect_ = TryConnect(sockfd_, ip, port);
	if(is_connect_){
		NET_DEBUG_D("connection is success. Create work threads(read and send).");
		read_thread_ = thread(&NetClient_Epoll::ReadProcess, this);
		write_thread_ = thread(&NetClient_Epoll::WriteProcess, this);
	}
	else{
		NET_DEBUG_E("Fail to connect.");
		return false;
	}

	return true;
}// }}}2

bool NetClient_Epoll::Stop(){// {{{2
	NET_DEBUG_D("Close connection.");
	if(sockfd_ > 0){
		close(sockfd_);
	}
	else{
		DEBUG_E("Fail to close client. errno [" << errno << "]");
	}

	is_connect_ = false;
	if(read_thread_.joinable()){
		read_thread_.join();
	}

	if(write_thread_.joinable()){
		write_thread_.join();
	}

	return true;
}// }}}2

eNetSendResult NetClient_Epoll::Send(const frpublic::BinaryMemoryPtr& binary){// {{{2
	return write_queue_.push(binary) ? eNetSendResult_Ok : eNetSendResult_CacheIsFull;
}// }}}2

Socket NetClient_Epoll::BuildSocket(){// {{{2
	Socket sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sockfd != -1){
		int flags = fcntl(sockfd, F_GETFL, 0);
		fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK);

		int cache_size(0);
		cache_size = read_cache_size();
		setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&cache_size, sizeof(int));
		cache_size = write_cache_size();
		setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&cache_size, sizeof(int));

		NET_DEBUG_D("Build socket : [" << sockfd << "]. Block and cache [" << cache_size << "]");
	}

	return sockfd;
}// }}}2

bool NetClient_Epoll::TryConnect(Socket sockfd, const std::string& ip, Port port){// {{{2
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip.c_str(), &address.sin_addr);
	address.sin_port = htons(port);

	if(::connect(sockfd, (struct sockaddr *)&address, sizeof(address)) != 0){
		NET_DEBUG_E("Fail to call connect function. errno[" << errno << "] ip:port [" << ip << ":" << port << "].");
		return false;
	}
	return true;
}// }}}2

void NetClient_Epoll::ReadProcess(){// {{{2
	auto CallOnReceive = [&](){
		size_t read_size(0);
		if(!OnReceive(sockfd_, read_binary_, read_size)){;
			DEBUG_W("Receive has error. Close client.");
			close(sockfd_);
		}
		read_binary_.del(read_size);
	};

	read_binary_.clear();
	while(is_connect_){
		if(read_binary_.usable_size() == 0){
			int re_call_times(0);
			do{
				FrSleep(pow(10, re_call_times++));
				CallOnReceive();

				if(re_call_times >= 4){
					if(close(sockfd_) < 0){
						DEBUG_E("Fail to close socket. errno [" << errno << "]");
					}
					read_binary_.clear();
				}
			} while(read_binary_.usable_size() == 0);
		}

		int recv_size = ::recv(sockfd_, read_binary_.buffer(), read_binary_.usable_size(), 0);
		if(recv_size > 0){
			read_binary_.CopyMemoryFromOut(recv_size);
			CallOnReceive();
		}
		else if(recv_size == 0){
			is_connect_ = false;
		}
		else{
			if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN){
				FrSleep(1);
			}
			else{
				// EBADF, EFAULT, EINVAL, ENOMEM, ENOTCONN
				is_connect_ = false;
			}
		}
	}

	DEBUG_D("Active Close Callback.");
	OnClose();
}// }}}2

void NetClient_Epoll::WriteProcess(){// {{{2
	while(is_connect_){
		if(write_binary_ == NULL || write_binary_->buffer() == NULL){
			write_queue_.pop(write_binary_);
		}

		if(write_binary_ != NULL && write_binary_->buffer() != NULL){
			int send_size = ::send(sockfd_, write_binary_->buffer(), write_binary_->size(), 0);
			if(send_size > 0){
				if(write_binary_->size() <= send_size){
					write_binary_.reset();
				}
				else{
					write_binary_->del(send_size);
				}
			}
			else{
				if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN){
					FrSleep(1);
				}
				else{
					// EBADF, EFAULT, EINVAL, ENOMEM, ENOTCONN
					is_connect_ = false;
				}
			}
		}
		else{
			FrSleep(1);
		}
	}
}// }}}2

// }}}1
	
// Class NetServer_Epoll {{{1
//

NetServer_Epoll::NetServer_Epoll(NetListen* listen)// {{{2
	:NetServer(listen),
	 write_queue_(),
	 event_active_queue_(),
	 work_threads_(),
	 epoll_thread_(),
	 socket_2cache_ptr_(),
	 listen_sockfd_(0),
	 epoll_sockfd_(0),
	 is_running_(false)
{
	set_work_thread_num(4);
}//}}}2

NetServer_Epoll::~NetServer_Epoll(){// {{{2
	Stop();
}// }}}2

bool NetServer_Epoll::Start(const std::string& ip, Port port){// {{{2
	ClearVariables();

	is_running_ = true;

	listen_sockfd_ = CreateListenSocket();

	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip.c_str(), &address.sin_addr);
	address.sin_port = htons(port);
	int ret = ::bind(listen_sockfd_, (struct sockaddr*)&address, sizeof(address)); 
	if(ret == -1){ 
		NET_DEBUG_E("Fail to bind. errno " << errno); 
		return false;
	}
	
	ret = ::listen(listen_sockfd_, 300);
	if(ret == -1){ 
		NET_DEBUG_E("Fail to listen. errno " << errno); 
		return false;
	}

	epoll_thread_ = thread(&NetServer_Epoll::EpollProcess, this);
	for(int index = 0; index < work_thread_num(); ++index){
		work_threads_.push_back(thread(&NetServer_Epoll::WorkProcess, this));
	}

	NET_DEBUG_P("Server is listening [" << ip << ":" << port << "]");
	return true;
}// }}}2

bool NetServer_Epoll::Stop(){// {{{2
	ClearVariables();

	for(auto& work_thread : work_threads_){
		if(work_thread.joinable()){
			work_thread.join();
		}
	}

	if(epoll_thread_.joinable()){
		epoll_thread_.join();
	}
}// }}}2

bool NetServer_Epoll::Disconnect(Socket sockfd){// {{{2
	DEBUG_D("Close socket [" << sockfd << "]");

	int ret = close(sockfd);
	if(ret < 0){
		DEBUG_E("Fail to close socket. errno [" << errno << "]");
	}

	return ret == 0;
}// }}}2

eNetSendResult NetServer_Epoll::Send(Socket sockfd, const frpublic::BinaryMemoryPtr& binary){// {{{2
	if(is_running_ && !binary->empty()){
		return write_queue_.push(TcpPacket(sockfd, binary)) ? eNetSendResult_Ok : eNetSendResult_SendFailed;
	}
	return eNetSendResult_SendFailed;
}// }}}2

Socket NetServer_Epoll::CreateListenSocket(){// {{{2
	Socket sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sockfd > 0){
		int flags = fcntl(sockfd, F_GETFL, 0);
		fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

		int resuse_addr_set(1);
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &resuse_addr_set, sizeof(resuse_addr_set));

		NET_DEBUG_P("Build socket : [" << sockfd << "]. ");
	}

	return sockfd;
}// }}}2

void NetServer_Epoll::EpollProcess(){// {{{2
	NET_DEBUG_D("Epoll thread run.");

	epoll_sockfd_ = epoll_create(max_listen_num());

	epoll_event event;
	event.data.fd = listen_sockfd_;
	event.events = EPOLLIN | EPOLLOUT | EPOLLET;
	if(epoll_ctl(epoll_sockfd_, EPOLL_CTL_ADD, listen_sockfd_, &event) == -1){
		is_running_ = false;
		NET_DEBUG_E("Fail to add listen sockfd to epoll.");
		close(epoll_sockfd_);
		return ;
	}

	int32_t event_length(30);
	epoll_event* events = (epoll_event*)calloc(event_length, sizeof(epoll_event));
	if(events == NULL){
		DEBUG_E("Fail to calloc memory. ");
		return ;
	}
	
	int32_t next_fetch_number(300);
	while(is_running_){
		memset(events, 0, event_length * sizeof(epoll_event));
		int recv_event_num = epoll_wait(epoll_sockfd_, events, event_length, 50); // 50ms timeout

		for(int index = 0; index < recv_event_num; ++index){
			if(events[index].data.fd == listen_sockfd_){
				PerformEpollEventFromListen(events[index]);
			}
			else{
				PerformEpollEvent(events[index]);
			}
		}

		RecvMessageFromWriteQueue(next_fetch_number);
	}

	close(epoll_sockfd_);
	
	NET_DEBUG_P("Epoll thread Stop.");
}// }}}2

void NetServer_Epoll::WorkProcess(){// {{{2
	NET_DEBUG_P("Work thread run.");

	EventInfo event_info;
	while(is_running_){
		while(is_running_ && event_active_queue_.empty()){
			FrSleep(1);
		}

		if(event_active_queue_.pop(event_info)){
			NET_DEBUG_D("work thread : receive event. socket [" << event_info.cache->sockfd << "] type [" << GetTypeName(event_info.type) << "]");

			switch(event_info.type){
				case eEpollEventType_Read:
					Read(event_info.cache);
					break;
				case eEpollEventType_Write:
					Write(event_info.cache);
					break;
				case eEpollEventType_Connect:
					OnConnect(event_info.cache->sockfd);
					break;
				case eEpollEventType_DisConnect:
					OnDisconnect(event_info.cache->sockfd);
					break;
			}
		}
	}

	NET_DEBUG_P("Work thread Stop.");
}// }}}2

void NetServer_Epoll::RecvMessageFromWriteQueue(int32_t number){// {{{2
	TcpPacket tcp_packet;
	while(--number >= 0 && !write_queue_.empty() && write_queue_.pop(tcp_packet)){
		auto socket_2cache_ptr_iter = socket_2cache_ptr_.find(tcp_packet.sockfd);
		if(socket_2cache_ptr_iter != socket_2cache_ptr_.end()){
			socket_2cache_ptr_iter->second->write_queue.push(tcp_packet.write_binary);

			if(socket_2cache_ptr_iter->second->can_write){
				event_active_queue_.push(EventInfo(eEpollEventType_Write, socket_2cache_ptr_iter->second));
			}
		}
		else{
			NET_DEBUG_P("socket is not exist, or it was disconnected.");
		}
	}
}// }}}2

// }}}2

void NetServer_Epoll::PerformEpollEventFromListen(const epoll_event& event){// {{{2
	// error and disconnect
	if((event.events & EPOLLHUP) || (event.events & EPOLLERR)){
		close(listen_sockfd_);

		OnClose();
		ClearVariables();
		return;
	}

	// read
	if((event.events & EPOLLIN) || (event.events & EPOLLPRI)){ 
		struct sockaddr_in address;
		socklen_t len = sizeof(address);
		while(true){
			bzero(&address, sizeof(address));
			Socket sockfd = accept(listen_sockfd_, (sockaddr*)&address, &len);
			if(sockfd > 0){
				int flags = fcntl(sockfd, F_GETFL, 0);
				fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

				int cache_size(0);
				cache_size = read_cache_size();
				setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&cache_size, sizeof(int));
				cache_size = write_cache_size();
				setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&cache_size, sizeof(int));

				SocketCachePtr cache(new SocketCache(sockfd, write_cache_size()));
				socket_2cache_ptr_.insert(make_pair(sockfd, cache));

				epoll_event event;
				event.data.fd = sockfd;
				event.events = EPOLLIN | EPOLLOUT | EPOLLET;
				if(epoll_ctl(epoll_sockfd_, EPOLL_CTL_ADD, sockfd, &event) == -1){
					NET_DEBUG_E("更新epoll状态失败。");
				}

				event_active_queue_.push(EventInfo(eEpollEventType_Connect, cache));
			}
			else{
				if(errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN){
					NET_DEBUG_E("accept return socket [" << sockfd << "] errno [" << errno << "]");
				}
				break;
			}
		}
	}

	// write
	if(event.events & EPOLLOUT){
		DEBUG_E("Recv write active on listen socket.");
	}
}// }}}2

void NetServer_Epoll::PerformEpollEvent(const epoll_event& event){// {{{2
	Socket sockfd = event.data.fd;
	auto socket_2cache_ptr_iter = socket_2cache_ptr_.find(sockfd);
	if(socket_2cache_ptr_iter != socket_2cache_ptr_.end()){
		if((event.events & EPOLLHUP) || (event.events & EPOLLERR)){
			// error and disconnect
			close(sockfd);

			NET_DEBUG_P("socket disconnect.");
			event_active_queue_.push(EventInfo(eEpollEventType_Connect, socket_2cache_ptr_iter->second));
			socket_2cache_ptr_.erase(socket_2cache_ptr_iter);
		}
		else{
			// read and write
			if((event.events & EPOLLIN) || (event.events & EPOLLPRI)){ 
				event_active_queue_.push(EventInfo(eEpollEventType_Read, socket_2cache_ptr_iter->second));
			}

			if(event.events & EPOLLOUT){
				if(socket_2cache_ptr_iter->second->write_binary == NULL && socket_2cache_ptr_iter->second->write_queue.empty()){
					socket_2cache_ptr_iter->second->can_write = true;
				}
				else{
					event_active_queue_.push(EventInfo(eEpollEventType_Read, socket_2cache_ptr_iter->second));
				}
			}
		}
	}
	else{
		NET_DEBUG_P("Can not find sockfd [" << sockfd << "]");
	}
}// }}}2

void NetServer_Epoll::ClearVariables(){// {{{2
	NET_DEBUG_P("Clear variables.");

	is_running_ = false;
	socket_2cache_ptr_.clear();

	if(listen_sockfd_ > 0){
		close(listen_sockfd_);
	}
	listen_sockfd_ = 0;

	if(epoll_sockfd_ > 0){
		close(epoll_sockfd_);
	}
	epoll_sockfd_ = 0;

	TcpPacket tcp_packet(0, BinaryMemoryPtr());
	while(!write_queue_.empty()){
		write_queue_.pop(tcp_packet);
	}

	EventInfo event_info(eEpollEventType_Error, SocketCachePtr());
	while(!write_queue_.empty()){
		event_active_queue_.pop(event_info);
	}
}// }}}2

void NetServer_Epoll::Read(SocketCachePtr cache){// {{{2
	int recv_size(0);
	do{
		if(recv_size > 0 || cache->read_binary.usable_size() == 0){
			size_t read_size(0);
			if(!OnReceive(cache->sockfd, cache->read_binary, read_size)){
				DEBUG_W("Receive has error. Close socket [" << cache->sockfd << "]");
				close(cache->sockfd);
				return ;
			}
			cache->read_binary.del(read_size);

			if(cache->read_binary.usable_size() == 0){
				close(cache->sockfd);
				return ;
			}
		}

		recv_size = ::recv(cache->sockfd, cache->read_binary.buffer(), cache->read_binary.usable_size(), MSG_DONTWAIT);
		if(recv_size > 0){
			cache->read_binary.CopyMemoryFromOut(recv_size);
		}
		else if(recv_size == 0){
			OnDisconnect(cache->sockfd);
		}
		else{
			if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN){
				epoll_event event;
				event.data.fd = cache->sockfd;
				event.events = EPOLLIN | EPOLLET;
				if(epoll_ctl(epoll_sockfd_, EPOLL_CTL_MOD, cache->sockfd, &event) == -1){
					NET_DEBUG_E("epoll cntl error : modify socket(" << cache->sockfd << ") failed. errno [" << errno << "]");
				}
			}
			else{
				// EBADF, EFAULT, EINVAL, ENOMEM, ENOTCONN
				close(cache->sockfd);
			}
			break;
		}
	}while(recv_size > 0);
}// }}}2

void NetServer_Epoll::Write(SocketCachePtr cache){// {{{2
	while(true){
		if(cache->write_binary == NULL){
			if(cache->write_queue.empty()){
				break;
			}

			if(!cache->write_queue.pop(cache->write_binary) || cache->write_binary == NULL){
				NET_DEBUG_E("Fail to fetch binary of write(or binary is null/empty.).");
				break;
			}
		}

		int send_size = ::send(cache->sockfd, cache->write_binary->buffer(), cache->write_binary->size(), MSG_DONTWAIT);
		if(send_size > 0){
			cache->write_binary->del(send_size);
			if(cache->write_binary->empty()){
				cache->write_binary.reset();
			}
		}
		else{
			if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN){
				epoll_event event;
				event.data.fd = cache->sockfd;
				event.events = EPOLLOUT | EPOLLET;
				if(epoll_ctl(epoll_sockfd_, EPOLL_CTL_MOD, cache->sockfd, &event) == -1){
					NET_DEBUG_E("epoll cntl error : modify socket(" << cache->sockfd << ") failed. errno [" << errno << "]");
				}
			}
			else{
				NET_DEBUG_E("Fail to send data. socket[" << cache->sockfd << "] errno [" << errno << "]");
			}
			break;
		}
	}
}// }}}2

//}}}1

} // frnet

#endif

