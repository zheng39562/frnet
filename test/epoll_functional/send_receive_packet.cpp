/**********************************************************
 *  \file send_receive_packet.cpp
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
#include <thread>
#include <mutex>

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
int max_socket_num_ = 2;
bool g_has_exception_ = false;
int g_max_packet_size_ = 500;

// class TestServer {{{1
class TestServer : public NetListen{
	public:
		TestServer():net_server_(CreateNetServer(this)), send_size_(0), read_size_(0){}
		virtual ~TestServer(){ if(net_server_ != NULL){ delete net_server_; net_server_ = NULL; } }
	public:
		inline bool Start(const std::string& ip, Port port){ return net_server_->Start(ip, port); }
		inline bool Stop(){ return net_server_->Stop(); }

		inline uint64_t send_size()const{ return send_size_; }
		inline uint64_t read_size()const{ return read_size_; }
	protected:
		virtual bool OnReceive(Socket sockfd, const frpublic::BinaryMemory& binary, size_t& read_size){ 
			read_size_ += binary.size();

			BinaryMemoryPtr packet(new BinaryMemory(binary));
			if(net_server_->Send(sockfd, packet) == eNetSendResult_Ok){
				send_size_ += packet->size();
				read_size = binary.size();
				return true;
			}
			return false;
		}

		virtual void OnConnect(Socket sockfd){}
		virtual void OnDisconnect(Socket sockfd){}

		virtual void OnClose(){ } 
		virtual void OnError(const NetError& error){ } 
	private:
		uint64_t send_size_;
		uint64_t read_size_;
		NetServer* net_server_;
};
//}}}1

// class TestClient {{{1
class TestClient : public NetListen{
	public:
		TestClient()
			:send_size_(0), read_size_(0),
			 net_client_(CreateNetClient(this)),
			 mutex_wait_check_msg_(),
			 wait_check_msg_(),
			 send_binary_(new BinaryMemory())
		{
			send_binary_->reserve(g_max_packet_size_ + 2);
		}
		virtual ~TestClient(){ if(net_client_ != NULL){ delete net_client_; net_client_ = NULL; } }
	public:
		inline bool Start(const std::string& ip, Port port){ return net_client_->Start(ip, port); }
		inline bool Stop(){ return net_client_->Stop(); }

		bool Send(){ 
			size_t send_size = (rand() % g_max_packet_size_) + 1;

			send_binary_->clear();
			char data(0);
			for(size_t index = 0; index < send_size; ++index){
				data = (rand() % 255);
				send_binary_->add(&data, sizeof(char));
			}

			if(net_client_->Send(send_binary_) == eNetSendResult_Ok){
				send_size_ += send_binary_->size();
				std::lock_guard<std::mutex> lock(mutex_wait_check_msg_);
				wait_check_msg_.push_back(string((const char*)send_binary_->buffer(), send_binary_->size()));
				return true;
			}

			DEBUG_E("Fail to send msg [" << send_binary_->print() << "]");
			return false;
		}

		inline uint64_t send_size()const{ return send_size_; }
		inline uint64_t read_size()const{ return read_size_; }
	protected:
		virtual bool OnReceive(Socket sockfd, const frpublic::BinaryMemory& binary, size_t& read_size){ 
			std::lock_guard<std::mutex> lock(mutex_wait_check_msg_);

			string msg((const char*)binary.buffer(), binary.size());
			auto wait_check_msg_iter = wait_check_msg_.begin();
			if(wait_check_msg_iter == wait_check_msg_.end() || *wait_check_msg_iter != msg){
				BinaryMemory binary_tmp;
				binary_tmp.add(wait_check_msg_iter->c_str(), wait_check_msg_iter->size());
				DEBUG_E("Receive error : msg[" << binary.print() << "] check msg [" << binary_tmp.print() << "]");
				return false;
			}

			wait_check_msg_.erase(wait_check_msg_iter);
			read_size = binary.size();  

			read_size_ += read_size;
			return true;
		}

		virtual void OnConnect(Socket sockfd){}
		virtual void OnDisconnect(Socket sockfd){ g_has_exception_ = true; }

		virtual void OnClose(){ g_has_exception_ = true; }

		virtual void OnError(const NetError& error){ 
			DEBUG_E("receive errno [" << error.err_no << "]");
			g_has_exception_ = true;
		}
	private:
		uint64_t send_size_;
		uint64_t read_size_;
		NetClient* net_client_;
		std::mutex mutex_wait_check_msg_;
		std::vector<std::string> wait_check_msg_;
		frpublic::BinaryMemoryPtr send_binary_;
};
//}}}1

// ServerProcess {{{1
void ServerProcess(){
	TestServer server;
	if(!server.Start(g_ip_, g_port_)){
		DEBUG_E("Fail to start server.");
	}

	DEBUG_I("server running.");
	while(!IsAskedToQuit()){
		FrSleep(100);
	}

	if(!server.Stop()){
		DEBUG_E("Fail to stop server.");
	}

	DEBUG_I("send size [" << server.send_size() << "] read size [" << server.read_size() << "] ");
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

	DEBUG_I("client running.");

	bool is_break(false);
	while(!IsAskedToQuit() && !g_has_exception_ && !is_break){
		for(auto& client : clients){
			if(!client->Send()){
				DEBUG_E("Fail to send msg.");
				is_break = true;
				break;
			}
		}
		FrSleep(100);
	}

	cout << "wait stop.." << endl;

	uint64_t send_size(0);
	uint64_t read_size(0);
	for(auto& client : clients){
		cout << "client stop.." << endl;
		send_size += client->send_size();
		read_size += client->read_size();

		if(!client->Stop()){
			DEBUG_E("Fail to stop client ");
		}

		cout << "client delete.." << endl;
		delete client;
		client = NULL;
		cout << "client delete finish.." << endl;
	}

	DEBUG_I("all client send size ["<< send_size << "] receive size [" << read_size << "]");
	DEBUG_I("All client is Stop.");
}
//}}}1

// main {{{1
int main(int argc, char* argv[]){
	srand(time(NULL));

	string key("send_receive_packet_client");

	//pid_t pid = fork();
	int pid = argc > 1 ? 0 : 123;
	if(pid < 0){
		DEBUG_E("error");
	}
	else if(pid == 0){
		//child
		FrSleep(2000);

		frpublic::SingleLogServer::GetInstance()->InitLog("./log", 10 * 1024 * 1024);
		frpublic::SingleLogServer::GetInstance()->set_log_level(key, frpublic::eLogLevel_Info);
		frpublic::SingleLogServer::GetInstance()->set_default_log_key(key);
		frnet::set_log_config(key, frpublic::eLogLevel_Info);

		ClientProcess();
	}
	else{
		key = "send_receive_packet_server";

		frpublic::SingleLogServer::GetInstance()->InitLog("./log", 10 * 1024 * 1024);
		frpublic::SingleLogServer::GetInstance()->set_log_level(key, frpublic::eLogLevel_Info);
		frpublic::SingleLogServer::GetInstance()->set_default_log_key(key);
		frnet::set_log_config(key, frpublic::eLogLevel_Info);

		//father
		ServerProcess();
	}

	return 0;
}
//}}}1


