FROM ubuntu:22.04

RUN apt update && \
    apt install -y fuse libfuse-dev gcc pkg-config && \
    mkdir /it24_host /mnt/antink /antink-logs

COPY antink.c /antink.c

RUN gcc -Wall -D_FILE_OFFSET_BITS=64 /antink.c $(pkg-config fuse --cflags --libs) -o /antink_fs

CMD ["/antink_fs", "-f", "/mnt/antink"]
