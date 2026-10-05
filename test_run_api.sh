#!/bin/bash
# Modul Core Setup:
echo "test core-setup"
curl http://192.168.1.22:8080/api/core-setup
echo "\n"
echo "test core-setup.. done"

# Modul Company Profile:
echo "test company-profile"
curl http://192.168.1.22:8080/api/company-profile
echo "\n"
echo "test company-profile.. done"

# Modul Core Authentication:
echo "test core-auth 1"
curl http://192.168.1.22:8080/api/core-auth
echo "\n"
echo "test core-auth 1.. done"

echo "test core-auth 2"
curl -X POST http://192.168.1.22:8080/api/core-auth \
-H "Content-Type: application/json" \
-d '{
  "email": "wongslam@azzammar.com",
  "google_email": "wongslam.dev@gmail.com",
  "google_id": "google_uid_10928301923",
  "google_refresh_token": "mock_refresh_token_xyz_123_secure",
  "bridge_key": "bridge_key_token_from_cloud",
  "account_type": "corporate"
}'
echo "\n"
echo "test core-auth 2.. done"

