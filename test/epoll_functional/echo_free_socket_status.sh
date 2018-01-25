#! /bin/bash

PID=`ps -ef | grep free_socket | awk -F ' ' '{ if($8 == "./free_socket") print $2; }'`
for id in ${PID[@]};
do 
	ls /proc/${id}/fd -l | grep socket: | wc -l 
done

exit

