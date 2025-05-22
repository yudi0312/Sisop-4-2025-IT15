FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    gcc \
    fuse3 \
    libfuse3-dev \
    pkg-config \
    make \
    vim \
    && apt-get clean

RUN mkdir -p /mnt/antink && chmod 777 /mnt/antink

COPY antink.c /antink.c

RUN gcc -Wall -D_FILE_OFFSET_BITS=64 /antink.c $(pkg-config fuse3 --cflags --libs) -o /antink_fs

CMD ["/antink_fs", "-f", "/mnt/antink"]
