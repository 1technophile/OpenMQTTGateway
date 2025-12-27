#!/usr/bin/env node
'use strict';

// Minimal HTTPS static server for generated/site only.

const fs = require('fs');
const path = require('path');
const https = require('https');
const mime = require('mime-types');

const repoRoot = path.resolve(__dirname, '..');
const siteRoot = path.join(repoRoot, 'generated', 'site');
const port = parseInt(process.argv[2] || 8443, 10);
const keyFile = process.argv[3] || path.join(repoRoot, '.ssl', 'key.pem');
const certFile = process.argv[4] || path.join(repoRoot, '.ssl', 'cert.pem');

if (!fs.existsSync(siteRoot)) {
    console.error(`Error: folder not found: ${siteRoot}`);
    process.exit(1);
}

if (!fs.existsSync(keyFile) || !fs.existsSync(certFile)) {
    console.error(`Error: SSL certificates not found at ${keyFile} or ${certFile}`);
    console.error('Use scripts/ensure_ssl_certs.js to generate them.');
    process.exit(1);
}

const server = https.createServer({
    key: fs.readFileSync(keyFile),
    cert: fs.readFileSync(certFile)
}, (req, res) => {
    const reqPath = decodeURIComponent(req.url.split('?')[0]);
    const safePath = reqPath.endsWith('/') ? `${reqPath}index.html` : reqPath;
    const filePath = path.join(siteRoot, safePath);

    if (!filePath.startsWith(siteRoot)) {
        res.writeHead(403);
        return res.end('Forbidden');
    }

    fs.stat(filePath, (err, stats) => {
        if (err || !stats.isFile()) {
            res.writeHead(404);
            return res.end('Not Found');
        }
        const type = mime.contentType(path.extname(filePath)) || 'application/octet-stream';
        res.writeHead(200, { 'Content-Type': type });
        fs.createReadStream(filePath).pipe(res);
    });
});

server.listen(port, '0.0.0.0', () => {
    console.log(`Serving generated/site over HTTPS at https://localhost:${port}/`);
    console.log(`Serving generated/site over HTTPS at https://<host-ip>:${port}/`);
    console.log(`Note: remember that if you are testing DEV site you should go on https://<host-ip>:${port}/dev/`);
});

process.on('SIGINT', () => {
    server.close(() => process.exit());
});
