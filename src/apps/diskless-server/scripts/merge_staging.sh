#!/bin/bash

# Pastikan skrip dijalankan sebagai root
if [ "$EUID" -ne 0 ]; then
  echo "[-] Error: Harap jalankan skrip ini menggunakan sudo!"
  exit 1
fi

CONFIG_FILE="/var/www/html/diskless/config.yaml"

# Pastikan file konfigurasi ada
if [ ! -f "$CONFIG_FILE" ]; then
  echo "[-] Error: File konfigurasi tidak ditemukan di $CONFIG_FILE"
  exit 1
fi

echo "=== Memulai Proses Penggabungan Staging ke Master Image ==="

# 1. Ambil path dari config.yaml secara dinamis menggunakan sed/awk (jika yq tidak terinstall)
MASTER_IMG=$(sed -n 's/^[[:space:]]*master_image:[[:space:]]*"\(.*\)"/\1/p' "$CONFIG_FILE")
CACHE_DIR=$(sed -n 's/^[[:space:]]*write_cache_dir:[[:space:]]*"\(.*\)"/\1/p' "$CONFIG_FILE")

# Gunakan fallback jika parsing gagal
MASTER_IMG=${MASTER_IMG:-"/var/diskless/images/windows_base.img"}
CACHE_DIR=${CACHE_DIR:-"/mnt/write_cache"}
STAGING_FILE="${CACHE_DIR}/staging_update.qcow2"
STAGING_NBD="/dev/nbd1"
STAGING_TID=3

echo "[+] Master Image : $MASTER_IMG"
echo "[+] Staging File : $STAGING_FILE"

# 2. Validasi keberadaan file staging
if [ ! -f "$STAGING_FILE" ]; then
  echo "[-] Abort: File staging '$STAGING_FILE' tidak ditemukan."
  echo "[*] Info: Berarti belum ada aktivitas mode super-user atau proses merge sudah dilakukan sebelumnya."
  exit 1
fi

# 3. Putuskan sesi iSCSI target untuk diskless-master (TID 3) agar data tidak korup
echo "[*] Menghapus target iSCSI diskless-master (TID $STAGING_TID) dari memori kernel..."
tgtadm --lld iscsi --op delete --mode logicalunit --tid $STAGING_TID --lun 2 >/dev/null 2>&1
tgtadm --lld iscsi --op delete --mode logicalunit --tid $STAGING_TID --lun 1 >/dev/null 2>&1
tgtadm --lld iscsi --op delete --mode target --tid $STAGING_TID >/dev/null 2>&1

# 4. Lepaskan ikatan kernel NBD
echo "[*] Memutuskan koneksi Network Block Device ($STAGING_NBD)..."
guestunmount -f /mnt/vdisk_diskless_master >/dev/null 2>&1
umount -f -l /mnt/vdisk_diskless_master >/dev/null 2>&1
fuser -k "$STAGING_FILE" >/dev/null 2>&1
qemu-nbd --disconnect $STAGING_NBD >/dev/null 2>&1
sleep 2

# 5. Lakukan proses commit/merge
echo "[*] Sedang menggabungkan data perubahan ke Master Image (Mohon tunggu)..."
if qemu-img commit "$STAGING_FILE"; then
    echo "[+] Sukses: Perubahan berhasil digabungkan ke master image!"
    
    # 6. Pembersihan file staging
    echo "[*] Menghapus berkas staging temporer..."
    rm -f "$STAGING_FILE"
    
    echo "===================================================================="
    echo "[!] PENTING: Jangan lupa untuk menghapus atau mengubah baris"
    echo "    'mode: \"super-user\"' pada PC02 di config.yaml menjadi 'read-only'"
    echo "    sebelum klien dinyalakan kembali."
    echo "===================================================================="
else
    echo "[-] Gagal: Terjadi kesalahan saat menjalankan qemu-img commit!"
    exit 1
fi
