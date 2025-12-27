#!/usr/bin/env node
'use strict';

const fs = require('fs');
const path = require('path');

const INPUT_FILE = 'environments.ini';
const BOARD_DIR = path.join('docs', 'prerequisites', 'boards');
const BOARD_LIST_FILE = path.join('docs', 'prerequisites', 'board.md');
const WEB_INSTALL_FILE = path.join('docs', 'upload', 'web-install.md');
const VERBOSE = process.env.BOARD_DOCS_VERBOSE === '1';

function cleanValue(v) {
    // Normalize to string to avoid calling replace on undefined or non-string values
    const s = (typeof v === 'string') ? v : String(v ?? '');
    return s
        .replace(/{/g, '')
        .replace(/}/g, '')
        .replace(/\$/g, '')
        .replace(/env:/g, '')
        .replace(/'/g, '')
        .replace(/-D/g, '');
}

// Parse INI with continuation lines (indented lines are part of previous key)
function parseIniWithContinuations(content) {
    const sections = {};
    let currentSection = null;
    let lastKey = null;

    content.split(/\r?\n/).forEach((line) => {
        if (!line.trim() || line.trim().startsWith(';') || line.trim().startsWith('#')) return;

        const sectionMatch = line.match(/^\s*\[(.+?)\]\s*$/);
        if (sectionMatch) {
            currentSection = sectionMatch[1];
            sections[currentSection] = sections[currentSection] || {};
            lastKey = null;
            return;
        }

        if (!currentSection) return;

        if (/^\s+/.test(line) && lastKey) {
            const trimmed = line.trim();
            const existing = sections[currentSection][lastKey];
            sections[currentSection][lastKey] = existing ? `${existing}\n${trimmed}` : trimmed;
            return;
        }

        const kv = line.match(/^\s*([^=]+?)\s*=\s*(.*)$/);
        if (kv) {
            const key = kv[1].trim();
            const value = kv[2].trim();
            sections[currentSection][key] = value;
            lastKey = key;
        }
    });

    return sections;
}

function cleanLibraries(raw) {
    const normalized = cleanValue(raw);
    return normalized
        .replace(/\ncom-esp\.lib_deps\n/gi, '\n')
        .replace(/\ncom-esp32\.lib_deps\n/gi, '\n')
        .replace(/\ncom-arduino\.lib_deps\n/gi, '\n')
        .replace(/libraries\./g, '')
        .split(/\n+/)
        .map((s) => s.trim())
        .filter(Boolean)
        .join(', ');
}

function collectTable(sections) {
    const rows = [];

    Object.entries(sections).forEach(([section, items]) => {
        if (!section.includes('env:') || section.includes('-test')) return;

        const env = section.replace('env:', '');
        let uc = '';
        let hardware = '';
        let description = '';
        let modules = '';
        let platform = '';
        let partitions = '';
        let libraries = '';
        let options = '';

        Object.entries(items).forEach(([k, raw]) => {
            const v = cleanValue(raw);

            if (k === 'board') uc = v;
            if (k === 'platform') platform = v;
            if (k === 'board_build.partitions') partitions = v;
            if (k === 'custom_description') description = v;
            if (k === 'custom_hardware') hardware = v;

            if (k === 'lib_deps') {
                libraries = cleanLibraries(raw);
            }

            if (k === 'build_flags') {
                options = v.replace(/com-esp\.build_flags\n/gi, '').replace(/com-esp32\.build_flags\n/gi, '');
                v.split('\n').forEach((o) => {
                    const line = o.trim();
                    if (line.includes('gateway') || line.includes('sensor') || line.includes('actuator')) {
                        if (modules) modules += '\n';
                        const end = line.lastIndexOf('="');
                        modules += line.slice(1, end >= 0 ? end : undefined);
                    }
                });
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
            Options: options
        });
    });

    // Sort by Environment name (case insensitive)
    rows.sort((a, b) => a.Environment.localeCompare(b.Environment, 'en', { sensitivity: 'base' }));
    return rows;
}

function ensureDir(dir) {
    if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });
}

function writeWithMarker(targetPath, marker, tableMd) {
    const parsed = path.parse(targetPath);
    const targetWithSuffix = path.join(parsed.dir, `${parsed.name}-full${parsed.ext}`);

    let prefix = '';
    // Always rebuild from original target; ignore any existing -full content
    if (fs.existsSync(targetPath)) {
        const existing = fs.readFileSync(targetPath, 'utf8');
        const idx = existing.indexOf(marker);
        if (idx >= 0) {
            prefix = existing.slice(0, idx).trimEnd();
        } else {
            prefix = existing.trimEnd();
        }
    }

    const content = `${prefix}\n\n${marker}\n${tableMd}\n`;
    fs.writeFileSync(targetWithSuffix, content, 'utf8');
}

async function main() {
    // Load ESM module when script runs
    const { markdownTable } = await import('markdown-table');
    // Parse INI file with indentation-aware parsing to keep multiline values
    const iniContent = fs.readFileSync(INPUT_FILE, 'utf8');
    const sections = parseIniWithContinuations(iniContent);
    const rows = collectTable(sections);

    // Ensure directories exist
    ensureDir(BOARD_DIR);
    ensureDir(path.dirname(BOARD_LIST_FILE));
    ensureDir(path.dirname(WEB_INSTALL_FILE));

    // Produce individual files for each board
    rows.forEach((row) => {
        const filePath = path.join(BOARD_DIR, `${row.Environment}.md`);
        const data = [['Board index', 'Value']];
        Object.entries(row).forEach(([key, value]) => {
            // Replace newlines with HTML breaks for proper table rendering
            const cleanValue = String(value).replace(/\n/g, '<br>');
            data.push([key, cleanValue]);
        });
        const content = markdownTable(data, { align: ['l', 'l'] });
        fs.writeFileSync(filePath, content, 'utf8');
        if (VERBOSE) console.log(row);
    });

    // Produce list file
    // Add links to individual board files and replace newlines with commas
    const linkedRows = rows.map((row) => {
        const linkedEnv = `[${row.Environment}](../prerequisites/boards/${row.Environment}.md)`;
        return {
            Environment: linkedEnv,
            uC: row.uC.replace(/\n/g, ', '),
            Description: row.Description.replace(/\n/g, ', '),
            Libraries: row.Libraries.replace(/\n/g, ', ')
        };
    });

    if (VERBOSE) console.log(linkedRows);

    // Generate markdown table for summary files
    const tableData = [['', 'Environment', 'uC', 'Description', 'Libraries']];
    linkedRows.forEach((row, idx) => {
        tableData.push([
            idx.toString(),
            row.Environment,
            row.uC,
            row.Description,
            row.Libraries
        ]);
    });

    const tableMd = markdownTable(tableData, { align: ['r', 'l', 'l', 'l', 'l'] });

    // Write to board.md (preserve intro content, replace generated table)
    writeWithMarker(BOARD_LIST_FILE, '# Supported', tableMd);

    // Write to web-install.md (preserve intro content, replace generated table)
    writeWithMarker(WEB_INSTALL_FILE, '## Environments characteristics', tableMd);
}

main().catch((err) => {
    console.error(err);
    process.exit(1);
});
