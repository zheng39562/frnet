## 文档性质
	* 记录socket的 使用 和 理论。
		* 网上的一些信息的整理和验证.
		* 一些自己的疑惑解答.
		* 部分man手册和其他来源的内容摘要(方便索引)
	* 文档上所有内容，必然在某个版本的linux系统下进行过测试(通常是centos7+)。

## 主体

### 使用.
	* client 使用步骤: socket() -> connect() -> recv()/write()/read()/send()

	* server 使用步骤: socket() -> bind() -> listen() -> accept() -> recv()/write()/read()/send()

	* socket 配置函数: int setsockopt( int socket, int level, int option_name, const void *option_value, size_t option_len);
		* 以下内容参考: http://blog.csdn.net/m0_37776493/article/details/78190587 
		* 如果要使用在socket上，level 需要指定为: SOL_SOCKET. 
		* option_name: 如果option_name 给出无法识别选项 则返回 ENOPROTOOPT
			* 开关型配置: option_value > 0 ? entable : disable;
				* SO_DEBUG: [调试信息]. sock->sk->sk_flag 的 SOCK_DBG.
				* SO_REUSEADDR: [地址复用功能]. sock->sk->sk_reuse = 1/0.
				* SO_DONTROUTE: [路由查找功能]. sock->sk->sk_flag 的 SOCK_LOCALROUTE.
				* SO_BROADCAST: [发送广播数据]. sock->sk->sk_flag 的 SOCK_BROADCAST.
				* SO_KEEPALIVE: [套接字保活/底层心跳]. sock->sk->sk_flag 的 SOCK_KEEPOPEN.
					* 要求是tcp协议, sock status is not listen and close.
				* SO_OOBINLINE: [紧急数据放入普通数据流]. sock->sk->sk_flag 的 SOCK_URGINLINE
				* SO_NO_CHECK: [校验和]. sock->sk->sk_no_check.
				* SO_PASSCRED: [SCM_CREDENTIALS控制消息的接收]. sock->sk->sk_flag 的 SOCK_PASSCRED
				* SO_TIMESTAMP: [数据报中的时间戳接收]. sock->sk->sk_flag 的 SOCK_RCVTSTAMP/SOCK_TIMESTAMP
			* 数据型配置: option_value 传入数据.
				* SO_SNDBUF/SO_RCVBUF: [设置发送/接收缓冲区的大小].
					* option_value : int
					* min ~ max : 2048 ~ 256 * (sizeof(struct sk_buff) + 256)
					* 实际是option_value * 2 (未求证)
				* SO_SNDTIMEO/SO_RCVTIMEO: [发送/接收超时时间]. sock->sk->sk_sndtimeo/sock->sk->sk_rcvtimeo
					* option_value 是一个结构体
						struct timeval {
							time_t tv_sec; // second
							time_t tv_usec; // us
						};
				* SO_LINGER: 开启, close or shutdown将等到所有套接字里排队的消息成功发送或到达延迟时间后>才会返回. 否则, 调用将立即返回
					* option_value 是一个结构体
						struct linger {
							int   l_onoff;    // sock->sk->sk_flag 的 SOCK_LINGER
							int   l_linger;   // sk->sk_lingertime
						};
				* SO_PRIORITY: [socket包的优先权]. sock->sk->sk_priority
					* 取值 [0, 6]

	* 一个进程可监听的socket数量设置.
		* 查看进程当前最大限制: ulimit -n .
			* ulimit -n number 可以进行设置，但仅对当前终端生效.
		* 查看用户级进程的软硬连接数限制: cat /etc/security/limits.conf
			* soft nofile : 软限制.
			* hard nofile : 硬限制.
			* 修改该限制，可对所有终端有效. 
		* 查看当前系统可打开的最大socket数: cat /proc/sys/fs/file-max
			* 主要是受限于硬件资源，不同机器的极限不同.
			* 可修改，但通常系统无法超越这个数量.

### 理论
	* 

### 测试结论.
	* 目标: 测试Listen socket 被关闭后, 尚处于连接的socket, 是否会被主动断开. 
		* 结论: 不会,centos7上测试，客户端client连接还正常.
		* 推论: 已经连接成功的socket，和原有的listen的socket已经没有任何关联了.
		* todo: 需要回头去看看协议是否有定义，如有是如何定义的.

### 一些网上记录，但未验证的可参考内容.
	* 

