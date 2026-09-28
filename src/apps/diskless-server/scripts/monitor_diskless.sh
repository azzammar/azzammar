#!/bin/bash

# Pastikan dijalankan sebagai root
if [ "$EUID" -ne 0 ]; then
  echo "[-] Harap jalankan menggunakan sudo!"
  exit 1
fi

CONFIG_FILE="/var/www/html/diskless/config.yaml"

sudo watch -n 2 -t "
echo '============================================================'
echo '        STATUS MODE OPERASIONAL KLIEN (CONFIG.YAML)         '
echo '============================================================'
if [ -f '$CONFIG_FILE' ]; then
    printf '%-25s %-18s %-15s\n' 'CLIENT ID' 'MAC ADDRESS' 'MODE STATUS'
    echo '------------------------------------------------------------'
    awk '
    BEGIN { id=\"\"; mac=\"\"; mode=\"read-only\" }
    /^[[:space:]]*- mac:/ {
        if (id != \"\") {
            if (mode == \"super-user\") printf \"%-25s %-18s \033[1;31mSUPER-USER\033[0m\n\", id, mac;
            else printf \"%-25s %-18s \033[1;32mREAD-ONLY\033[0m\n\", id, mac;
        }
        mac=\$0; sub(/.*mac:[[:space:]]*/, \"\", mac); gsub(/\"/, \"\", mac);
        id=\"\"; mode=\"read-only\";
    }
    /^[[:space:]]*id:/ { id=\$0; sub(/.*id:[[:space:]]*/, \"\", id); gsub(/\"/, \"\", id) }
    /^[[:space:]]*mode:/ { mode=\$0; sub(/.*mode:[[:space:]]*/, \"\", mode); gsub(/\"/, \"\", mode) }
    END {
        if (id != \"\") {
            if (mode == \"super-user\") printf \"%-25s %-18s \033[1;31mSUPER-USER\033[0m\n\", id, mac;
            else printf \"%-25s %-18s \033[1;32mREAD-ONLY\033[0m\n\", id, mac;
        }
    }
    ' '$CONFIG_FILE'
else
    echo '[-] File config.yaml tidak ditemukan.'
fi

echo ''
echo '========================================================================'
echo '       KONEKSI ISCSI ACTIVE & REAL-TIME SIZE LUN 1 MONITOR (TGT)        '
echo '========================================================================'
printf '%-10s %-45s %-18s\n' 'TARGET' 'IQN NAME' 'LUN 1 SIZE'
echo '------------------------------------------------------------------------'

tgt_data=\$(tgtadm --lld iscsi --op show --mode target 2>/dev/null)

if [ -n \"\$tgt_data\" ]; then
    echo \"\$tgt_data\" | grep '^Target ' | while read -r line; do
        tid=\$(echo \"\$line\" | awk '{print \$2}' | tr -d ':')
        iqn=\$(echo \"\$line\" | awk '{print \$3}')
        
        lun1_size=\$(echo \"\$tgt_data\" | awk -v id=\"\$tid\" '
            \$1 == \"Target\" && \$2 ~ \"^\"id\":\" { flag = 1; next }
            \$1 == \"Target\" && \$2 !~ \"^\"id\":\" { flag = 0 }
            flag == 1 && \$1 == \"LUN:\" && \$2 == \"1\" { lun_flag = 1; next }
            flag == 1 && lun_flag == 1 && \$1 == \"Size:\" { print \$2, \$3; exit }
            flag == 1 && \$1 == \"LUN:\" && \$2 != \"1\" { lun_flag = 0 }
        ')

        lun1_size=\$(echo \"\$lun1_size\" | tr -d ',')

        if [ -z \"\$lun1_size\" ]; then
            size_display=\"\033[1;30mTidak Ada LUN 1\033[0m\"
        else
            case \"\$lun1_size\" in
                0*) size_display=\"\033[5;1;31m⚠️ 0 GB (EROR!)\033[0m\" ;;
                *)  size_display=\"\033[1;36m\$lun1_size\033[0m\" ;;
            esac
        fi

        if [ \"\$tid\" = \"3\" ]; then
            printf 'Target %-3s \033[1;33m[STAGING MASTER]\033[0m %-28s %-18b\n' \"\$tid\" \"\$iqn\" \"\$size_display\"
        else
            printf 'Target %-3s %-45s %-18b\n' \"\$tid\" \"\$iqn\" \"\$size_display\"
        fi
        
        echo \"\$tgt_data\" | awk -v id=\"\$tid\" '
            \$1 == \"Target\" && \$2 ~ \"^\"id\":\" { flag = 1; next }
            \$1 == \"Target\" && \$2 !~ \"^\"id\":\" { flag = 0 }
            flag == 1 && (\$1 == \"Initiator:\" || \$1 == \"IP\") { print \"   └─ \" \$0 }
        '
    done
else
    echo 'Tidak ada sub-sistem iSCSI TGT yang berjalan.'
fi

echo ''
echo '========================================================================'
echo '             LOG TERBARU ANTRIAN BOOT REQUEST ISCSI CLIENT              '
echo '========================================================================'
tail -n 10 /var/log/nginx/access.log 2>/dev/null | grep 'boot.cgi' || echo 'Belum ada lalu lintas boot baru di log Nginx.'
"
