# ==========================================
# STAGE 1: Build Environment (Kompilasi C++)
# ==========================================
FROM ubuntu:24.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

# Install dependensi compiler dan pengembangan Qt6 di Linux
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    qt6-base-private-dev \
    qt6-networkauth-dev \
    qt6-webengine-dev \
    libqt6sql6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
# Salin seluruh repositori untuk kompilasi lintas modul yang valid
COPY . .

# Lakukan proses build global lewat CMake
RUN rm -rf build && mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --target core-setup core-auth company-profile

# ==========================================
# STAGE 2: Runtime Environment (Web Server Apache)
# ==========================================
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

# Install Apache, modul CGI, dan runtime SQLite
RUN apt-get update && apt-get install -y \
    apache2 \
    libapache2-mod-fcgid \
    libqt6sql6-sqlite \
    sqlite3 \
    && rm -rf /var/lib/apt/lists/*

# Aktifkan modul eksekusi script CGI di Apache
RUN a2enmod cgi && a2enmod headers

# Konfigurasi hak akses folder runtime web
RUN mkdir -p /usr/lib/cgi-bin/azzammar/data && \
    chown -R www-data:www-data /usr/lib/cgi-bin/azzammar

# Salin berkas konfigurasi Apache VirtualHost dari repositori lokal
COPY ./src/apps/azzammar-core/apache/azzammar.conf /etc/apache2/sites-available/000-default.conf

# Salin berkas inisialisasi database (DLL/DML) & config.ini default
COPY ./src/apps/azzammar-core/database/init.sql /usr/lib/cgi-bin/azzammar/data/init.sql
COPY ./src/apps/azzammar-core/config.ini /usr/lib/cgi-bin/azzammar/config.ini

# Salin binary hasil kompilasi Qt dari Stage 1 langsung ke folder CGI-BIN Apache
COPY --from=builder /workspace/build/src/apps/core-setup/core-setup /usr/lib/cgi-bin/azzammar/core-setup
COPY --from=builder /workspace/build/src/apps/core-auth/core-auth /usr/lib/cgi-bin/azzammar/core-auth
COPY --from=builder /workspace/build/src/apps/company-profile/company-profile /usr/lib/cgi-bin/azzammar/company-profile

# Pastikan Apache memiliki izin eksekusi pada seluruh binary C++ CGI
RUN chown -R www-data:www-data /usr/lib/cgi-bin/azzammar && \
    chmod -R 755 /usr/lib/cgi-bin/azzammar && \
    chmod +x /usr/lib/cgi-bin/azzammar/core-setup && \
    chmod +x /usr/lib/cgi-bin/azzammar/core-auth && \
    chmod +x /usr/lib/cgi-bin/azzammar/company-profile

EXPOSE 80

CMD ["apachectl", "-D", "FOREGROUND"]

