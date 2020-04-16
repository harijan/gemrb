FROM ubuntu:20.04

RUN apt update && apt -y upgrade
ENV TZ=Australia/Melbourne
RUN echo $TZ > /etc/timezone && \
    apt-get update && apt-get install -y tzdata && \
    rm /etc/localtime && \
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && \
    dpkg-reconfigure -f noninteractive tzdata
RUN apt install -y build-essential cmake python-is-python3 git vim libpython3-dev libsdl2-dev libopenal-dev gdb
RUN apt clean