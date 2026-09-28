#include <iostream>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <yaml-cpp/yaml.h>

// Fungsi untuk parsing MAC dari QUERY_STRING (format: mac=xx-xx-xx-xx-xx-xx)
std::string get_mac_from_query(const std::string& query) {
    std::string key = "mac=";
    size_t pos = query.find(key);
    if (pos == std::string::npos) return "";

    size_t start = pos + key.length();
    size_t end = query.find('&', start);
    std::string mac = (end == std::string::npos) ? query.substr(start) : query.substr(start, end - start);

    // Ubah ke lowercase
    std::transform(mac.begin(), mac.end(), mac.begin(), ::tolower);
    return mac;
}

int main() {
    // 1. Ambil QUERY_STRING dari environment variable web server
    char* query_env = std::getenv("QUERY_STRING");
    std::string query_str = query_env ? std::string(query_env) : "";
    std::string mac = get_mac_from_query(query_str);

    // 2. Load konfigurasi YAML
    std::string config_path = "/var/www/html/diskless/config.yaml";
    YAML::Node config;
    try {
        config = YAML::LoadFile(config_path);
    } catch (const std::exception& e) {
        std::cout << "Content-Type: text/plain\r\n\r\n#\nError loading config\n";
        return 1;
    }

    // Koreksi pembacaan IP Server berdasarkan hierarki YAML Anda
    std::string server_ip = "192.168.1.10";
    if (config["server"] && config["server"]["ip"]) {
        server_ip = config["server"]["ip"].as<std::string>();
    }

    // Inisialisasi variabel dengan nilai default (fallback)
    std::string client_id = "diskless_master";
    std::string target_iqn = "iqn.2026-09.server:diskless-master";
    std::string nbd_dev = "/dev/nbd1";
    std::string target_pc = "z97";
    std::string client_mode = "read-only";

    bool found = false;

    // 3. Iterasi list "clients" untuk mencari kecocokan MAC Address
    if (!mac.empty() && config["clients"]) {
        for (size_t i = 0; i < config["clients"].size(); ++i) {
            std::string client_mac = config["clients"][i]["mac"].as<std::string>("");
            // Ubah mac dari YAML ke lowercase untuk kecocokan yang aman
            std::transform(client_mac.begin(), client_mac.end(), client_mac.begin(), ::tolower);

            if (client_mac == mac) {
                YAML::Node client = config["clients"][i];
                client_id = client["id"].as<std::string>(client_id);
                target_iqn = client["iqn"].as<std::string>(target_iqn);
                nbd_dev = client["nbd"].as<std::string>(nbd_dev);
                target_pc = client["profile"].as<std::string>(target_pc);
                
                if (client["mode"]) {
                    client_mode = client["mode"].as<std::string>("read-only");
                }
                
                found = true;
                break;
            }
        }
    }

    // 4. Eksekusi program backend via fork & exec jika data ditemukan / gunakan fallback default
    pid_t pid = fork();
    if (pid == 0) {
        std::freopen("/dev/null", "w", stdout);
        std::freopen("/dev/null", "w", stderr);

        // Panggil binary boot_client C++ baru Anda dengan parameter asli dari YAML
        // Di sisi boot_client.cpp, argumen input asli ini yang akan mendeteksi ulang mode "super-user"
        execl("/usr/bin/sudo", "sudo", "/var/www/html/diskless/boot_client/boot_client",
              client_id.c_str(), nbd_dev.c_str(), target_pc.c_str(), nullptr);

        _exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0); // Eksekusi secara sinkron
    }

    // 5. INTERVENSI LOGIKA TARGET IQN UNTUK RESPONS HTTP KE iPXE CLIENT
    if (client_mode == "super-user") {
        // Alihkan boot iPXE client langsung menuju iSCSI Target 3 (diskless-master)
        target_iqn = "iqn.2026-09.server:diskless-master";
    }

    // 6. Kirim respons HTTP bersih ke iPXE Client
    std::cout << "Content-Type: text/plain\r\n\r\n";
    std::cout << "#!ipxe\n";
    std::cout << "sanboot iscsi:" << server_ip << ":::1:" << target_iqn << "\n";

    return 0;
}
