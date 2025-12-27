#!/usr/bin/env node

// Creates web installer manifests for ESP Web Tools firmware installation

const fs = require('fs');
const path = require('path');
const https = require('https');
const { loadBoardsInfo, ensureDir } = require('./boards-info');

const {
    mf_temp32,
    mf_temp32c3,
    mf_temp32s3,
    mf_temp8266,
    cors_proxy,
    esp32_boot
} = require('./common_wu.js');

// ============================================================================
// Directory Configuration
// ============================================================================

// Base directories
const ROOT_DIR = path.join(__dirname, '..');
const DOCS_DIR = path.join(ROOT_DIR, 'docs');
const VUEPRESS_DIR = path.join(DOCS_DIR, '.vuepress');
const PUBLIC_DIR = path.join(VUEPRESS_DIR, 'public');
const COMPONENTS_DIR = path.join(VUEPRESS_DIR, 'components');
const ARTIFACTS_DIR = path.join(ROOT_DIR, 'generated', 'artifacts');

// Feature-specific directories
const FIRMWARE_SRC_DIR = path.join(ARTIFACTS_DIR, 'firmware_build');
const FIRMWARE_BUILD_DIR = path.join(PUBLIC_DIR, 'firmware_build');
const BOARDS_INFO_FILE = path.join(PUBLIC_DIR, 'boards-info.json');

// Configuration files
const DEFAULTS_CONFIG_PATH = path.join(VUEPRESS_DIR, 'defaults.json');
const META_CONFIG_PATH = path.join(VUEPRESS_DIR, 'meta.json');



let meta = require(DEFAULTS_CONFIG_PATH);
try {
    const meta_overload = require(META_CONFIG_PATH);
    meta = { ...meta, ...meta_overload };
} catch (e) {
    console.warn('meta.json not found or not valid. Using default configuration.');
}

// Parse command line arguments
const args = process.argv.slice(2);
const dev = args.includes('--dev') || meta.mode === 'dev';
const repo = meta.repo || '1technophile/OpenMQTTGateway';
const firmwareManifestFolder = dev ? `/dev/firmware_build/` : `/firmware_build/`;

// ============================================================================
// Utility Functions
// ============================================================================

function ensureFirmwareArtifacts() {
    if (!fs.existsSync(FIRMWARE_SRC_DIR)) {
        throw new Error(`Missing firmware artifacts in ${FIRMWARE_SRC_DIR}. Run "ci.sh build ..." first to populate this folder.`);
    }
    console.log(`Found firmware artifacts in: ${FIRMWARE_SRC_DIR}`);
}
// Replace version_tag in template and write to destination
function renderVersionTemplate(templatePath, outputPath, version) {
    if (!fs.existsSync(templatePath)) {
        throw new Error(`Template not found: ${templatePath}`);
    }
    const content = fs.readFileSync(templatePath, 'utf8').replace(/version_tag/g, version);
    fs.writeFileSync(outputPath, content);
    console.log(`Generated version file from template: ${outputPath}`);
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
    console.log(`Downloaded asset: ${filename} to ${destPath}`);
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
        part: firmwareManifestFolder + partPath,
        bin: firmwareManifestFolder + name,
        bl: firmwareManifestFolder + fwb_name,
        boot: firmwareManifestFolder + esp32_boot.split('/').pop()
    });

    const outPath = path.join(FIRMWARE_BUILD_DIR, man_file);
    fs.writeFileSync(outPath, mani_str);
    console.log(`Created manifest for ${fw}: ${outPath}`);
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
        bin: firmwareManifestFolder + name
    });

    const outPath = path.join(FIRMWARE_BUILD_DIR, man_file);
    fs.writeFileSync(outPath, mani_str);
    console.log(`Created manifest for ${fw} (ESP8266): ${outPath}`);

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
    console.log('DEV mode: preparing web upload files...');
    ensureFirmwareArtifacts();
    // Generate OTA latest version definition from template
    const tpl = path.join(__dirname, 'latest_version_dev.json.tpl');
    renderVersionTemplate(tpl, path.join(FIRMWARE_BUILD_DIR, 'latest_version_dev.json'), meta.version);

    // Copy the binaries from FIRMWARE_SRC_DIR to FIRMWARE_BUILD_DIR
    const files = fs.readdirSync(FIRMWARE_SRC_DIR);
    let copied = 0;
    for (const name of files) {
        if (name.includes('.bin')) {
            fs.copyFileSync(
                path.join(FIRMWARE_SRC_DIR, name),
                path.join(FIRMWARE_BUILD_DIR, name)
            );
            copied++;
            console.log(`Copied binary: ${name}`);
        }
    }
    console.log(`Copied ${copied} firmware binaries to ${FIRMWARE_BUILD_DIR}`);
}

/**
 * Setup release environment
 */
