#!/usr/bin/bash

while :
do
    inotifywait -e modify server.cfg
    ./restart_server
done