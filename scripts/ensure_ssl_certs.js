#!/usr/bin/env node
'use strict';

const fs = require('fs');
const path = require('path');

function ensureSSLCerts() {
    const sslDir = path.join(process.cwd(), '.ssl');
    const keyFile = path.join(sslDir, 'key.pem');
    const certFile = path.join(sslDir, 'cert.pem');

    if (fs.existsSync(keyFile) && fs.existsSync(certFile)) {
        return;
    }

    const { execSync } = require('child_process');

    fs.mkdirSync(sslDir, { recursive: true });

    try {
        execSync(`openssl req -new -x509 -keyout "${keyFile}" -out "${certFile}" -days 365 -nodes -subj "/C=US/ST=State/L=City/O=OpenMQTTGateway/CN=localhost" 2>/dev/null`, {
            stdio: 'inherit'
        });
        console.log('✓ SSL certificate generated');
    } catch (err) {
        console.error('Failed to generate SSL certificate. Ensure openssl is installed.');
        process.exit(1);
    }
}

ensureSSLCerts();
