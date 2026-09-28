#!/bin/bash

# Pastikan dijalankan sebagai root
if [ "$EUID" -ne 0 ]; then
  echo "[-] Harap jalankan menggunakan sudo!"
  exit 1
fi

CONFIG_FILE="/var/www/html/diskless/config.yaml"
BOOT_ALL_BIN="/var/www/html/diskless/boot_all_clients.sh"
RESET_NODE_BIN="/var/www/html/diskless/reset_node.sh"

if [ ! -f "$RESET_NODE_BIN" ]; then
    echo "[-] Error: Skrip komponen $RESET_NODE_BIN tidak ditemukan!"
    exit 1
fi

echo "====================================================================="
echo "   MEMULAI RESET TOTAL INFRASTRUKTUR DISKLESS VIA NODE CONTROLLER    "
echo "====================================================================="

# 1. Ambil semua Client ID secara dinamis dari config.yaml
LIST_NODES=$(awk '/^[[:space:]]*id:/ { id=$2; gsub(/"/, "", id); print id }' "$CONFIG_FILE")

if [ -z "$LIST_NODES" ]; then
    echo "[-] Error: Tidak ada Node/Client ID yang ditemukan di config.yaml!"
    exit 1
fi

# 2. Perulangan pembersihan dengan proteksi untuk mode super-user
echo ""
echo "🔄 [PROSES] Menyegarkan Koneksi Slot Staging Master (TID 3)..."
echo "------------------------------------------------------------"
# Matikan paksa target staging lama untuk mencegah penguncian kernel
tgtadm --lld iscsi --op delete --mode logicalunit --tid 3 --lun 2 2>/dev/null
tgtadm --lld iscsi --op delete --mode logicalunit --tid 3 --lun 1 2>/dev/null
tgtadm --lld iscsi --op delete --mode target --tid 3 2>/dev/null
fuser -k /dev/nbd1 2>/dev/null
qemu-nbd --disconnect /dev/nbd1 2>/dev/null
sudo blockdev --flushbufs /dev/nbd1 2>/dev/null
# Kunci fuser pada berkas agar tidak corrupt saat modul nbd direstart
fuser -k "${OVERLAY_DIR}/staging_update.qcow2" 2>/dev/null
sleep 1

for node in $LIST_NODES; do
    # Cek mode operasional klien saat ini (read-only atau super-user)
    current_mode=$(awk -v id="$node" '/^[[:space:]]*id:/ {curr_id=$2; gsub(/"/, "", curr_id)} /^[[:space:]]*mode:/ {mode=$2; gsub(/"/, "", mode); if(curr_id==id) print mode}' "$CONFIG_FILE")
    [ -z "$current_mode" ] && current_mode="read-only"

    echo ""
    if [ "$current_mode" = "super-user" ]; then
        echo "⚠️  [WARNING] Node: $node terdeteksi dalam mode SUPER-USER!"
        echo "   -> Memproses refresh koneksi TGT tanpa merusak penulisan update master."
    else
        echo "🔄 [PROSES] Mengirim instruksi reset penuh untuk Node: $node (Mode: $current_mode)"
    fi
    echo "------------------------------------------------------------"
    
    retry=1
    max_retry=3
    sukses=false
    
    while [ $retry -le $max_retry ]; do
        # Panggil skrip reset_node.sh
        sudo "$RESET_NODE_BIN" "$node"
        
        # Ambil TID untuk verifikasi kernel TGT
        tid=$(awk -v target="$node" '
            BEGIN { id=""; tid="" }
            /^[[:space:]]*- mac:/ { if (id == target) { print tid; exit }; id=""; tid="" }
            /^[[:space:]]*id:/ { id=$2; gsub(/"/, "", id) }
            /^[[:space:]]*tid:/ { tid=$2; gsub(/"/, "", tid) }
            END { if (id == target) print tid }
        ' "$CONFIG_FILE")

        if [ -z "$tid" ] || ! tgtadm --lld iscsi --op show --mode target --tid "$tid" 2>/dev/null | grep -q "Target $tid:"; then
            echo "[✓] Node $node BERHASIL disegarkan pada percobaan ke-$retry."
            sukses=true
            break
        fi
        
        echo "[!] Node $node masih menggantung di kernel TGT, mencoba ulang ($retry/$max_retry)..."
        sleep 2
        retry=$((retry + 1))
    done
done

echo ""
echo "====================================================================="
echo "       MEMUAT ULANG MODUL KERNEL & SINKRONISASI AKHIR SISTEM        "
echo "====================================================================="

# 3. Bersihkan sisa global mount point
guestunmount -f /mnt/games_update_point 2>/dev/null
umount -f -l /mnt/games_update_point 2>/dev/null
guestunmount -f /mnt/vdisk_diskless_master 2>/dev/null
umount -f -l /mnt/vdisk_diskless_master 2>/dev/null

# 4. Refresh modul kernel NBD
echo "[*] Memuat ulang modul kernel NBD secara segar..."
modprobe -r nbd 2>/dev/null
sleep 2
modprobe nbd max_part=8
sudo udevadm settle
sleep 3

# 5. Sinkronisasi ulang target statis TGT utama bawaan server
echo "[*] Sinkronisasi ulang pemetaan target iSCSI TGT..."
sudo tgt-admin --update ALL --force
sudo tgt-admin --execute
sleep 2

echo "[✓] SEMUA PROSES RESET TOTAL SELESAI."
echo ""

# 6. Jalankan pemanasan / pra-inisialisasi semua slot klien kembali ke memori
if [ -f "$BOOT_ALL_BIN" ]; then
    echo "[*] Menunggu stabilitas sistem sebelum menyalakan seluruh klien..."
    sleep 2
    sudo "$BOOT_ALL_BIN"
fi
