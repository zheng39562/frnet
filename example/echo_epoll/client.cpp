/**********************************************************
 *  \file client.cpp
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

//
class EchoClient : public NetListen{
	public:
		EchoClient():net_client_(CreateNetClient(this)){}
		virtual ~EchoClient(){ if(net_client_){ delete net_client_; net_client_ = NULL; } }
	public:
		inline bool Start(const std::string& ip, Port port){ return net_client_->Start(ip, port); }
		inline bool Stop(){ return net_client_->Stop(); }

		inline eNetSendResult Send(const frpublic::BinaryMemoryPtr& binary){ return net_client_->Send(binary); }
	protected:
		virtual bool OnReceive(Socket sockfd, const frpublic::BinaryMemory& binary, size_t& read_size){
			DEBUG_D("receive [%s]", string((const char*)binary.buffer(), binary.size()).c_str());
			read_size = binary.size();

			DEBUG_D("Receive res, Disconnect.");
			// close connection when return false;
			return false;
		}

		virtual void OnConnect(Socket sockfd){ 
			DEBUG_D("connect socket [%d].", sockfd);
		}

		virtual void OnDisconnect(Socket sockfd){ 
			DEBUG_D("disconnect socket [%d].", sockfd);
		}

		virtual void OnClose(){ 
			DEBUG_D("Stop Server."); 
			AskToQuit(); 
		}

		virtual void OnError(const NetError& error){
			DEBUG_D("Receive error [%d]", error.err_no);
		}
	private:
		NetClient* net_client_;
};

int main(int argc, char* argv[]){
	frpublic::SingleLogServer::GetInstance()->InitLog("./log", 10 * 1024);
	frpublic::SingleLogServer::GetInstance()->set_log_level("client", frpublic::eLogLevel_Program);
	frpublic::SingleLogServer::GetInstance()->set_default_log_key("client");
	frnet::set_log_config("client", frpublic::eLogLevel_Program);

	EchoClient client;
	if(client.Start("127.0.0.1", 12345)){
		string msg("Hello tcp.....");
		BinaryMemoryPtr binary(new BinaryMemory());
		binary->add(msg.c_str(), msg.size());
		if(client.Send(binary) != eNetSendResult_Ok){
			DEBUG_D("Fail to send [%s]", msg.c_str());
			return 1;
		}

		while(!IsAskedToQuit()){
			FrSleep(100);
		}
	}
	else{
		DEBUG_E("Fail to start client.");
	}

	return 0;
}

