const fs = require('fs');

function parseBordersMap(xmlPath) {
  if (!fs.existsSync(xmlPath)) return new Map();
  const content = fs.readFileSync(xmlPath, 'utf8');
  const regex = /<border\s+id="([^"]+)"([\s\S]*?)<\/border>/g;
  const map = new Map();
  let m;
  while ((m = regex.exec(content)) !== null) {
    map.set(m[1], m[0]);
  }
  return map;
}

const dir1098 = 'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1098';
const dir1310 = 'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1310';

const borders1098 = parseBordersMap(`${dir1098}/borders.xml`);
const borders1310 = parseBordersMap(`${dir1310}/borders.xml`);

console.log('1098 borders count:', borders1098.size);
console.log('1310 borders count:', borders1310.size);

const missingBordersIn1098 = [];
for (const [id, xml] of borders1310.entries()) {
  if (!borders1098.has(id)) {
    missingBordersIn1098.push({ id, xml });
  }
}

console.log('Missing borders in 1098:', missingBordersIn1098.length);
