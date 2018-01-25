include ../../make/config.mk

OPTIONAL_CFLAGS=-D__FRNET_EPOLL
EXAMPLE_LIBRARY=-lfrnet_epoll

all : send_receive_packet

# 测试最大的承载上限(40k,更大意义不大，只是改linux内核配置而已)
MAX_SOCKET_OBJECTS=./$(OUTPUT_TEMPORARY_PATH)/./max_socket.o
max_socket : ${MAX_SOCKET_OBJECTS}
	${COMMON_CXX} ${COMMON_CFLAGS} ${OPTIONAL_CFLAGS} -o ./max_socket ${MAX_SOCKET_OBJECTS} ${COMMON_LIBRARY_PATH} ${COMMON_LIBRARY} ${EXAMPLE_LIBRARY}


# 测试socket回收的情况.
FREE_SOCKET_OBJECTS=./$(OUTPUT_TEMPORARY_PATH)/./free_socket.o
free_socket : ${FREE_SOCKET_OBJECTS}
	${COMMON_CXX} ${COMMON_CFLAGS} ${OPTIONAL_CFLAGS} -o ./free_socket ${FREE_SOCKET_OBJECTS} ${COMMON_LIBRARY_PATH} ${COMMON_LIBRARY} ${EXAMPLE_LIBRARY}


#
OPEN_CLOSE_SOCKET_OBJECTS=./$(OUTPUT_TEMPORARY_PATH)/./open_close_socket.o
open_close_socket : ${OPEN_CLOSE_SOCKET_OBJECTS}
	${COMMON_CXX} ${COMMON_CFLAGS} ${OPTIONAL_CFLAGS} -o ./open_close_socket ${OPEN_CLOSE_SOCKET_OBJECTS} ${COMMON_LIBRARY_PATH} ${COMMON_LIBRARY} ${EXAMPLE_LIBRARY}


# send/read数据
SEND_RECEIVE_PACKET_OBJECTS=./$(OUTPUT_TEMPORARY_PATH)/./send_receive_packet.o
send_receive_packet : ${SEND_RECEIVE_PACKET_OBJECTS}
	rm log/*
	${COMMON_CXX} ${COMMON_CFLAGS} ${OPTIONAL_CFLAGS} -o ./send_receive_packet ${SEND_RECEIVE_PACKET_OBJECTS} ${COMMON_LIBRARY_PATH} ${COMMON_LIBRARY} ${EXAMPLE_LIBRARY}

clean :
	rm -f ./max_socket 
	rm -rf ${OUTPUT_TEMPORARY_PATH}/max_socket.o
	rm -f ./free_socket 
	rm -rf ${OUTPUT_TEMPORARY_PATH}/free_socket.o
	rm -f ./open_close_socket
	rm -rf ${OUTPUT_TEMPORARY_PATH}/open_close_socket.o
	rm -f ./send_receive_packet
	rm -rf ${OUTPUT_TEMPORARY_PATH}/send_receive_packet.o

