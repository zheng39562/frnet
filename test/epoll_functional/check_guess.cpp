/**********************************************************
 *  \file main.cpp
 *  \brief
 *  \note	注意事项： 
 * 
 * \version 
 * * \author zheng39562@163.com
**********************************************************/
#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <memory.h>
#include <unistd.h>
#include <sys/epoll.h>

using namespace std;

typedef int Socket;

string ip = "127.0.0.1";
int port = 12345;

void ClientProcess(){ // {{{1
	cout << "Client : start." << endl;
	Socket sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sockfd != -1){
		int flags = fcntl(sockfd, F_GETFL, 0);
		fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK);

		struct timeval fd_time_out;
		fd_time_out.tv_sec = 0;
		fd_time_out.tv_usec = 500 * 1000;
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &fd_time_out, sizeof(fd_time_out));
	}

	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip.c_str(), &address.sin_addr);
	address.sin_port = htons(port);

	if(::connect(sockfd, (struct sockaddr *)&address, sizeof(address)) != 0){
		cout << "Fail to call connect function. errno[" << errno << "] ip:port [" << ip << ":" << port << "]." << endl;
		return ;
	}

	cout << "Client : connect success." << endl;
	int index(0);
	while(++index < 10){
		char buffer[10];
		int recv_size = ::recv(sockfd, buffer, 10, 0);
		cout << "Client : recv_size [" << recv_size << "] errno [" << errno << "]" << endl;
	}
	close(sockfd);
}
//}}}1

void ServerProcess(){// {{{2
	cout << "Server : start." << endl;
	Socket listen_sockfd_ = socket(PF_INET, SOCK_STREAM, 0);
	if(listen_sockfd_ > 0){
		/*
		int flags = fcntl(listen_sockfd_, F_GETFL, 0);
		fcntl(listen_sockfd_, F_SETFL, flags | O_NONBLOCK);
		*/
		int flags = fcntl(listen_sockfd_, F_GETFL, 0);
		fcntl(listen_sockfd_, F_SETFL, flags & ~O_NONBLOCK);

		int resuse_addr_set(1);
		setsockopt(listen_sockfd_, SOL_SOCKET, SO_REUSEADDR, &resuse_addr_set, sizeof(resuse_addr_set));

		cout << "Build socket : [" << listen_sockfd_ << "]. " << endl;
	}

	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip.c_str(), &address.sin_addr);
	address.sin_port = htons(port);
	int ret = ::bind(listen_sockfd_, (struct sockaddr*)&address, sizeof(address)); 
	if(ret == -1){ 
		cout << "Fail to bind. errno " << errno << endl; 
		return ;
	}
	
	ret = ::listen(listen_sockfd_, 300);
	if(ret == -1){ 
		cout << "Fail to listen. errno " << errno << endl; 
		return ;
	}

	cout << "Server : running." << endl;
	struct sockaddr_in address_t;
	socklen_t len = sizeof(address_t);
	bzero(&address_t, sizeof(address_t));
	Socket sockfd = accept(listen_sockfd_, (sockaddr*)&address_t, &len);
	if(sockfd > 0){
		cout << "Server: recev connect socket " << endl;
		close(sockfd);
		close(listen_sockfd_);
		cout << "Server: close listen socket " << endl;
	}

	Socket epoll_sockfd_ = epoll_create(10);

	// 已经关闭
	epoll_event event;
	event.data.fd = sockfd;
	event.events = EPOLLIN | EPOLLET;
	if(epoll_ctl(epoll_sockfd_, EPOLL_CTL_MOD, sockfd, &event) == -1){
		cout << "Server: Already closed. epoll cntl error : modify socket(" << sockfd << ") failed. errno [" << errno << "]" << endl;
	}

	// 不存在
	event.data.fd = sockfd + 17;
	event.events = EPOLLIN | EPOLLET;
	if(epoll_ctl(epoll_sockfd_, EPOLL_CTL_MOD, sockfd + 17, &event) == -1){
		cout << "Server: Not exist. epoll cntl error : modify socket(" << sockfd << ") failed. errno [" << errno << "]" << endl;
	}

	cout << "Server: end " << endl;
}//}}}2

int main(){
	thread t1 = thread(ServerProcess);

	sleep(1);
	thread t2 = thread(ClientProcess);

	if(t2.joinable()){
		t2.join();
	}

	if(t1.joinable()){
		t1.join();
	}

	return 0;
}

