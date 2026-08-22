const fs = require('fs');
const path = require('path');

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

const dataRoot = 'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data';
const dirs = fs.readdirSync(dataRoot).filter(f => fs.statSync(path.join(dataRoot, f)).isDirectory() && !isNaN(parseInt(f)));

console.log('Checking version directories in data/...');
for (const v of dirs) {
  const vDir = path.join(dataRoot, v);
  const grounds = extractBrushNames(path.join(vDir, 'grounds.xml'));
  const doodads = extractBrushNames(path.join(vDir, 'doodads.xml'));
  const walls = extractBrushNames(path.join(vDir, 'walls.xml'));
  const allBrushes = new Set([...grounds, ...doodads, ...walls]);

  const tilesets = extractTilesetBrushes(path.join(vDir, 'tilesets.xml'));
  let totalBrushes = 0;
  let totalMissing = 0;
  for (const ts of tilesets) {
    totalBrushes += ts.brushes.length;
    const missing = ts.brushes.filter(b => !allBrushes.has(b));
    totalMissing += missing.length;
  }
  if (totalMissing > 0) {
    console.log(`Version ${v}: ${totalBrushes} tileset brushes, ${totalMissing} MISSING!`);
  } else {
    console.log(`Version ${v}: OK (0 missing)`);
  }
}
