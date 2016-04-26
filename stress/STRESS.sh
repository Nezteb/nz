#!/bin/bash

forking() {
	sleep 5
	open http://127.0.0.1:8089/
}

forking &
locust --host=$1

