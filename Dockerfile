FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

ENV TZ=Europe/Moscow

RUN apt-get update && apt-get install -y \
    qt5-default \
    qtbase5-dev \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/typeserver

COPY main.cpp mytcpserver.h mytcpserver.cpp 
mytcpserver.pro ./

RUN qmake mytcpserver.pro && make -j$(nproc) && mv GuessNumberServer server

EXPOSE 9000

CMD ["./server"]