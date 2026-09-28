#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <array>
#include <yaml-cpp/yaml.h>

void run_command(const std::string& cmd) {
    int retval = std::system(cmd.c_str());
}

bool file_exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// Fungsi untuk mengambil sisa kapasitas penyimpanan kosong dalam satuan KiloByte (KB)
long long get_available_space_kb(const std::string& path) {
    std::string cmd = "df --output=avail " + path + " | tail -n 1";
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return 0;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    try {
        return std::stoll(result);
    } catch (...) {
        return 0;
    }
}

int main(int argc, char* argv[]) {
    if (geteuid() != 0) {
        std::cerr << "[-] Harap jalankan menggunakan sudo!\n";
        return 1;
    }

    // 1. Load Konfigurasi Global dari YAML
    std::string config_path = "/var/www/html/diskless/config.yaml";
    YAML::Node config;
    try {
        config = YAML::LoadFile(config_path);
    } catch (const std::exception& e) {
        std::cerr << "[-] Gagal memuat file konfigurasi YAML\n";
        return 1;
    }

    std::string game_img = config["server"]["game_image"].as<std::string>("/mnt/games/games_master.img");
    std::string update_mount = config["server"]["game_mount"].as<std::string>("/mnt/games_update_point");

    if (argc < 2) {
        std::cout << "=== Utility Pengelola Game Master Diskless (Versi YAML) ===\n";
        std::cout << "Pilihan Argumen:\n";
        std::cout << "  " << argv[0] << " create [size_in_GB | full] -> Buat berkas image game kosong baru\n";
        std::cout << "  " << argv[0] << " mount                    -> Mount Image Game ke server untuk di-update\n";
        std::cout << "  " << argv[0] << " umount                   -> Unmount dan kunci kembali Image Game\n";
        return 0;
    }

    std::string action = argv[1];

    if (action == "create") {
        std::string size_param = (argc > 2) ? argv[2] : "500";
        
        if (file_exists(game_img)) {
            std::cerr << "[-] Batalkan: Berkas master game sudah ada di " << game_img << "\n";
            return 1;
        }

        std::string allocate_cmd = "";
        
        if (size_param == "full") {
            std::cout << "[*] Mendeteksi kapasitas sisa penyimpanan pada partisi NVMe...\n";
            long long avail_kb = get_available_space_kb("/mnt/games");
            if (avail_kb <= 0) {
                std::cerr << "[-] Gagal mendeteksi ruang kosong di /mnt/games!\n";
                return 1;
            }
            // Sisakan sekitar 5 GB untuk toleransi keamanan kernel filesystem agar tidak benar-benar 0%
            long long safe_kb = avail_kb - (5LL * 1024LL * 1024LL);
            std::cout << "[+] Ukuran maksimal aman yang ditemukan: " << (safe_kb / 1024 / 1024) << " GB\n";
            allocate_cmd = "fallocate -l " + std::to_string(safe_kb) + "K " + game_img;
        } else {
            allocate_cmd = "fallocate -l " + size_param + "G " + game_img;
        }

        std::cout << "[*] Alokasi blok berkas RAW Image di NVMe...\n";
        run_command(allocate_cmd);

        std::cout << "[*] Memformat file system ext4 pada image game...\n";
        run_command("mkfs.ext4 -F " + game_img);
        std::cout << "[✓] Selesai! Silakan lakukan mount untuk mengisi game.\n";
    }
    else if (action == "mount") {
        if (!file_exists(game_img)) {
            std::cerr << "[-] Error: Berkas " << game_img << " tidak ditemukan!\n";
            return 1;
        }
        run_command("mkdir -p " + update_mount);
        
        std::cout << "[*] Memetakan tabel partisi berkas gambar via guestmount...\n";
        run_command("sudo guestmount -a " + game_img + " -m /dev/sda2 --rw " + update_mount + " -o allow_other");
        sleep(1);

        std::cout << "[✓] Siap! Anda bisa menambah/meng-update game langsung dari folder " << update_mount << "\n";
    }
    else if (action == "umount") {
        std::cout << "[*] Menyinkronkan seluruh perubahan data ke NVMe...\n";
        run_command("sync");

        std::cout << "[*] Melepaskan mount point Game Master...\n";
        run_command("sudo guestunmount " + update_mount);
        sleep(1);

        std::cout << "[✓] Berkas master disk dikunci kembali dan aktif di jaringan klien secara aman.\n";
    }

    return 0;
}
