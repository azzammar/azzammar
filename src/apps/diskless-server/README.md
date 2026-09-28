# Azzammar Diskless Server Application Ecosystem

Aplikasi ini merupakan bagian ekosistem mandiri di dalam proyek **Azzammar** yang dirancang untuk mengelola, mengatur, mengoptimalkan, serta melakukan *monitoring* real-time infrastruktur Linux Diskless 
Server berbasis **iSCSI (TGT)**, **NBD (Network Block Device)**, **iPXE**, dan **DNSMasq**.

---

## 📂 Struktur Aplikasi & Layout Berkas

```text
diskless-server/
├── config.yaml.example     # Templat pemetaan Client, MAC, NBD, dan Target iSCSI
├── config/                 # Kumpulan berkas konfigurasi pendukung infrastruktur server
│   ├── dnsmasq/
│   │   └── pxe.conf        # Konfigurasi DHCP & TFTP Server (DNSMasq)
│   ├── pxe/
│   │   ├── autoexec.ipxe   # Skrip awal eksekusi boot iPXE ROM klien
│   │   └── boot.ipxe       # Logika pemilihan image boot target iPXE
│   └── tgt/
│       └── targets.conf    # Konfigurasi target iSCSI statis bawaan server
├── scripts/                # Komponen skrip Bash Pengontrol Utilitas & Monitoring
│   ├── boot.cgi            # Endpoint CGI penerima request boot MAC Address klien
│   ├── boot_all_clients.sh # Pra-inisialisasi massal seluruh slot memori NBD klien
│   ├── merge_staging.sh    # Penggabungan (commit) write-cache (.qcow2) ke Master disk
│   ├── monitor_diskless.sh # Dashboard live monitor kapasitas LUN 1 iSCSI TGT & NBD
│   ├── reset_diskless.sh   # Penghapusan aman I/O macet global & re-alokasi modul kernel
│   └── reset_node.sh       # Penanganan pembersihan I/O macet pada spesifik single node PC
└── src/                    # Kode Sumber Aplikasi Utama (C++)
    ├── boot_client/        # Modul handler alokasi image per mesin klien
    ├── boot_loader/        # Modul bootstrap sistem booting
    ├── game_updater/       # Modul penyinkron update virtual disk
    └── winreg_injector/    # Modul injeksi driver NIC internal Windows klien (Registry)
```

---

## 🛠️ Langkah Instalasi di Server Baru

Ikuti tahapan berikut untuk melakukan instalasi dan kompilasi ekosistem aplikasi diskless server ini:

### 1. Kloning Repositori & Masuk ke Folder Aplikasi
```bash
git clone https://github.com
cd azzammar/src/apps/diskless-server
```

### 2. Kompilasi Seluruh Kode Sumber C++
Masuk ke masing-masing sub-folder di dalam direktori `src/` dan jalankan skrip kompilasi lokal yang telah disediakan:

```bash
# Kompilasi Boot Loader
cd src/boot_loader/ && chmod +x compile.sh && ./compile.sh && cd ../..

# Kompilasi Boot Client
cd src/boot_client/ && chmod +x compile.sh && ./compile.sh && cd ../..

# Kompilasi Game Updater
cd src/game_updater/ && chmod +x compile.sh && ./compile.sh && cd ../..

# Kompilasi Windows Registry Injector
cd src/winreg_injector/ && chmod +x compile.sh && ./compile.sh && cd ../..
```
*Hasil kompilasi biner executable C++ di atas akan tercipta secara lokal pada masing-masing sub-folder dan aman dari pelacakan Git berkat proteksi berkas `.gitignore`.*

### 3. Deploy dan Pasang Skrip ke Direktori Kerja Web Server
Buat direktori kerja operasional server, salin seluruh berkas skrip, dan berikan izin eksekusi (`chmod +x`):
```bash
sudo mkdir -p /var/www/html/diskless

# Salin skrip utilitas
sudo cp scripts/* /var/www/html/diskless/

# Pindahkan biner boot_client hasil kompilasi ke folder panggilannya
sudo mkdir -p /var/www/html/diskless/boot_client
sudo cp src/boot_client/boot_client /var/www/html/diskless/boot_client/

# Berikan izin eksekusi penuh pada direktori kerja server
sudo chmod +x /var/www/html/diskless/*.sh
sudo chmod +x /var/www/html/diskless/*.cgi
sudo chmod +x /var/www/html/diskless/boot_client/boot_client
```

### 4. Setup Berkas Konfigurasi Utama (`config.yaml`)
Salin berkas templat bawaan proyek menjadi berkas konfigurasi aktif di server, lalu sesuaikan isinya (seperti MAC Address, alokasi `/dev/nbd*`, dan Target ID `tid:` iSCSI klien):
```bash
sudo cp config.yaml.example /var/www/html/diskless/config.yaml
sudo nano /var/www/html/diskless/config.yaml
```

---

## 🎛️ Sinkronisasi Layanan Pihak Ketiga (Infrastructure Integration)

Untuk menyelaraskan konfigurasi bawaan repositori ini dengan sistem operasi server, salin berkas dari folder `config/` ke sistem utama:

### 1. DHCP / TFTP Server (DNSMasq)
```bash
sudo cp config/dnsmasq/pxe.conf /etc/dnsmasq.d/
sudo systemctl restart dnsmasq
```

### 2. iSCSI Target Daemon (TGT)
```bash
sudo cp config/tgt/targets.conf /etc/tgt/
sudo systemctl restart tgt
```

### 3. Nginx / Web Server (iPXE Boot Script)
```bash
sudo mkdir -p /var/www/html/pxe
sudo cp config/pxe/*.ipxe /var/www/html/pxe/
sudo systemctl restart nginx
```

---

## 🖥️ Cara Penggunaan Skrip Kontrol Utama

* **Menjalankan Monitoring Dashboard Real-Time:**
  ```bash
  sudo /var/www/html/diskless/monitor_diskless.sh
  ```
  *Dashboard ini memantau kapasitas LUN 1, mendeteksi status Klien (`READ-ONLY` atau `SUPER-USER`), serta memberikan alarm teks merah berkedip `⚠️ 0 GB (EROR!)` secara real-time jika ada alokasi kernel yang 
tersangkut.*

* **Melakukan Reset Total / Pembersihan Massal Server:**
  ```bash
  sudo /var/www/html/diskless/reset_diskless.sh
  ```

* **Melakukan Reset / Refresh Paksa pada Mesin Tertentu:**
  ```bash
  sudo /var/www/html/diskless/reset_node.sh "client_asus_z97c"
  ```

