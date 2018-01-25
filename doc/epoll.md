## 文档性质
	* 记录epoll的 使用 和 理论。
		* 网上的一些信息的整理和验证.
		* 一些自己的疑惑解答.
		* 部分man手册和其他来源的内容摘要(方便索引)
	* 文档上所有内容，必然在某个版本的linux系统下进行过测试(通常是centos7+)。

## 主体

### 使用.
	* 使用顺序: epoll_create() -> epoll_cntl() ->epoll_wait();
		* 详情可见: frnet_epoll.cpp
	* 几个注意点和未确认的问题:
		* epoll_create最大值无法确认.但测试过万级别是不受限的.
		* epoll_cntl ET模式的收发，需要全部读取完才可继续触发，或者可以重新进行epoll_cntl调用进行重新触发.

### 理论.
	* 

### 测试结论.
	* 目标: epoll_cntl 监听 不存在or已经关闭的 socket. 的返回值和errno
		* 结论: 返回-1 且errno is 9. --> Bad file number

### 一些网上记录，但未验证的可参考内容.
	* 

