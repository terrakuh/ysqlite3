FROM ubuntu:21.10

ENV DEBIAN_FRONTEND=noninteractive

RUN \
	apt-get update && apt-get install -y git git-lfs cmake python2 clang-format g++-11 gdb python3-pip && \
	pip install cmakelang
