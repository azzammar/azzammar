#!/bin/bash
# Azzammar Automated On-Site Installer Script

set -e

echo "Starting Azzammar On-Site Deployment Suite..."

# 1. Install System Dependencies
sudo apt update
sudo apt install -y nginx qt6-base-dev libqt6networkauth6 cmake build-essential

# 2. Setup Production Directories
sudo mkdir -p /var/www/azzammar/plugins
sudo mkdir -p /etc/azzammar
sudo chown -R $USER:$USER /var/www/azzammar

# 3. Compile the Ecosystem Binaries Cleanly
echo "Compiling Azzammar Core Framework..."
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target azzammar_server azzammar_cli cashier_cli
cd ..

# 4. Copy Binaries to Production Targets
sudo cp build/server/azzammar_server /usr/local/bin/
sudo cp build/cli/azzammar_cli /usr/local/bin/

# 5. Configure Nginx Reverse Proxy Route Gateway
echo "Configuring Nginx Reverse Proxy Web Gateway..."
sudo tee /etc/nginx/sites-available/azzammar <<EOF
server {
    listen 80;
    server_name localhost;

    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
    }
}
EOF

sudo ln -sf /etc/nginx/sites-available/azzammar /etc/nginx/sites-enabled/
sudo rm -f /etc/nginx/sites-enabled/default
sudo systemctl restart nginx

# 6. Setup Daemon Automatic Service (systemd)
echo "Creating Azzammar Background Service Runner..."
sudo tee /etc/systemd/system/azzammar.service <<EOF
[Unit]
Description=Azzammar Ecosystem Server Engine Daemon
After=network.target

[Service]
Type=simple
User=$USER
ExecStart=/usr/local/bin/azzammar_server
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable azzammar.service
sudo systemctl start azzammar.service

echo "🎉 Azzammar System is deployed successfully on-site!"
echo "Check engine status with: systemctl status azzammar.service"

