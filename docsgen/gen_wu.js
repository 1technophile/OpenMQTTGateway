#!/usr/bin/env node

// Creates web installer manifests for ESP Web Tools firmware installation


const fs = require('fs');
const path = require('path');
const https = require('https');

const {
    mf_temp32,
    mf_temp32c3,
    mf_temp32s3,
    mf_temp8266,
    wu_temp_opt,
    wu_temp_p1,
    wu_temp_p2,
    wu_temp_p3,
    wu_temp_p4,
    wu_temp_end,
    manif_path,
    vue_path,
    cors_proxy,
    esp32_boot
} = require('./common_wu.js');

// Get site configuration and defaults
const defaultsPath = path.join(__dirname, '..', 'docs', '.vuepress', 'defaults.json');
const metaPath = path.join(__dirname, '..', 'docs', '.vuepress', 'meta.json');

let meta = require(defaultsPath);
try {
    const meta_overload = require(metaPath);
    meta = { ...meta, ...meta_overload };
} catch (e) {
    console.warn('meta.json not found or not valid. Using default configuration.');
}

// Parse command line arguments
const args = process.argv.slice(2);
let dev = args.includes('--dev') || meta.mode === 'dev';  //Just for feature parity with previous script
let repo = meta.repo || '1technophile/OpenMQTTGateway';

const firmware_src_directory = path.join('generated', 'artifacts', 'firmware_build');
const manifestFolder = dev ? `/dev/firmware_build/` : `/firmware_build/`;

function ensureFirmwareArtifacts() {
    if (!fs.existsSync(firmware_src_directory)) {
        throw new Error(`Missing firmware artifacts in ${firmware_src_directory}. Run "ci.sh build ..." first to populate this folder.`);
    }
}

// Replace version_tag in template and write to destination
function renderVersionTemplate(templatePath, outputPath, version) {
    if (!fs.existsSync(templatePath)) {
        throw new Error(`Template not found: ${templatePath}`);
    }
    const content = fs.readFileSync(templatePath, 'utf8').replace(/version_tag/g, version);
    fs.writeFileSync(outputPath, content);
}

/**
 * Download file from URL
 */
function downloadFile(url) {
    return new Promise((resolve, reject) => {
        https.get(url, (response) => {
            // Handle redirects
            if (response.statusCode === 302 || response.statusCode === 301) {
                return downloadFile(response.headers.location).then(resolve).catch(reject);
            }
            if (response.statusCode !== 200) {
                return reject(new Error(`Failed to download: ${response.statusCode}`));
            }
            const chunks = [];
            response.on('data', (chunk) => chunks.push(chunk));
            response.on('end', () => resolve(Buffer.concat(chunks)));
            response.on('error', reject);
        }).on('error', reject);
    });
}

/**
 * Fetch JSON from URL
 */
function fetchJson(url) {
    return new Promise((resolve, reject) => {
        https.get(url, { headers: { 'User-Agent': 'OpenMQTTGateway-Script' } }, (response) => {
            if (response.statusCode !== 200) {
                return reject(new Error(`Failed to fetch: ${response.statusCode}`));
            }
            let data = '';
            response.on('data', (chunk) => data += chunk);
            response.on('end', () => {
                try {
                    resolve(JSON.parse(data));
                } catch (e) {
                    reject(e);
                }
            });
            response.on('error', reject);
        }).on('error', reject);
    });
}

/**
 * Download and save asset
 */
async function downloadAsset(asset, destPath) {
    const buffer = await downloadFile(asset.browser_download_url);
    const filename = asset.browser_download_url.split('/').pop();
    fs.writeFileSync(path.join(destPath, filename), buffer);
    console.log('Downloaded: ' + filename);
}

/**
 * Create manifest and Vue option for a firmware
 * Partition path uses filename only (matches Python; split is redundant but harmless)
 */
function createManifest(name, templateFn) {
    const fw = name.split('-firmware')[0];
    const man_file = fw + '.manifest.json';
    const fwp_name = fw + '-partitions.bin';
    const fwb_name = fw + '-bootloader.bin';

    // Use filename to mirror Python behavior (no directories present today)
    const partPath = fwp_name.split('/').pop();

    const mani_str = templateFn({
        cp: cors_proxy,
        part: manifestFolder + partPath,
        bin: manifestFolder + name,
        bl: manifestFolder + fwb_name,
        boot: manifestFolder + esp32_boot.split('/').pop()
    });

    fs.writeFileSync(path.join(manif_path, man_file), mani_str);
    console.log('Created: ' + man_file);

    return wu_temp_opt({
        mff: manifestFolder + man_file,
        mfn: fw
    });
}

/**
 * Create manifest for ESP8266
 * Python adds manif_folder when writing to file, not in return
 */
