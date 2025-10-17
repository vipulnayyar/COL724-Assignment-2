
cd client

for iteration in {1..30}; do
    for client_id in {1..16}; do
        nohup ../../bin/client --req-id ${iteration}_${client_id} --op GET --name sample2.txt --config ../../conf/config.json > client${iteration}_${client_id}.log 2>&1 &
        echo "Iteration ${iteration} Client ${client_id} started"
    done

    sleep 1

done 


