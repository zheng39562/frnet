include ../../make/config.mk

OPTIONAL_CFLAGS=-D__FRNET_EPOLL
EXAMPLE_LIBRARY=-lfrnet_epoll

all : max_socket 

MAX_SOCKET_OBJECTS=./$(OUTPUT_TEMPORARY_PATH)/./max_socket.o
max_socket : ${MAX_SOCKET_OBJECTS}
	${COMMON_CXX} ${COMMON_CFLAGS} ${OPTIONAL_CFLAGS} -o ./max_socket ${MAX_SOCKET_OBJECTS} ${COMMON_LIBRARY_PATH} ${COMMON_LIBRARY} ${EXAMPLE_LIBRARY}


OPEN_CLOSE_SOCKET_OBJECTS=./$(OUTPUT_TEMPORARY_PATH)/./open_close_socket.o
open_close_socket : ${OPEN_CLOSE_SOCKET_OBJECTS}
	${COMMON_CXX} ${COMMON_CFLAGS} ${OPTIONAL_CFLAGS} -o ./open_close_socket ${OPEN_CLOSE_SOCKET_OBJECTS} ${COMMON_LIBRARY_PATH} ${COMMON_LIBRARY} ${EXAMPLE_LIBRARY}


SEND_RECEIVE_PACKET_OBJECTS=./$(OUTPUT_TEMPORARY_PATH)/./send_receive_packet.o
send_receive_packet : ${SEND_RECEIVE_PACKET_OBJECTS}
	${COMMON_CXX} ${COMMON_CFLAGS} ${OPTIONAL_CFLAGS} -o ./send_receive_packet ${SEND_RECEIVE_PACKET_OBJECTS} ${COMMON_LIBRARY_PATH} ${COMMON_LIBRARY} ${EXAMPLE_LIBRARY}

clean :
	rm -f ./max_socket 
	rm -rf ${OUTPUT_TEMPORARY_PATH}/max_socket.o
	rm -f ./open_close_socket
	rm -rf ${OUTPUT_TEMPORARY_PATH}/open_close_socket.o
	rm -f ./send_receive_packet
	rm -rf ${OUTPUT_TEMPORARY_PATH}/send_receive_packet.o
	rm log/*

