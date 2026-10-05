# Stage 1: Build & Dependency Gathering
FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies yang dibutuhkan untuk kompilasi/setup
RUN apt-get update && apt-get install -y \
    build-essential \
    tgt \
    dnsmasq \
    iptables \
    && rm -rf /var/lib/apt/lists/*

# Stage 2: Final Runtime Image
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# Install runtime utilities
RUN apt-get update && apt-get install -y \
    dnsmasq \
    tgt \
    iptables \
    iproute2 \
    kmod \
    && rm -rf /var/lib/apt/lists/*

# Salin source code / binary diskless-server dari direktori lokal ke container
WORKDIR /app
COPY ./core /app/core
COPY ./cli /app/cli
COPY ./server /app/server

# Salin konfigurasi default (Pastikan folder config ini ada di repositori Anda)
COPY ./config/dnsmasq.conf /etc/dnsmasq.conf
COPY ./config/targets.conf /etc/tgt/targets.conf

# Script entrypoint untuk menjalankan daemon dnsmasq dan tgtd sekaligus
COPY ./deploy.sh /app/entrypoint.sh
RUN chmod +x /app/entrypoint.sh

ENTRYPOINT ["/app/entrypoint.sh"]