function createManifest8266(name) {
    const fw = name.split('-firmware')[0];
    const man_file = fw + '.manifest.json';

    const mani_str = mf_temp8266({
        cp: cors_proxy,
        bin: manifestFolder + name
    });

    fs.writeFileSync(path.join(manif_path, man_file), mani_str);
    console.log('Created: ' + man_file);

    // Match Python: manif_folder + wu_temp_opt.substitute(...)
    return manifestFolder + wu_temp_opt({
        mff: manifestFolder + man_file,
        mfn: fw
    });
}

/**
 * Device type matchers
 */
const ESP32_NAMES = ['esp32', 'ttgo', 'heltec', 'thingpulse', 'theengs', 'lilygo', 'shelly', 'tinypico'];
const ESP8266_NAMES = ['nodemcu', 'sonoff', 'rf-wifi-gateway', 'manual-wifi-test', 'rfbridge'];

const deviceMatchers = {
    esp32: (name) => name.includes('firmware.bin') &&
        !name.includes('esp32c3') && !name.includes('esp32s3') &&
        ESP32_NAMES.some(key => name.includes(key)),

    esp32c3: (name) => name.includes('firmware.bin') && name.includes('esp32c3'),

    esp32s3: (name) => name.includes('firmware.bin') && name.includes('esp32s3'),

    esp8266: (name) => name.includes('firmware.bin') &&
        ESP8266_NAMES.some(key => name.includes(key))
};

/**
 * Setup dev environment
 */
async function setupDevEnvironment() {
    console.log('Generate Web Upload in dev mode');
    ensureFirmwareArtifacts();
    // Generate OTA latest version definition from template
    const tpl = path.join(__dirname, 'latest_version_dev.json.tpl');
    renderVersionTemplate(tpl, path.join(manif_path, 'latest_version_dev.json'), meta.version);

    // Copy the binaries from firmware_src_directory to manif_path
    const files = fs.readdirSync(firmware_src_directory);
    for (const name of files) {
        if (name.includes('.bin')) {
            fs.copyFileSync(
                path.join(firmware_src_directory, name),
                path.join(manif_path, name)
            );
        }
    }
}

/**
 * Setup release environment
 */
async function setupReleaseEnvironment() {
    console.log('Generate Web Upload in release mode');

    // Generate OTA latest version definition from template
    const tpl = path.join(__dirname, 'latest_version.json.tpl');
    renderVersionTemplate(tpl, path.join(manif_path, 'latest_version.json'), meta.version);

    const releaseUrl = `https://api.github.com/repos/${repo}/releases/latest`;
    const rel_data = await fetchJson(releaseUrl);

    if (!rel_data.assets) {
        console.log('Assets not found');
        process.exit(1);
    }

    // Download all assets
    for (const asset of rel_data.assets) {
        const name = asset.name;
        if (name.includes('firmware.bin') ||
            name.includes('partitions.bin') ||
            name.includes('bootloader.bin')) {
            await downloadAsset(asset, manif_path);
        }
    }
}

/**
 * Process firmware files and generate manifests
 */
function processFirmwareFiles(files) {
    let wu_file = wu_temp_p1;

    // Process ESP32 devices (bootloader @ 0x1000)
    for (const name of files) {
        if (deviceMatchers.esp32(name)) {
            wu_file += createManifest(name, mf_temp32);
        }
    }

    wu_file += wu_temp_p2;

    // Process ESP32-C3 devices (bootloader @ 0x0)
    for (const name of files) {
        if (deviceMatchers.esp32c3(name)) {
            wu_file += createManifest(name, mf_temp32c3);
        }
    }

    wu_file += wu_temp_p3;

    // Process ESP32-S3 devices (bootloader @ 0x0)
    for (const name of files) {
        if (deviceMatchers.esp32s3(name)) {
            wu_file += createManifest(name, mf_temp32s3);
        }
    }

    wu_file += wu_temp_p4;

    // Process ESP8266 devices
    for (const name of files) {
        if (deviceMatchers.esp8266(name)) {
            wu_file += createManifest8266(name);
        }
    }

    wu_file += wu_temp_end;

    return wu_file;
}

/**
 * Main execution function
 */
async function main() {
    // Create directories
    if (!fs.existsSync(manif_path)) {
        fs.mkdirSync(manif_path, { recursive: true });
    }
    if (!fs.existsSync(vue_path)) {
        fs.mkdirSync(vue_path, { recursive: true });
    }

    // Setup environment (dev or release)
    try {
        if (dev) {
            await setupDevEnvironment();
        } else {
            await setupReleaseEnvironment();
        }
    } catch (error) {
        console.error('Error setting up environment:', error.message);
        process.exit(1);
    }

    // Download boot binary
    const boot_bin = await downloadFile(esp32_boot);
    const boot_filename = esp32_boot.split('/').pop();
    fs.writeFileSync(path.join(manif_path, boot_filename), boot_bin);

    // Process all firmware files and generate Vue component
    const files = fs.readdirSync(manif_path).sort();
    const wu_content = processFirmwareFiles(files);

    fs.writeFileSync(path.join(vue_path, 'web-uploader.vue'), wu_content);
}

// Run main function
main().catch(error => {
    console.error('Error:', error);
    process.exit(1);
});
