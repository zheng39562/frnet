/**********************************************************
 *  \file server.cpp
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

#include <sys/types.h> 
#include <signal.h> 

using namespace std;
using namespace frnet;
using namespace frpublic;

// Exit function list.

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

// class EchoServer 
class EchoServer : public NetListen{
	public:
		EchoServer():net_server_(CreateNetServer(this)){}
		virtual ~EchoServer(){ if(net_server_){ delete net_server_; net_server_ = NULL; } }
	public:
		inline bool Start(const std::string& ip, Port port){ return net_server_->Start(ip, port); }
		inline bool Stop(){ return net_server_->Stop(); }
	protected:
		virtual bool OnReceive(Socket sockfd, const frpublic::BinaryMemory& binary, size_t& read_size){
			DEBUG_D("receive [%s]", string((const char*)binary.buffer(), binary.size()).c_str());
			BinaryMemoryPtr write_binary(new BinaryMemory(binary));
			if(net_server_->Send(sockfd, write_binary) == eNetSendResult_Ok){;
				read_size = binary.size();
				Stop();
				return true;
			}
			else{
				DEBUG_E("Fail to send response.[%s]", string((const char*)binary.buffer(), binary.size()).c_str());
			}
			return false;
		}

		virtual void OnClose(){
			AskToQuit();
			DEBUG_D("Stop Server.");
		}

		virtual void OnError(const NetError& error){
			DEBUG_D("Receive error [%d]", error.err_no);
		}

		virtual void OnConnect(Socket sockfd){
			DEBUG_D("Connect socket [%d]", sockfd);
		}

		virtual void OnDisconnect(Socket sockfd){
			DEBUG_D("Disconnect socket [%d]", sockfd);
		}
	private:
		NetServer* net_server_;
};

int main(int argc, char* argv[]){
	frpublic::SingleLogServer::GetInstance()->InitLog("./log", 10 * 1024);
	frpublic::SingleLogServer::GetInstance()->set_log_level("server", frpublic::eLogLevel_Program);
	frpublic::SingleLogServer::GetInstance()->set_default_log_key("server");
	frnet::set_log_config("server", frpublic::eLogLevel_Program);

	EchoServer server;
	if(server.Start("127.0.0.1", 12345)){
		while(!IsAskedToQuit()){
			FrSleep(100);
		}
	}
	else{
		DEBUG_E("Fail to start server.");
	}

	DEBUG_D("close");
	return 0;
}

