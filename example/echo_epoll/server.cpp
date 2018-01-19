/**********************************************************
 *  \file server.cpp
 *  \brief
 *  \note	注意事项： 
 * 
 * \version 
 * * \author zheng39562@163.com
**********************************************************/
#include "fr_public/pub_log.h"
#include "fr_public/pub_memory.h"
#include "fr_public/pub_tool.h"
#include "network/frnet_epoll.h"

#include <sys/types.h> 
#include <signal.h> 

using namespace std;
using namespace frnet;
using namespace fr_public;

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

// class EchoServer {{{1
class EchoServer : public NetServer_Epoll{
	public:
		EchoServer():NetServer_Epoll(){}
		EchoServer(size_t min_cache_size, size_t max_cache_size, int32_t _max_listen_num):NetServer_Epoll(min_cache_size, max_cache_size, _max_listen_num){ }
		virtual ~EchoServer()=default;
	protected:
		virtual bool OnReceive(Socket sockfd, const fr_public::BinaryMemory& binary, size_t& read_size){
			DEBUG_D("receive [" << string((const char*)binary.buffer(), binary.size()) << "]");
			BinaryMemoryPtr write_binary(new BinaryMemory(binary));
			if(Send(sockfd, write_binary) == eNetSendResult_Ok){;
				read_size = binary.size();
				return true;
			}
			else{
				DEBUG_E("Fail to send response.[" << string((const char*)binary.buffer(), binary.size()) << "]");
			}
			return false;
		}

		virtual void OnClose(){
			DEBUG_D("Stop Server.");
		}

		virtual void OnError(const NetError& error){
			DEBUG_D("Receive error [" << error.err_no << "]");
		}

		virtual bool OnDisconnect(Socket sockfd){
			DEBUG_D("Disconnect socket [" << sockfd << "]");
			return true;
		}
};
//}}}1

int main(int argc, char* argv[]){
	fr_public::SingleLogServer::GetInstance()->InitLog("./log", 10 * 1024);
	fr_public::SingleLogServer::GetInstance()->set_log_level("server", fr_public::eLogLevel_Program);
	fr_public::SingleLogServer::GetInstance()->set_default_log_key("server");

	EchoServer server;
	if(server.Start("127.0.0.1", 12345)){
		while(!IsAskedToQuit()){
			FrSleep(100);
		}
	}
	else{
		DEBUG_E("Fail to start server.");
	}

	cout << "close" << endl;
	return 0;
}

