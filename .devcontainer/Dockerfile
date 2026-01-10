# .devcontainer/Dockerfile
# rootで運用する前提。C++(gcc/clang)、ACL、pahcer を入れる。
FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ARG ACL_TAG=v1.5.1

# 基本ツール + C++周り
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential gdb clang clang-format cmake ninja-build \
    git curl ca-certificates \
    python3 python3-pip time zip unzip wget jq \
    pkg-config libssl-dev \
    && rm -rf /var/lib/apt/lists/*


# AtCoder Library (C++) を /usr/local/include/atcoder に配置
RUN git clone --depth 1 -b ${ACL_TAG} https://github.com/atcoder/ac-library.git /tmp/acl \
    && cp -r /tmp/acl/atcoder /usr/local/include/ \
    && rm -rf /tmp/acl

# Rust を rustup で導入（root のホームに入る）
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# pahcer（提出用変換ツール想定）をインストール
RUN cargo install pahcer --locked

# （必要なら）ojツール等も
# RUN pip3 install -U pip && pip3 install online-judge-tools==11.7.0

# 作業ディレクトリ
WORKDIR /work
