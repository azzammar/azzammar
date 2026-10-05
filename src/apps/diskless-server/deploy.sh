#!/bin/bash

# Jalankan TGTD (iSCSI Target Daemon) di background
echo "Starting TGTD..."
tgtd

# Jalankan DNSMasq (DHCP/TFTP) di foreground agar container tetap hidup
echo "Starting DNSMasq..."
exec dnsmasq -k

