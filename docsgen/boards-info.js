'use strict';
/**
 * Universal parser for PlatformIO dependencies.
 * Formats URLs and registry strings into a consistent "Registry Style":
 * Name @ Version (provider:user)
 */
function smartFormat(dep) {
    if (!dep) return "";
    if (typeof dep !== 'string') return dep;

    const cleanDep = dep.trim();

    // Configuration for Git providers with specific regex for archives, releases, and git repos
    const providers = [
        {
            id: 'gh',
            name: 'github',
            // Captures: 1. Author, 2. Repo, 3. Version from path (releases), 4. Version from filename/branch
            regex: /github\.com\/([^/]+)\/([^/.]+)(?:\/(?:archive|releases\/download\/([^/]+)|tree)\/)?([^/]+)?(?:\.zip|\.git)?$/i
        },
        {
            id: 'gl',
            name: 'gitlab',
            // Captures: 1. Author, 2. Repo, 3. Version from path, 4. Version from filename
            regex: /gitlab\.com\/([^/]+)\/([^/.]+)(?:\/(?:-\/)?(?:archive|releases)\/([^/]+))?\/([^/]+)?(?:\.zip|\.git)?$/i
        },
        {
            id: 'bb',
            name: 'bitbucket',
            // Captures: 1. Author, 2. Repo, 3. Version from path, 4. Version from filename
            regex: /bitbucket\.org\/([^/]+)\/([^/.]+)(?:\/(?:get|downloads)\/([^/]+))?\/([^/]+)?(?:\.zip|\.git)?$/i
        }
    ];

    // 1. Try to match against Git providers (GitHub, GitLab, Bitbucket)
    for (const p of providers) {
        const match = cleanDep.match(p.regex);
        if (match) {
            let [_, author, repo, pathVer, fileVer] = match;

            // Prioritize version from path (typical in releases) over filename
            let version = pathVer || fileVer || "latest";

            // Clean up version string: remove extensions and 'v' prefix
            version = version
                .replace(/\.(zip|git|tar\.gz)$/i, '')
                .replace(/^v(\d)/i, '$1'); // Removes 'v' only if followed by a number

            // Avoid redundancy if the version string is identical to the repo name
            if (version.toLowerCase() === repo.toLowerCase()) {
                version = "latest";
            }

            return `${repo} @ ${version} (${p.id}:${author})`;
        }
    }

    // 2. Fallback for Standard PlatformIO Registry format (e.g., owner/lib @ ^1.0.0)
    if (cleanDep.includes('/') || cleanDep.includes('@')) {
        const parts = cleanDep.split('@');
        const fullName = parts[0].trim(); // Includes owner/name

        // Clean up version if present
        let version = "latest";
        if (parts[1]) {
            version = parts[1].trim().replace(/^[\^~=]/, '');
        }

        // Separate owner and library name for consistent formatting
        if (fullName.includes('/')) {
            const [owner, libName] = fullName.split('/');
            return `${libName.trim()} @ ${version} (pio:${owner.trim()})`;
        }

        return `${fullName} @ ${version}`;
    }

    // 3. Return original string if no patterns match
    return cleanDep;
}

function rowConfigFromPlatformIO() {
    const { execSync } = require('child_process');

    try {
        const jsonConfig = execSync('pio project config --json-output').toString();
        const config = JSON.parse(jsonConfig);
        return config;
    } catch (error) {
        console.error("Make sure PlatformIO Core is installed and in PATH");
        throw error;
    }
}

function cleanValue(v) {
    if (typeof v !== 'string') return v;
    return v
        .replace(/{/g, '')
        .replace(/}/g, '')
        .replace(/\$/g, '')
        .replace(/env:/g, '')
        .replace(/'/g, '')
        .replace(/-D/g, '');
}

function convertJsonToSections(jsonConfig) {
    const sections = {};
    jsonConfig.forEach(([sectionName, configArray]) => {
        sections[sectionName] = {};
        configArray.forEach(([key, value]) => {
            sections[sectionName][key] = value;
        });
    });
    return sections;
}

function cleanLibraries(raw) {
    if (!raw) return [];
    if (typeof raw === 'string') {
        raw = raw.split(',')
    }
    return raw.map((dep) => smartFormat(dep));
}

function extractModulesFromFlags(flags) {
    if (!flags) return [];
    let flagArray = [];
    if (Array.isArray(flags)) {
        flagArray = flags;
    } else if (typeof flags === 'string') {
        flagArray = flags.split(',').map(s => s.trim()).filter(s => s.length > 0);
    } else {
        return [];
    }
    const modules = [];
    flagArray.forEach((flag) => {
        // Match -DZmoduleName, allowing surrounding quotes
        const match = flag.match(/^['" ]*-DZ([^=]+)/);
        if (match) {
            const moduleName = match[1];
            // Additional constraint: must contain 'gateway', 'sensor', or 'actuator'
            if (moduleName.includes('gateway') || moduleName.includes('sensor') || moduleName.includes('actuator')) {
                modules.push(moduleName);
            }
        }
        //if MQTT_BROKER_MODE = true then modules.push("MQTT Broker Mode");
        const brokerMatch = flag.match(/^['" ]*-DMQTT_BROKER_MODE(?:=([^'"\s]+))?/);
        if (brokerMatch) {
            const value = brokerMatch[1];
            // Add only if not explicitly set to false (case insensitive)
            if (!value || value.toLowerCase() !== 'false') {
                modules.push("MQTT Broker Mode");
            }
        }

    });
    return modules;
}

function collectBoardsInformations(sections, { includeTests = false } = {}) {
    const rows = [];

    Object.entries(sections).forEach(([section, items]) => {
        if (!section.includes('env:')) return;
        if (!includeTests && section.includes('-test')) return;

        const env = section.replace('env:', '');
        let uc = '';
        let hardware = '';
        let description = '';
        let modules = [];
        let platform = '';
        let partitions = '';
        let libraries = [];
        let options = [];
        let customImg = '';

        Object.entries(items).forEach(([k, raw]) => {
            const v = cleanValue(raw);


            if (k === 'board') uc = v;
            if (k === 'platform') platform = smartFormat(v);
            if (k === 'board_build.partitions') partitions = v;
            if (k === 'custom_description') description = v;
            if (k === 'custom_hardware') hardware = v;
            if (k === 'custom_img') customImg = v;

            if (k === 'lib_deps') {
                libraries = cleanLibraries(raw);
            }

            if (k === 'build_flags') {
                options = v;
                modules = extractModulesFromFlags(v);
            }
        });

        rows.push({
            Environment: env,
            uC: uc,
            Hardware: hardware,
            Description: description,
            Modules: modules,
            Platform: platform,
            Partitions: partitions,
            Libraries: libraries,
            Options: options,
            CustomImg: customImg
        });
    });

    rows.sort((a, b) => a.Environment.localeCompare(b.Environment, 'en', { sensitivity: 'base' }));
    return rows;
}

function loadBoardsInfo(options = {}) {
    const { includeTests = false } = options;
    const config = rowConfigFromPlatformIO();
    const sections = convertJsonToSections(config);
    return collectBoardsInformations(sections, { includeTests });
}

function ensureDir(dir) {
    const fs = require('fs');
    if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });
}

module.exports = {
    loadBoardsInfo,
    ensureDir
};
