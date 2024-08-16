#!/bin/bash

# Start Docker service
echo "Starting Docker Service..."
service docker start
echo ""

echo "Checking for docker image"
# Build Docker image if not exists
IMAGE=$( docker image ls | awk 'NR > 1' | cut -d ' ' -f 1 )

if echo $IMAGE | grep -q 'systemdesign'; then
    echo "Image already exists..."
else
    echo "Building Docker image..."
    docker build -t systemdesign -f Dockerfile .. --no-cache
fi

# Setup firewall rules
echo "Setting up firewall rules..."
iptables -A INPUT -i docker0 -j ACCEPT
iptables -A FORWARD -i docker0 -j ACCEPT

# Start tmux session
echo "Setting up TMUX..."
tmux new-session -d -s services 'bash'

# Split tmux window and run Docker containers
tmux split-window -v 'docker run --rm -it --network="host" --name store1 systemdesign /app/dkvstore -t store -p 6000'
tmux split-window -v 'docker run --rm -it --network="host" --name store2 systemdesign /app/dkvstore -t store -p 6001'
tmux split-window -v 'docker run --rm -it --network="host" --name store3 systemdesign /app/dkvstore -t store -p 6002'

# Wait
sleep 2

# Start coordinator container
tmux split-window -v 'docker run --rm -it --network="host" --name coordinator systemdesign /app/dkvstore -t coordinator -p 31337 -s "127.0.0.1:6000,127.0.0.1:6001,127.0.0.1:6002"'

# Start web servers
echo "Starting web servers..."
python3 -m http.server 5000 &
python3 -m http.server 5001 &
python3 -m http.server 5002 &

# Wait
sleep 2

# Start load balancer
echo "Starting load balancer..."
tmux split-window -h 'docker run --rm -it --network="host" --name loadbalancer systemdesign /app/loadbalancer 127.0.0.1:5000 127.0.0.1:5001 127.0.0.1:5002'

# Attach to tmux session
echo "Attaching to TMUX session..."
tmux attach-session -t services
