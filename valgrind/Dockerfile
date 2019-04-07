FROM phusion/baseimage:0.9.18

CMD ["/sbin/my_init"]

WORKDIR /app

RUN apt-get -y update && apt-get install -y wget make g++ dh-autoreconf pkg-config valgrind

RUN wget https://github.com/premake/premake-core/releases/download/v5.0.0-alpha13/premake-5.0.0-alpha13-linux.tar.gz && \ 
    tar -zxvf premake-*.tar.gz && \
    rm premake-*.tar.gz && \
    mv premake5 /usr/local/bin

ADD reliable.io /app/reliable.io

RUN cd reliable.io && find . -exec touch {} \; && premake5 gmake && make -j32 test && make -j32 soak && cp ./bin/* /app

CMD [ "valgrind", "--tool=memcheck", "--leak-check=yes", "--show-reachable=yes", "--num-callers=20", "--track-fds=yes", "--track-origins=yes", "./soak", "1000" ]

RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
