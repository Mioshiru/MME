const fs = require('fs');

function loadItemIds(itemsXmlPath) {
  if (!fs.existsSync(itemsXmlPath)) return new Set();
  const content = fs.readFileSync(itemsXmlPath, 'utf8');
  const set = new Set();
  
  // <item id="123" ...
  for (const m of content.matchAll(/<item\s+[^>]*\bid="(\d+)"/g)) {
    set.add(parseInt(m[1]));
  }
  // <item fromid="100" toid="110" ...
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
console.log('1098 total items in items.xml:', items1098.size);
const max1098 = Math.max(...items1098);
console.log('1098 max item id:', max1098);

const items854 = loadItemIds('c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/854/items.xml');
console.log('854 total items in items.xml:', items854.size);
const max854 = Math.max(...items854);
console.log('854 max item id:', max854);
