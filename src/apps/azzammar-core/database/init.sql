CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    email VARCHAR(100) UNIQUE NOT NULL,
    google_email VARCHAR(100) UNIQUE NOT NULL, 
    google_id VARCHAR(255) UNIQUE NOT NULL,
    google_refresh_token TEXT,
    private_key_bridge VARCHAR(255) NOT NULL,
    account_type VARCHAR(20) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS company_profiles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    profile_slug VARCHAR(50) UNIQUE NOT NULL, 
    company_name VARCHAR(100) NOT NULL,
    theme_config TEXT,
    is_default BOOLEAN DEFAULT 0, 
    FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
);

INSERT OR IGNORE INTO users (id, email, google_email, google_id, google_refresh_token, private_key_bridge, account_type)
VALUES (1, 'system@azzammar.com', 'system@gmail.com', 'google_system_id_123', 'offline_refresh_token_seed', 
'hash_root_key_azzammar_cloud', 'corporate');

INSERT OR IGNORE INTO company_profiles (user_id, profile_slug, company_name, theme_config, is_default)
VALUES (1, 'azzammar', 'Azzammar Official', '{"theme":"dark", "primary_color":"#007bff"}', 1);

