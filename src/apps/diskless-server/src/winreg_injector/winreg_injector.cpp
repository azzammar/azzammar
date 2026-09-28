#include <iostream>
#include <string>
#include <hivex.h>

// Helper untuk mengubah DWORD (32-bit)
bool set_registry_dword(hive_h *h, hive_node_h node, const std::string& value_name, uint32_t new_value) {
    unsigned char value_bytes[4];
    value_bytes[0] = new_value & 0xFF;
    value_bytes[1] = (new_value >> 8) & 0xFF;
    value_bytes[2] = (new_value >> 16) & 0xFF;
    value_bytes[3] = (new_value >> 24) & 0xFF;

    hive_set_value val;
    val.key = const_cast<char*>(value_name.c_str());
    val.t = hive_t_REG_DWORD;
    val.len = 4;
    val.value = reinterpret_cast<char*>(value_bytes);

    return (hivex_node_set_value(h, node, &val, 0) != -1);
}

// Helper untuk menyuntikkan Group = "PNP_TDI" secara biner aman (REG_SZ Windows)
bool set_registry_group_pnp_tdi(hive_h *h, hive_node_h node) {
    unsigned char pnp_tdi_bytes[] = {
        'P', 0, 'N', 0, 'P', 0, '_', 0, 'T', 0, 'D', 0, 'I', 0, 0, 0
    };

    hive_set_value val;
    val.key = const_cast<char*>("Group");
    val.t = hive_t_REG_SZ;
    val.len = sizeof(pnp_tdi_bytes);
    val.value = reinterpret_cast<char*>(pnp_tdi_bytes);

    return (hivex_node_set_value(h, node, &val, 0) != -1);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Cara Penggunaan: " << argv[0] << " <path_ke_file_SYSTEM> <target_pc>" << std::endl;
        return 1;
    }

    std::string hive_path = argv[1];
    std::string target_pc = argv[2];

    hive_h *h = hivex_open(hive_path.c_str(), HIVEX_OPEN_WRITE);
    if (!h) {
        std::cerr << "[-] Gagal membuka Hive Registry!" << std::endl;
        return 1;
    }

    hive_node_h root = hivex_root(h);
    
    // 1. KOREKSI UTAMA: Hapus MountedDevices langsung dari Root Node (\MountedDevices)
    hive_node_h mounted_node = hivex_node_get_child(h, root, "MountedDevices");
    if (mounted_node) {
        std::cerr << "[+] Membersihkan cache GUID di \\MountedDevices..." << std::endl;
        hivex_node_delete_child(h, mounted_node);
    }

    hive_node_h control_set = hivex_node_get_child(h, root, "ControlSet001");
    if (!control_set) { hivex_close(h); return 1; }

    hive_node_h services_node = hivex_node_get_child(h, control_set, "Services");
    if (!services_node) { hivex_close(h); return 1; }

    // Ambil Objek Node Driver Jaringan
    hive_node_h MSiSCSI_node = hivex_node_get_child(h, services_node, "MSiSCSI");
    hive_node_h NDIS_node = hivex_node_get_child(h, services_node, "NDIS");
    hive_node_h z97_lan_node = hivex_node_get_child(h, services_node, "e1i65x64");   // Intel i218-V
    hive_node_h rtk_lan_node = hivex_node_get_child(h, services_node, "rt640x64");   // Realtek Gbe / 2.5G
    hive_node_h i226V_lan_node = hivex_node_get_child(h, services_node, "e2fexpress"); // Intel i226-V PCIe

    // Atur Default Awal: Nonaktifkan semua adapter kompetitor (Start = 4)
    if (z97_lan_node) set_registry_dword(h, z97_lan_node, "Start", 4);
    if (rtk_lan_node) set_registry_dword(h, rtk_lan_node, "Start", 4);
    if (i226V_lan_node) set_registry_dword(h, i226V_lan_node, "Start", 4);

    // Wajib Aktifkan Pemicu Boot iSCSI & NDIS Core (Start = 0)
    if (MSiSCSI_node) set_registry_dword(h, MSiSCSI_node, "Start", 0);
    if (NDIS_node) set_registry_dword(h, NDIS_node, "Start", 0);

    // Jalankan Logika Injeksi Berdasarkan Profil Target PC dari YAML
    if (target_pc == "i226V" && i226V_lan_node) {
        std::cerr << "[+] Mengaktifkan driver Intel i226-V PCIe (e2fexpress)..." << std::endl;
        set_registry_dword(h, i226V_lan_node, "Start", 0);
        set_registry_dword(h, i226V_lan_node, "ErrorControl", 1);
        set_registry_group_pnp_tdi(h, i226V_lan_node);
    }
    else if (target_pc == "z97" && z97_lan_node) {
        std::cerr << "[+] Mengaktifkan driver Intel i218-V Onboard (e1i65x64)..." << std::endl;
        set_registry_dword(h, z97_lan_node, "Start", 0);
    }
    else if ((target_pc == "z87" || target_pc == "rtk2500") && rtk_lan_node) {
        std::cerr << "[+] Mengaktifkan driver Realtek Gbe/2.5G (rt640x64)..." << std::endl;
        set_registry_dword(h, rtk_lan_node, "Start", 0);
    }
    else {
        std::cerr << "[-] Profil target PC tidak dikenal atau node driver hilang!" << std::endl;
    }

    // Tulis dan Commit seluruh modifikasi ke disk diskless
    if (hivex_commit(h, nullptr, 0) == -1) {
        std::cerr << "[-] Gagal melakukan commit perubahan ke hive berkas!" << std::endl;
        hivex_close(h);
        return 1;
    }

    hivex_close(h);
    std::cout << "[+] Sukses menginjeksi Registry via C++!" << std::endl;
    return 0;
}
