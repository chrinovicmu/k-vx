FROM debian:bookworm 

ENV DEBIAN_FRONTEND=noninteractive


RUN apt-get update && \
    apt-get install -y build-essential linux-headers-$(uname -r) && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*



WORKDIR /src


COPY . /src


CMD ["make"]


