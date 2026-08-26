FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    flex \
    bison \
    valgrind \
    graphviz \
    gdb \
    python3 \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["bash"]
