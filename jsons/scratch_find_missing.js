const fs = require('fs');

function extractTilesetBrushes(tilesetPath) {
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

function parseBrushesMap(xmlPath) {
  if (!fs.existsSync(xmlPath)) return new Map();
  const content = fs.readFileSync(xmlPath, 'utf8');
  const regex = /<brush\s+name="([^"]+)"([\s\S]*?)<\/brush>/g;
  const map = new Map();
  let m;
  while ((m = regex.exec(content)) !== null) {
    map.set(m[1], m[0]);
  }
  return map;
}

const dir1098 = 'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1098';
const dir1310 = 'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1310';

const grounds1098 = parseBrushesMap(`${dir1098}/grounds.xml`);
const doodads1098 = parseBrushesMap(`${dir1098}/doodads.xml`);
const walls1098 = parseBrushesMap(`${dir1098}/walls.xml`);

const grounds1310 = parseBrushesMap(`${dir1310}/grounds.xml`);
const doodads1310 = parseBrushesMap(`${dir1310}/doodads.xml`);
const walls1310 = parseBrushesMap(`${dir1310}/walls.xml`);

const tilesets1098 = extractTilesetBrushes(`${dir1098}/tilesets.xml`);

let missingIn1098 = [];
for (const ts of tilesets1098) {
  for (const b of ts.brushes) {
    if (!grounds1098.has(b) && !doodads1098.has(b) && !walls1098.has(b)) {
      missingIn1098.push({ tileset: ts.name, brush: b });
    }
  }
}

console.log('Total missing brush references in 1098 tilesets:', missingIn1098.length);

let foundIn1310Grounds = 0;
let foundIn1310Doodads = 0;
let foundIn1310Walls = 0;
let notFoundAnywhere = 0;

for (const item of missingIn1098) {
  if (grounds1310.has(item.brush)) {
    foundIn1310Grounds++;
  } else if (doodads1310.has(item.brush)) {
    foundIn1310Doodads++;
  } else if (walls1310.has(item.brush)) {
    foundIn1310Walls++;
  } else {
    notFoundAnywhere++;
    console.log('Not found anywhere:', item);
  }
}

console.log({
  foundIn1310Grounds,
  foundIn1310Doodads,
  foundIn1310Walls,
  notFoundAnywhere
});
