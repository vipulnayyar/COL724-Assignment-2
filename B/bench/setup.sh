
mkdir -p server1 server2 server3 server4 lb client

cd server1
nohup ../../bin/server --config-file ../../conf/config.json --sched fcfs --file ../server_files --p 20 --server-id 1  > server1.log 2>&1  &

cd ../server2
nohup ../../bin/server --config-file ../../conf/config.json --sched fcfs --file ../server_files --p 20 --server-id 2 > server2.log 2>&1  &

cd ../server3
nohup ../../bin/server --config-file ../../conf/config.json --sched fcfs --file ../server_files --p 20 --server-id 3 > server3.log 2>&1  &

cd ../server4
nohup ../../bin/server --config-file ../../conf/config.json --sched fcfs --file ../server_files --p 20 --server-id 4 > server4.log 2>&1  &

cd ../lb
nohup ../../bin/lb --config-file ../../conf/config.json  > lb.log 2>&1 &


sleep 2
