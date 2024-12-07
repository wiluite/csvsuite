# Quick Help is right underneath.

FROM ubuntu:22.04
LABEL Description="Build environment"

ENV HOME=/root
ENV SOCI_DB_SQLITE3="sqlite3://db=sample.sqlite"
ENV SOCI_DB_MYSQL="mysql://db=testdb user=root"
ENV SOCI_DB_POSTGRESQL="postgresql://dbname=tester user=root"
ENV SOCI_DB_FIREBIRD="firebird://service=/dbname.fdb user=SYSDBA password=masterkey"

SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get -qy --no-install-recommends install \
    build-essential \
    cmake \
    gdb \
    wget \
    python3 \
    clang-15 \
    libc++-15-dev \
    libc++abi-15-dev \
    pip \
    libmysql++-dev \
    libpq-dev \
    curl

# Install Python's csvkit to be able to compare everything.
RUN pip3 install csvkit==1.5.0

# Deal with the MySQL server.
RUN apt-get install -y mysql-server && sleep 1
RUN service mysql start && mysql -u root -e "CREATE DATABASE testdb" && service mysql stop

# Deal with the PostgreSQL server.
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get install -qy postgresql
RUN apt-get install sudo
RUN service postgresql start && sudo -u postgres createuser -s root; createdb tester && service postgresql stop

# Some filesystem fix.
RUN mkdir -p /nonexistent

# Deal with the Firebird in total.
RUN apt-get -qy --no-install-recommends install libtomcrypt1 libtommath1 libtomcrypt-dev libtommath-dev firebird3.0-common firebird3.0-utils
RUN curl -L -o Firebird-5.0.0.1306-0-linux-x64.tar.gz -L https://github.com/FirebirdSQL/firebird/releases/download/v5.0.0/Firebird-5.0.0.1306-0-linux-x64.tar.gz
RUN tar -xzvf Firebird-5.0.0.1306-0-linux-x64.tar.gz && rm -f Firebird-5.0.0.1306-0-linux-x64.tar.gz
RUN cd Firebird-5.0.0.1306-0-linux-x64 && y masterkey | ./install.sh && \
    echo "CREATE DATABASE '/dbname.fdb' PAGE_SIZE 4096 USER 'sysdba' PASSWORD 'masterkey' ;" | isql-fb -q

# Finalize.
ENTRYPOINT service postgresql start && service mysql start && service firebird start && bash

# QUICK HELP

# Go to the CSV_co directory, then:
# docker build -t foo/CSV_co_ubuntu_22_04 -f Dockerfile .
# docker run -it --rm --name=CSV_co_ubuntu_22_04 --mount type=bind,source=${PWD},target=/src  foo/CSV_co_ubuntu_22_04

# Inside the docker container, build and test:
# cd src && mkdir build && cd build && cmake .. && make -j 4 all
# cd suite/test && ctest -j 8
