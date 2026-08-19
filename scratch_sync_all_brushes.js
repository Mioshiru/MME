const fs = require('fs');
const path = require('path');

const dataRoot = 'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data';
const distDataRoot = 'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/dist/data';
const src1310 = path.join(dataRoot, '1310');

function parseBrushes(xmlPath) {
  if (!fs.existsSync(xmlPath)) return [];
  const content = fs.readFileSync(xmlPath, 'utf8');
  const regex = /<brush\s+name="([^"]+)"([\s\S]*?)<\/brush>/g;
  const list = [];
  let m;
  while ((m = regex.exec(content)) !== null) {
    list.push({ name: m[1], xml: m[0] });
  }
  return list;
}

function parseBorders(xmlPath) {
  if (!fs.existsSync(xmlPath)) return [];
  const content = fs.readFileSync(xmlPath, 'utf8');
  const regex = /<border\s+id="([^"]+)"([\s\S]*?)<\/border>/g;
  const list = [];
  let m;
  while ((m = regex.exec(content)) !== null) {
    list.push({ id: m[1], xml: m[0] });
  }
  return list;
}

function syncFileBrushes(targetXmlPath, sourceBrushes, rootTag) {
  if (!fs.existsSync(targetXmlPath)) return;
  let targetContent = fs.readFileSync(targetXmlPath, 'utf8');
  const existingMatches = targetContent.matchAll(/<brush\s+name="([^"]+)"/g);
  const existingNames = new Set();
  for (const em of existingMatches) {
    existingNames.add(em[1]);
  }

  const toAdd = [];
  for (const sb of sourceBrushes) {
    if (!existingNames.has(sb.name)) {
      toAdd.push(sb.xml);
      existingNames.add(sb.name);
    }
  }

  if (toAdd.length > 0) {
    console.log(`Adding ${toAdd.length} brushes to ${targetXmlPath}`);
    const closeTag = `</${rootTag}>`;
    const idx = targetContent.lastIndexOf(closeTag);
    if (idx !== -1) {
      targetContent = targetContent.slice(0, idx) + '\n\t<!-- Synced Brushes -->\n' + toAdd.join('\n\n') + '\n' + targetContent.slice(idx);
      fs.writeFileSync(targetXmlPath, targetContent, 'utf8');
    }
  }
}

function syncFileBorders(targetXmlPath, sourceBorders) {
  if (!fs.existsSync(targetXmlPath)) return;
  let targetContent = fs.readFileSync(targetXmlPath, 'utf8');
  const existingMatches = targetContent.matchAll(/<border\s+id="([^"]+)"/g);
  const existingIds = new Set();
  for (const em of existingMatches) {
    existingIds.add(em[1]);
  }

  const toAdd = [];
  for (const sb of sourceBorders) {
    if (!existingIds.has(sb.id)) {
      toAdd.push(sb.xml);
      existingIds.add(sb.id);
    }
  }

  if (toAdd.length > 0) {
    console.log(`Adding ${toAdd.length} borders to ${targetXmlPath}`);
    const closeTag = `</materials>`;
    const idx = targetContent.lastIndexOf(closeTag);
    if (idx !== -1) {
      targetContent = targetContent.slice(0, idx) + '\n\t<!-- Synced Borders -->\n' + toAdd.join('\n\n') + '\n' + targetContent.slice(idx);
      fs.writeFileSync(targetXmlPath, targetContent, 'utf8');
    }
  }
}

const grounds1310 = parseBrushes(path.join(src1310, 'grounds.xml'));
const doodads1310 = parseBrushes(path.join(src1310, 'doodads.xml'));
const walls1310 = parseBrushes(path.join(src1310, 'walls.xml'));
const borders1310 = parseBorders(path.join(src1310, 'borders.xml'));

const targetVersions = ['1098', '854', '1290', '1010', '10100', '1020', '1021', '1030', '1031', '1035', '1041', '1077', '1271', '1281', '1285', '1286', '1287', '860', '870', '910', '920', '946', '954', '960', '970', '986', '810', '820', '840', '850'];

for (const v of targetVersions) {
  const vDir = path.join(dataRoot, v);
  if (!fs.existsSync(vDir)) continue;

  syncFileBorders(path.join(vDir, 'borders.xml'), borders1310);
  syncFileBrushes(path.join(vDir, 'grounds.xml'), grounds1310, 'materials');
  syncFileBrushes(path.join(vDir, 'doodads.xml'), doodads1310, 'materials');
  syncFileBrushes(path.join(vDir, 'walls.xml'), walls1310, 'materials');

  // Also sync tilesets.xml for 1098 and 854 so all structured categories (Nature, Tiny Borders, City Walls, etc.) match 1310
  if (v === '1098' || v === '854' || v === '1290') {
    fs.copyFileSync(path.join(src1310, 'tilesets.xml'), path.join(vDir, 'tilesets.xml'));
  }
}

console.log('Sync complete.');
