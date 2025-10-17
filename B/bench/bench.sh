LB_SCHED_POLICY=$1

./cleanup.sh

./stop_servers.sh

./setup.sh ${LB_SCHED_POLICY}

./status.sh

./client_load.sh

./stop_servers.sh

