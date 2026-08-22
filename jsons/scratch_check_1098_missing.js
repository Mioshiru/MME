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

const dir1098 = 'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1098';
const dir1310 = 'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1310';

const grounds1098 = extractBrushNames(`${dir1098}/grounds.xml`);
const doodads1098 = extractBrushNames(`${dir1098}/doodads.xml`);
const walls1098 = extractBrushNames(`${dir1098}/walls.xml`);
const allBrushes1098 = new Set([...grounds1098, ...doodads1098, ...walls1098]);

const tilesets1098 = extractTilesetBrushes(`${dir1098}/tilesets.xml`);

console.log('1098 total defined brushes:', allBrushes1098.size);
console.log('1098 grounds count:', grounds1098.size);
console.log('1098 doodads count:', doodads1098.size);
console.log('1098 walls count:', walls1098.size);

for (const ts of tilesets1098) {
  const missing = ts.brushes.filter(b => !allBrushes1098.has(b));
  if (missing.length > 0) {
    console.log(`Tileset "${ts.name}": total ${ts.brushes.length}, MISSING ${missing.length}:`, missing.slice(0, 10));
  }
}