async function setupReleaseEnvironment() {
    console.log('RELEASE mode: downloading and preparing web upload files...');

    // Generate OTA latest version definition from template
    const tpl = path.join(__dirname, 'latest_version.json.tpl');
    renderVersionTemplate(tpl, path.join(FIRMWARE_BUILD_DIR, 'latest_version.json'), meta.version);

    const releaseUrl = `https://api.github.com/repos/${repo}/releases/latest`;
    console.log(`Fetching latest release info from: ${releaseUrl}`);
    const rel_data = await fetchJson(releaseUrl);

    if (!rel_data.assets) {
        console.error('No assets found in the latest release!');
        process.exit(1);
    }

    // Download all assets
    let downloaded = 0;
    for (const asset of rel_data.assets) {
        const name = asset.name;
        if (name.includes('firmware.bin') ||
            name.includes('partitions.bin') ||
            name.includes('bootloader.bin')) {
            await downloadAsset(asset, FIRMWARE_BUILD_DIR);
            downloaded++;
        }
    }
    console.log(`Downloaded ${downloaded} firmware assets to ${FIRMWARE_BUILD_DIR}`);
}

/**
 * Process firmware files and generate manifests
 */
function processFirmwareFiles(files) {
    let manifestCount = 0;
    for (const name of files) {
        if (deviceMatchers.esp32(name)) {
            createManifest(name, mf_temp32);
            manifestCount++;
        }
        if (deviceMatchers.esp32c3(name)) {
            createManifest(name, mf_temp32c3);
            manifestCount++;
        }
        if (deviceMatchers.esp32s3(name)) {
            createManifest(name, mf_temp32s3);
            manifestCount++;
        }
        if (deviceMatchers.esp8266(name)) {
            createManifest8266(name);
            manifestCount++;
        }
    }
    console.log(`Generated ${manifestCount} manifest files in ${FIRMWARE_BUILD_DIR}`);

}

/**
 * Main execution function
 */
// ===================== OpenMQTTGateway Web Uploader Manifest Generator =====================
// =====================                MAIN SCRIPT STARTS HERE                =====================
async function main() {
    console.log('================================================================================');
    console.log(' OpenMQTTGateway Web Uploader Manifest Generator - START');
    console.log('================================================================================');

    // === [1] Load and generate boards info ===
    console.log('\n[1/4] Generating boards-info.json ...');
    const boardsInfo = loadBoardsInfo({ verbose: 0 });
    const boardsJson = boardsInfo.map((row) => ({
        environment: row.Environment,
        hardware: row.Hardware,
        description: row.Description,
        microcontroller: row.uC,
        modules: row.Modules.filter(Boolean),
        platform: row.Platform,
        partitions: row.Partitions,
        libraries: row.Libraries.filter(Boolean),
        options: row.Options,
        customImg: row.CustomImg
    }));
    ensureDir(path.dirname(BOARDS_INFO_FILE));
    fs.writeFileSync(BOARDS_INFO_FILE, JSON.stringify(boardsJson, null, 2), 'utf8');
    console.log(`Generated boards-info.json with ${boardsJson.length} boards: ${BOARDS_INFO_FILE}`);

    // === [2] Ensure output directory ===
    console.log('\n[2/4] Ensuring output directory ...');
    ensureDir(FIRMWARE_BUILD_DIR);
    console.log(`Ensured output directory exists: ${FIRMWARE_BUILD_DIR}`);

    // === [3] Setup environment (dev or release) ===
    console.log('\n[3/4] Preparing firmware files ...');
    try {
        if (dev) {
            await setupDevEnvironment();
        } else {
            await setupReleaseEnvironment();
        }
    } catch (error) {
        console.error(`Error setting up environment: ${error.message}`);
        process.exit(1);
    }

    // === [4] Download boot binary and generate manifests ===
    console.log('\n[4/4] Downloading boot binary and generating manifests ...');
    console.log(`Downloading boot binary: ${esp32_boot}`);
    const boot_bin = await downloadFile(esp32_boot);
    const boot_filename = esp32_boot.split('/').pop();
    fs.writeFileSync(path.join(FIRMWARE_BUILD_DIR, boot_filename), boot_bin);
    console.log(`Saved boot binary as: ${boot_filename}`);

    const files = fs.readdirSync(FIRMWARE_BUILD_DIR).sort();
    console.log(`Processing firmware files in ${FIRMWARE_BUILD_DIR}...`);
    processFirmwareFiles(files);

    console.log('\n================================================================================');
    console.log(' OpenMQTTGateway Web Uploader Manifest Generator - END');
    console.log('================================================================================');
}
// =====================                MAIN SCRIPT ENDS HERE                  =====================

// Run main function
main().catch(error => {
    console.error('Fatal error:', error);
    process.exit(1);
});
