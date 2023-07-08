FROM gcc:latest

COPY . /usr/src/projects/Server

WORKDIR /usr/src/projects/Server

RUN gcc -o server main.cpp

CMD ["./server"]