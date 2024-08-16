FROM ubuntu:22.04

RUN mkdir -p  /app
RUN apt-get -y update; apt-get install -y sudo; apt-get install -y libc6 libc6-dev build-essential net-tools iputils-ping iproute2 wget curl


WORKDIR /app

COPY  ../src/loadbalancer /app/loadbalancer
COPY  ../src/dkvstore /app/dkvstore
COPY  ../src/server.py /app/server.py
