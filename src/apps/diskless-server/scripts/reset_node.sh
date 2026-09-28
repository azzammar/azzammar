#!/bin/bash

# 1. Pastikan dijalankan sebagai root
if [ "$EUID" -ne 0 ]; then
  echo "[-] Error: Harap jalankan menggunakan sudo!"
  exit 1
fi

CONFIG_FILE="/var/www/html/diskless/config.yaml"
BOOT_CLIENT_BIN="/var/www/html/diskless/boot_client/boot_client"

# 2. Validasi parameter input
if [ -z "$1" ]; then
  echo "[-] Error: Masukkan nomor urut PC atau ID Klien!"
  echo "Penggunaan: sudo $0 <nomor_pc_atau_id>"
  echo "Contoh 1  : sudo $0 2               (Untuk mereset z87k_pc02)"
  echo "Contoh 2  : sudo $0 client_asus_z87k_pc02"
  exit 1
fi

INPUT="$1"
TARGET_MATCH=""

# Mengubah input angka tunggal menjadi format pencarian ID jika user mengetik angka saja (misal: "2" menjadi "pc02")
if [[ "$INPUT" =~ ^[0-9]+$ ]]; then
    # Pad angka dengan nol jika di bawah 10 (misal: 2 menjadi 02)
    printf -v PC_NUM "%02d" "$INPUT"
    TARGET_MATCH="pc${PC_NUM}"
else
    TARGET_MATCH="$INPUT"
fi

echo "=== Memulai Reset Spesifik Node Terkunci ==="

# 3. Parsing data dari config.yaml berdasarkan kecocokan ID
# Menggunakan AWK untuk mencari data spesifik satu client
CLIENT_DATA=$(awk -v target="$TARGET_MATCH" '
BEGIN { id=""; mac=""; nbd=""; prof=""; tid="" }
/^[[:space:]]*- mac:/ {
    if (id ~ target) { print id, nbd, prof, tid; exit }
    id=""; mac=""; nbd=""; prof=""; tid=""
}
/^[[:space:]]*id:/ { id=$2; gsub(/"/, "", id) }
/^[[:space:]]*nbd:/ { nbd=$2; gsub(/"/, "", nbd) }
/^[[:space:]]*profile:/ { prof=$2; gsub(/"/, "", prof) }
/^[[:space:]]*tid:/ { tid=$2; gsub(/"/, "", tid) }
END { if (id ~ target) { print id, nbd, prof, tid } }
' "$CONFIG_FILE")

if [ -z "$CLIENT_DATA" ]; then
  echo "[-] Error: Klien dengan kata kunci '$INPUT' tidak ditemukan di config.yaml!"
  exit 1
fi

# Pecah data hasil parsing
read -r client_id nbd_dev profile tid <<< "$CLIENT_DATA"

echo "[+] Menerima Perintah Reset untuk:"
echo "    - Client ID : $client_id"
echo "    - NBD Device: $nbd_dev"
echo "    - Target TID: $tid"
echo "    - Profile   : $profile"
echo "------------------------------------------------------------"

# 4. PROSES PEMBONGKARAN PAKSA LEVEL KERNEL & TGT
echo "[*] Memutus paksa iSCSI TGT untuk LUN 1 & 2..."
tgtadm --lld iscsi --op delete --mode logicalunit --tid "$tid" --lun 2 2>/dev/null
tgtadm --lld iscsi --op delete --mode logicalunit --tid "$tid" --lun 1 2>/dev/null
tgtadm --lld iscsi --op delete --mode target --tid "$tid" 2>/dev/null

echo "[*] Melepaskan Virtual Mount Point (jika ada)..."
guestunmount -f "/mnt/vdisk_${client_id}" 2>/dev/null
umount -f -l "/mnt/vdisk_${client_id}" 2>/dev/null

echo "[*] Membunuh proses pengunci file cache..."
OVERLAY_DIR=$(grep -E '^[[:space:]]*write_cache_dir:' "$CONFIG_FILE" | awk -F'"' '{print $2}')
[ -z "$OVERLAY_DIR" ] && OVERLAY_DIR="/mnt/write_cache"
CLIENT_OVERLAY="${OVERLAY_DIR}/${client_id}.qcow2"

if [ -f "$CLIENT_OVERLAY" ]; then
    fuser -k "$CLIENT_OVERLAY" 2>/dev/null
fi

echo "[*] Memutus paksa Network Block Device ($nbd_dev)..."
fuser -k "$nbd_dev" 2>/dev/null
qemu-nbd --disconnect "$nbd_dev" 2>/dev/null

# Jika Anda memiliki binary dynamic-nbd, command ini akan dieksekusi juga
if command -v dynamic-nbd &> /dev/null; then
    sudo dynamic-nbd --disconnect "$nbd_dev" 2>/dev/null
fi
sleep 1

# 5. HAPUS FILE CACHE LAMA
if [ -f "$CLIENT_OVERLAY" ]; then
    echo "[*] Menghapus file cache .qcow2 lama..."
    rm -f "$CLIENT_OVERLAY"
fi

# 6. INVERSI KEMBALI MENGGUNAKAN BINER C++
echo "[*] Membangun ulang slot cache segar via biner C++..."
if [ -f "$BOOT_CLIENT_BIN" ]; then
    # Ambil status mode saat ini dari config.yaml untuk diteruskan ke C++
    current_mode=$(awk -v id="$client_id" '/^[[:space:]]*id:/ {curr_id=$2; gsub(/"/, "", curr_id)} /^[[:space:]]*mode:/ {mode=$2; gsub(/"/, "", mode); if(curr_id==id) print mode}' "$CONFIG_FILE")
    [ -z "$current_mode" ] && current_mode="read-only"
    
    sudo "$BOOT_CLIENT_BIN" "$client_id" "$nbd_dev" "$profile" "$current_mode"
else
    echo "[-] Error: Biner boot_client tidak ditemukan di $BOOT_CLIENT_BIN!"
    exit 1
fi

echo "------------------------------------------------------------"
echo "[✓] BERHASIL: Node $client_id telah disegarkan total dan siap boot!"
