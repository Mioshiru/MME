const fs = require('fs');

function loadItemIds(itemsXmlPath) {
  if (!fs.existsSync(itemsXmlPath)) return new Set();
  const content = fs.readFileSync(itemsXmlPath, 'utf8');
  const set = new Set();
  for (const m of content.matchAll(/<item\s+[^>]*\bid="(\d+)"/g)) {
    set.add(parseInt(m[1]));
  }
  for (const m of content.matchAll(/<item\s+[^>]*\bfromid="(\d+)"\s+toid="(\d+)"/g)) {
    const from = parseInt(m[1]);
    const to = parseInt(m[2]);
    for (let i = from; i <= to; ++i) {
      set.add(i);
    }
  }
  return set;
}

const items1098 = loadItemIds('c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1098/items.xml');

// Parse brushes from XML
function parseBrushes(xmlPath) {
  if (!fs.existsSync(xmlPath)) return [];
  const content = fs.readFileSync(xmlPath, 'utf8');
  const regex = /<brush\s+name="([^"]+)"([\s\S]*?)<\/brush>/g;
  const list = [];
  let m;
  while ((m = regex.exec(content)) !== null) {
    const full = m[0];
    const name = m[1];
    const body = m[2];
    const itemIds = [];
    for (const im of body.matchAll(/id="(\d+)"/g)) {
      itemIds.push(parseInt(im[1]));
    }
    for (const im of body.matchAll(/fromid="(\d+)"\s+toid="(\d+)"/g)) {
      for (let i = parseInt(im[1]); i <= parseInt(im[2]); ++i) {
        itemIds.push(i);
      }
    }
    list.push({ name, full, itemIds });
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
    const full = m[0];
    const id = m[1];
    const body = m[2];
    const itemIds = [];
    for (const im of body.matchAll(/item="(\d+)"/g)) {
      itemIds.push(parseInt(im[1]));
    }
    for (const im of body.matchAll(/id="(\d+)"/g)) {
      itemIds.push(parseInt(im[1]));
    }
    list.push({ id, full, itemIds });
  }
  return list;
}

const grounds1310 = parseBrushes('c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1310/grounds.xml');
const doodads1310 = parseBrushes('c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1310/doodads.xml');
const walls1310 = parseBrushes('c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1310/walls.xml');
const borders1310 = parseBorders('c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1310/borders.xml');

console.log('1310 total grounds:', grounds1310.length);
console.log('1310 total doodads:', doodads1310.length);
console.log('1310 total walls:', walls1310.length);
console.log('1310 total borders:', borders1310.length);

const grounds1098 = parseBrushes('c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1098/grounds.xml');
const existingGroundNames1098 = new Set(grounds1098.map(g => g.name));

const doodads1098 = parseBrushes('c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1098/doodads.xml');
const existingDoodadNames1098 = new Set(doodads1098.map(d => d.name));

const walls1098 = parseBrushes('c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1098/walls.xml');
const existingWallNames1098 = new Set(walls1098.map(w => w.name));

const borders1098 = parseBorders('c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1098/borders.xml');
const existingBorderIds1098 = new Set(borders1098.map(b => b.id));

let validGroundsToAdd = 0;
let invalidGrounds = 0;
for (const g of grounds1310) {
  if (!existingGroundNames1098.has(g.name)) {
    const allValid = g.itemIds.every(id => items1098.has(id));
    if (allValid) {
      validGroundsToAdd++;
    } else {
      invalidGrounds++;
    }
  }
}

let validDoodadsToAdd = 0;
let invalidDoodads = 0;
for (const d of doodads1310) {
  if (!existingDoodadNames1098.has(d.name)) {
    const allValid = d.itemIds.every(id => items1098.has(id));
    if (allValid) {
      validDoodadsToAdd++;
    } else {
      invalidDoodads++;
    }
  }
}

let validWallsToAdd = 0;
let invalidWalls = 0;
for (const w of walls1310) {
  if (!existingWallNames1098.has(w.name)) {
    const allValid = w.itemIds.every(id => items1098.has(id));
    if (allValid) {
      validWallsToAdd++;
    } else {
      invalidWalls++;
    }
  }
}

let validBordersToAdd = 0;
for (const b of borders1310) {
  if (!existingBorderIds1098.has(b.id)) {
    const allValid = b.itemIds.every(id => items1098.has(id));
    if (allValid) {
      validBordersToAdd++;
    }
  }
}

console.log({
  validGroundsToAdd,
  invalidGrounds,
  validDoodadsToAdd,
  invalidDoodads,
  validWallsToAdd,
  invalidWalls,
  validBordersToAdd
});
