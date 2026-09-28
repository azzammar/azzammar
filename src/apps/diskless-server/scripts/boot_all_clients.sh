#!/bin/bash

if [ "$EUID" -ne 0 ]; then
  echo "[-] Harap jalankan menggunakan sudo!"
  exit 1
fi

CONFIG_FILE="/var/www/html/diskless/config.yaml"
BOOT_CLIENT_BIN="/var/www/html/diskless/boot_client/boot_client"

echo "====================================================================="
echo "   MEMULAI PRE-STAGE INITIALIZATION SELURUH KLIEN DISKLESS (YAML)   "
echo "====================================================================="

if ! lsmod | grep -q "^nbd"; then
    echo "[*] Mengaktifkan modul kernel nbd..."
    modprobe nbd max_part=8
fi

# Parser AWK baru yang jauh lebih kuat membaca per blok client (- mac)
awk '
BEGIN {
    id = ""
    nbd = ""
    prof = ""
    mode = "read-only"
}
# Setiap kali mendeteksi entri mac baru, cetak data dari client sebelumnya jika lengkap
/^[[:space:]]*- mac:/ {
    if (id != "" && nbd != "" && prof != "") {
        print id, nbd, prof, mode
    }
    # Reset variabel untuk client baru
    id = ""
    nbd = ""
    prof = ""
    mode = "read-only"
}
/^[[:space:]]*id:/ { id = $2; gsub(/"/, "", id) }
/^[[:space:]]*nbd:/ { nbd = $2; gsub(/"/, "", nbd) }
/^[[:space:]]*profile:/ { prof = $2; gsub(/"/, "", prof) }
/^[[:space:]]*mode:/ { mode = $2; gsub(/"/, "", mode) }
END {
    # Jangan lupa cetak client terakhir di ujung file
    if (id != "" && nbd != "" && prof != "") {
        print id, nbd, prof, mode
    }
}
' "$CONFIG_FILE" | while read -r client_id nbd_dev profile mode; do

    if [ -z "$client_id" ] || [ -z "$nbd_dev" ] || [ -z "$profile" ]; then
        continue
    fi

    echo "⚡ Menyiapkan Slot Klien: $client_id [$profile] di $nbd_dev (Mode: $mode)..."

    # Jalankan program C++ boot_client dengan 4 parameter utama
    sudo "$BOOT_CLIENT_BIN" "$client_id" "$nbd_dev" "$profile" "$mode"

    echo "[OK] Inisialisasi slot $client_id selesai."
    echo "---------------------------------------------------------------------"
done

echo "=== Selesai! Semua Slot Klien Telah Siap di Memori Server ==="
