#!/bin/bash

forking() {
	sleep 5
	open http://127.0.0.1:8089/
}

rm -f *.log
forking &
locust -f locustfile.py --host=$1 --logfile=locust.log
