FROM phusion/baseimage:0.9.18

CMD ["/sbin/my_init"]

WORKDIR /app

RUN apt-get -y update && apt-get install -y wget make g++ dh-autoreconf pkg-config

RUN wget https://github.com/premake/premake-core/releases/download/v5.0.0-alpha13/premake-5.0.0-alpha13-linux.tar.gz && \ 
    tar -zxvf premake-*.tar.gz && \
    rm premake-*.tar.gz && \
    mv premake5 /usr/local/bin

ADD reliable.io /app/reliable.io

RUN cd reliable.io && find . -exec touch {} \; && premake5 gmake && make -j32 test config=release_x64 && cp ./bin/* /app

EXPOSE 40000

ENTRYPOINT ./test

RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
