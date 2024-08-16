#!/bin/bash

# Extract running container IDs
CONTAINERS=$(docker container ls -q)

# Extract PIDs of running python3 processes, excluding the grep command
SERVERS=$(ps aux | awk '/python3/ && !/grep/ {print $2}')

echo "Killing/stopping containers..."
echo "$CONTAINERS"

# Stop Docker containers
if [ -n "$CONTAINERS" ]; then
    SPLIT=$(echo "$CONTAINERS" | tr '\n' ' ')
    docker container stop $SPLIT
else
    echo "No Docker containers to stop."
fi

echo "Killing web servers..."
for PID in $SERVERS; do
    echo "Killing $PID"
    kill $PID
done
