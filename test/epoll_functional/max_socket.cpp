/**********************************************************
 *  \file max_socket.cpp
 *  \brief
 *  \note	注意事项： 
 * 
 * \version 
 * * \author zheng39562@163.com
**********************************************************/
#include "frpublic/pub_log.h"
#include "frpublic/pub_memory.h"
#include "frpublic/pub_tool.h"
#include "network/frnet_epoll.h"
#include <vector>

#include <sys/types.h> 
#include <signal.h> 

using namespace std;
using namespace frnet;
using namespace frpublic;

// Exit function list. {{{1

void AskToQuit();

void SignalHandle(int sign){
	switch(sign){
		case SIGINT: {
			AskToQuit();
			break;
		}
	}
}
void RegisterQuitSignal(){
	sighandler_t ret = signal(SIGINT, SignalHandle);
	if(ret != SIG_DFL || ret != SIG_IGN){
		if(ret == SIG_ERR){
			abort();
		}
	}
	else{
		abort();
	}
}
static bool g_is_quit = false;
static pthread_once_t frrpc_func_s_signal_handle_once = PTHREAD_ONCE_INIT;
void AskToQuit(){
	g_is_quit = true;
}
bool IsAskedToQuit(){
	pthread_once(&frrpc_func_s_signal_handle_once, RegisterQuitSignal);
	return g_is_quit;
}
//}}}1

string g_ip_ = "127.0.0.1";
int g_port_ = 12345;
int max_socket_num_ = 40000;

// class TestServer {{{1
class TestServer : public NetListen{
	public:
		TestServer():net_server_(CreateNetServer(this)), connect_count_(0), disconnect_count_(0){}
		virtual ~TestServer(){ if(net_server_ != NULL){ delete net_server_; net_server_ = NULL; } }
	public:
		inline bool Start(const std::string& ip, Port port){ return net_server_->Start(ip, port); }
		inline bool Stop(){ return net_server_->Stop(); }
	protected:
		virtual bool OnReceive(Socket sockfd, const frpublic::BinaryMemory& binary, size_t& read_size){ 
			BinaryMemoryPtr packet(new BinaryMemory(binary));
			net_server_->Send(sockfd, packet);
			return true; 
		}

		virtual void OnConnect(Socket sockfd){ ++connect_count_; }
		virtual void OnDisconnect(Socket sockfd){ ++disconnect_count_; }

		virtual void OnClose(){ } 
		virtual void OnError(const NetError& error){ } 
	public:
		int connect_count_;
		int disconnect_count_;
	private:
		NetServer* net_server_;
};
//}}}1

// class TestClient {{{1
class TestClient : public NetListen{
	public:
		TestClient():is_connect(true), net_client_(CreateNetClient(this)){}
		virtual ~TestClient(){ if(net_client_ != NULL){ delete net_client_; net_client_ = NULL; } }
	public:
		inline bool Start(const std::string& ip, Port port){ return net_client_->Start(ip, port); }
		inline bool Stop(){ return net_client_->Stop(); }

		inline eNetSendResult Send(const frpublic::BinaryMemoryPtr& binary){ return net_client_->Send(binary); }

		static int32_t client_disconnect_count(){ return static_client_disconnect_count_; }
	protected:
		virtual bool OnReceive(Socket sockfd, const frpublic::BinaryMemory& binary, size_t& read_size){ read_size = binary.size(); return false; }

		virtual void OnConnect(Socket sockfd){ is_connect = true; }
		virtual void OnDisconnect(Socket sockfd){ ; }

		virtual void OnClose(){ 
			is_connect = false;
			++static_client_disconnect_count_; 
		}
		virtual void OnError(const NetError& error){ }
	public:
		bool is_connect;
	private:
		static int static_client_disconnect_count_;
		NetClient* net_client_;
};
int TestClient::static_client_disconnect_count_ = 0;
//}}}1

// ServerProcess {{{1
void ServerProcess(){
	TestServer server;
	if(!server.Start(g_ip_, g_port_)){
		DEBUG_E("Fail to start server.");
	}

	DEBUG_I("server running.");
	while((max_socket_num_ != server.connect_count_) || (max_socket_num_ != server.disconnect_count_)){
		FrSleep(100);
	}

	if(!server.Stop()){
		DEBUG_E("Fail to stop server.");
	}

	DEBUG_I("server accept scoket [" << server.connect_count_ << "]");
	DEBUG_I("server disconnect scoket ["<< server.disconnect_count_ << "]");
}
//}}}1

// ClientProcess {{{1
void ClientProcess(){
	std::vector<TestClient*> clients;

	clients.reserve(max_socket_num_);
	for(int index = 0; index < max_socket_num_; ++index){
		clients.push_back(new TestClient());
	}
	
	for(auto& client : clients){
		if(!client->Start(g_ip_, g_port_)){
			DEBUG_E("Fail to start client ");
			return ;
		}
	}

	DEBUG_D("client running.");

	string msg("Hello tcp.....");
	BinaryMemoryPtr binary(new BinaryMemory());
	binary->add(msg.c_str(), msg.size());
	for(auto& client : clients){
		if(client->Send(binary) != eNetSendResult_Ok){
			DEBUG_E("Fail to send msg.");
			return ;
		}
	}

	bool is_quit(false);
	while(!is_quit){
		FrSleep(100);

		is_quit = true;
		for(auto& client : clients){
			if(client->is_connect){
				is_quit = false;
				break;
			}
		}
	}

	for(auto& client : clients){
		if(!client->Stop()){
			DEBUG_E("Fail to stop client ");
		}
		delete client;
		client = NULL;
	}

	DEBUG_D("All client is Stop. client receive close callback ["<< TestClient::client_disconnect_count() << "]");
}
//}}}1

// main {{{1
int main(int argc, char* argv[]){
	// check your ulimit -n : confirm it is bigger than number of test.
	
	string key("max_socket_client");

	pid_t pid = fork();
	//int pid = argc > 1 ? 0 : 123;
	if(pid < 0){
		DEBUG_E("error");
	}
	else if(pid == 0){
		//child
		FrSleep(2000);

		frpublic::SingleLogServer::GetInstance()->InitLog("./log", 10 * 1024 * 1024);
		frpublic::SingleLogServer::GetInstance()->set_log_level(key, frpublic::eLogLevel_Info);
		frpublic::SingleLogServer::GetInstance()->set_default_log_key(key);
		frnet::set_log_config(key, frpublic::eLogLevel_Program);

		ClientProcess();
	}
	else{
		key = "max_socket_server";

		frpublic::SingleLogServer::GetInstance()->InitLog("./log", 10 * 1024 * 1024);
		frpublic::SingleLogServer::GetInstance()->set_log_level(key, frpublic::eLogLevel_Info);
		frpublic::SingleLogServer::GetInstance()->set_default_log_key(key);
		frnet::set_log_config(key, frpublic::eLogLevel_Program);

		//father
		ServerProcess();
	}

	return 0;
}
//}}}1

