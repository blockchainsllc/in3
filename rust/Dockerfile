FROM rust:1.40

USER root
WORKDIR /root/

ENV CROSS_DOCKER_IN_DOCKER=true
RUN apt-get update && apt-get install -y --no-install-recommends docker.io

RUN wget https://github.com/rust-embedded/cross/archive/master.zip \
    && unzip master.zip &&  cd cross-master/ && cargo build \
    && cargo install --path . --force

# docker build -t cross-rust:1.0 .
# docker run --rm -it -v `pwd`:/root/in3-core/rust -v /var/run/docker.sock:/var/run/docker.sock cross-rust:1.0
