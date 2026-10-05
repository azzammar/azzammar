#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <algorithm>
#include <yaml-cpp/yaml.h>

// Fungsi pembantu untuk menjalankan perintah sistem secara diam-diam (> /dev/null 2>&1)
void run_command(const std::string& cmd) {
    std::string silent_cmd = cmd + " >/dev/null 2>&1";
    std::system(silent_cmd.c_str());
}

// Cek apakah modul kernel sudah aktif
bool is_modprobe_active(const std::string& mod_name) {
    std::string cmd = "lsmod | grep -q '^" + mod_name + "'";
    return (std::system(cmd.c_str()) == 0);
}

// Cek apakah file ada
bool file_exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

int main(int argc, char* argv[]) {
    // 1. Pastikan dijalankan sebagai root/sudo
    if (geteuid() != 0) {
        std::cerr << "[-] Harap jalankan menggunakan sudo!\n";
        return 1;
    }

    // 2. Tangkap parameter input atau gunakan default fallback jika kosong
    std::string client_id   = (argc > 1) ? argv[1] : "client_asus_z97c";
    std::string nbd_dev     = (argc > 2) ? argv[2] : "/dev/nbd1";
    std::string target_pc   = (argc > 3) ? argv[3] : "z97";
    std::string input_mode  = (argc > 4) ? argv[4] : ""; // Tangkap argumen ke-4 jika ada

    // 3. Load Konfigurasi Global dari YAML
    std::string config_path = "/var/www/html/diskless/config.yaml";
    YAML::Node config;
    try {
        config = YAML::LoadFile(config_path);
    } catch (const std::exception& e) {
        std::cerr << "[-] Gagal memuat file konfigurasi YAML: " << e.what() << "\n";
        return 1;
    }

    // Ambil data konfigurasi server
    std::string overlay_dir = config["server"]["write_cache_dir"].as<std::string>("/mnt/write_cache");
    std::string master_img  = config["server"]["master_image"].as<std::string>("/var/diskless/images/windows_base.img");
    std::string cpp_binary  = config["server"]["winreg_injector_path"].as<std::string>("/var/www/html/diskless/winreg_injector/winreg_injector");
    int lun = 1; 

    // 4. Deteksi Parameter 'mode' dan 'tid' dari YAML secara dinamis
    int tid = 4; 
    std::string client_mode = "read-only"; 

    if (config["clients"]) {
        for (size_t i = 0; i < config["clients"].size(); ++i) {
            if (config["clients"][i]["id"].as<std::string>("") == client_id) {
                tid = config["clients"][i]["tid"].as<int>(4);
                if (config["clients"][i]["mode"]) {
                    client_mode = config["clients"][i]["mode"].as<std::string>("read-only");
                }
                break;
            }
        }
    }

    // Jika argumen ke-4 via CLI diisi, prioritaskan nilai dari CLI tersebut
    if (!input_mode.empty()) {
        client_mode = input_mode;
    }

    // KUNCI UTAMA: Amankan nilai asli fisik PC di awal SEBELUM dimodifikasi oleh logika staging
    std::string original_id      = client_id;
    std::string original_nbd     = nbd_dev;
    int original_tid             = tid;
    std::string original_overlay = overlay_dir + "/" + original_id + ".qcow2";
    std::string original_mount   = "/mnt/vdisk_" + original_id;

    // 5. MEMULAI MODUL KERNEL NBD
    if (!is_modprobe_active("nbd")) {
        std::cout << "[*] Mengaktifkan modul kernel nbd...\n";
        run_command("modprobe nbd max_part=8");
    }

    // =====================================================================
    // A. JIKA MODE SUPER-USER: INISIALISASI JALUR STAGING MASTER (TID 3)
    // =====================================================================
    if (client_mode == "super-user") {
        std::cout << "[!] Peringatan: Klien masuk dalam MODE SUPER-USER (Staging Mode) [!]\n";
        
        std::string staging_nbd     = "/dev/nbd1";
        int staging_tid             = 3;
        std::string staging_id      = "diskless_master";
        std::string staging_overlay = overlay_dir + "/staging_update.qcow2";
        std::string staging_mount   = "/mnt/vdisk_" + staging_id;

        std::cout << "[*] Menyiapkan jalur STAGING MASTER (TID 3 / /dev/nbd1)...\n";

        // Bersihkan sesi Staging lama
        run_command("tgtadm --lld iscsi --op delete --mode logicalunit --tid " + std::to_string(staging_tid) + " --lun 2");
        run_command("tgtadm --lld iscsi --op delete --mode logicalunit --tid " + std::to_string(staging_tid) + " --lun 1");
        run_command("tgtadm --lld iscsi --op delete --mode target --tid " + std::to_string(staging_tid));
        run_command("guestunmount -f " + staging_mount);
        run_command("umount -f -l " + staging_mount);
        run_command("qemu-nbd --disconnect " + staging_nbd);
        sleep(0.5);

        if (file_exists(staging_overlay)) {
            run_command("fuser -k " + staging_overlay);
            std::remove(staging_overlay.c_str());
        }

        // Buat Staging Image & Sunting Driver Hardware PC Saat Ini
        run_command("qemu-img create -f qcow2 -F raw -b " + master_img + " " + staging_overlay);
        run_command("mkdir -p " + staging_mount);
        run_command("guestmount -a " + staging_overlay + " -m /dev/sda2 " + staging_mount);

        std::string staging_hive = staging_mount + "/Windows/System32/config/SYSTEM";
        if (file_exists(staging_hive)) {
            run_command(cpp_binary + " " + staging_hive + " " + target_pc);
        }
        run_command("sync");
        run_command("guestunmount " + staging_mount);
        sleep(0.5);

        // Ekspos ke Network Socket NBD1 untuk Staging Target 3
        run_command("qemu-nbd --connect=" + staging_nbd + " " + staging_overlay + " --cache=none --aio=native");
        sleep(1);
        run_command("blockdev --rereadpt " + staging_nbd);

        run_command("tgtadm --lld iscsi --op new --mode target --tid " + std::to_string(staging_tid) + " --targetname iqn.2026-09.server:diskless-master");
        run_command("tgtadm --lld iscsi --op new --mode logicalunit --tid " + std::to_string(staging_tid) + " --lun 1 --backing-store " + staging_nbd);
        run_command("tgtadm --lld iscsi --op new --mode logicalunit --tid " + std::to_string(staging_tid) + " --lun 2 --backing-store /mnt/games/games_master.img");
        run_command("tgtadm --lld iscsi --op bind --mode target --tid " + std::to_string(staging_tid) + " --initiator-address ALL");
        std::cout << "[✓] Jalur STAGING MASTER (TID 3) sukses diaktifkan.\n";
    }

    // =====================================================================
    // B. JALUR REGULAR: SELALU INIDIALISASI TARGET UTAMA BAWAAN KLIEN
    // =====================================================================
    std::cout << "[*] Menyiapkan target asli klien (" << original_id << " / TID " << original_tid << ") di " << original_nbd << "...\n";
    
    // Bersihkan sesi regular lama
    run_command("tgtadm --lld iscsi --op delete --mode logicalunit --tid " + std::to_string(original_tid) + " --lun 2");
    run_command("tgtadm --lld iscsi --op delete --mode logicalunit --tid " + std::to_string(original_tid) + " --lun 1");
    run_command("tgtadm --lld iscsi --op delete --mode target --tid " + std::to_string(original_tid));
    run_command("guestunmount -f " + original_mount);
    run_command("umount -f -l " + original_mount);
    run_command("qemu-nbd --disconnect " + original_nbd);
    sleep(0.5);

    if (file_exists(original_overlay)) {
        run_command("fuser -k " + original_overlay);
        std::remove(original_overlay.c_str());
    }

    // Buat Disk Regular & Inject Driver Registry Bawaan Klien
    run_command("qemu-img create -f qcow2 -F raw -b " + master_img + " " + original_overlay);
    run_command("mkdir -p " + original_mount);
    run_command("guestmount -a " + original_overlay + " -m /dev/sda2 " + original_mount);
    
    std::string original_hive = original_mount + "/Windows/System32/config/SYSTEM";
    if (file_exists(original_hive)) {
        run_command(cpp_binary + " " + original_hive + " " + target_pc);
    }
    run_command("sync");
    run_command("guestunmount " + original_mount);
    sleep(0.5);

    // Ekspos ke Network Socket NBD bawaan PC untuk Target Aslinya
    run_command("qemu-nbd --connect=" + original_nbd + " " + original_overlay + " --cache=none --aio=native");
    sleep(1);
    run_command("blockdev --rereadpt " + original_nbd);

    // Bangun nama IQN target
    std::string target_iqn = "iqn.2026-09.server:" + original_id;
    std::replace(target_iqn.begin(), target_iqn.end(), '_', '-');

    run_command("tgtadm --lld iscsi --op new --mode target --tid " + std::to_string(original_tid) + " --targetname " + target_iqn);
    run_command("tgtadm --lld iscsi --op new --mode logicalunit --tid " + std::to_string(original_tid) + " --lun 1 --backing-store " + original_nbd);
    run_command("tgtadm --lld iscsi --op new --mode logicalunit --tid " + std::to_string(original_tid) + " --lun 2 --backing-store /mnt/games/games_master.img");
    run_command("tgtadm --lld iscsi --op bind --mode target --tid " + std::to_string(original_tid) + " --initiator-address ALL");

    std::cout << "=== Sukses! Klien " << original_id << " [TID: " << original_tid << "] Siap Digunakan (Mode Konfigurasi: " << client_mode << ") ===\n";

    return 0;
}
