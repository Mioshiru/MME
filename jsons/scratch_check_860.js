const fs = require('fs');

function extractBrushNames(xmlPath) {
  if (!fs.existsSync(xmlPath)) return new Set();
  const content = fs.readFileSync(xmlPath, 'utf8');
  const matches = content.matchAll(/<brush\s+name="([^"]+)"/g);
  const set = new Set();
  for (const m of matches) {
    set.add(m[1]);
  }
  return set;
}

function extractTilesetBrushes(tilesetPath) {
  if (!fs.existsSync(tilesetPath)) return [];
  const content = fs.readFileSync(tilesetPath, 'utf8');
  const regex = /<tileset\s+name="([^"]+)">([\s\S]*?)<\/tileset>/g;
  const tilesets = [];
  let m;
  while ((m = regex.exec(content)) !== null) {
    const tsName = m[1];
    const body = m[2];
    const brushMatches = body.matchAll(/<brush\s+name="([^"]+)"/g);
    const brushes = [];
    for (const bm of brushMatches) {
      brushes.push(bm[1]);
    }
    tilesets.push({ name: tsName, brushes });
  }
  return tilesets;
}

const vDir = 'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/860';
const grounds = extractBrushNames(`${vDir}/grounds.xml`);
const doodads = extractBrushNames(`${vDir}/doodads.xml`);
const walls = extractBrushNames(`${vDir}/walls.xml`);
const allBrushes = new Set([...grounds, ...doodads, ...walls]);
const tilesets = extractTilesetBrushes(`${vDir}/tilesets.xml`);
for (const ts of tilesets) {
  const missing = ts.brushes.filter(b => !allBrushes.has(b));
  if (missing.length > 0) {
    console.log(`860 Tileset ${ts.name} missing:`, missing);
  }
}
