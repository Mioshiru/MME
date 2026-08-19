const fs = require('fs');
const path = require('path');

const src = 'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/dist/Saves/Slot 1';
const dstBuild = 'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/build/Saves/Slot 1';

if (fs.existsSync(src)) {
  fs.mkdirSync(dstBuild, { recursive: true });
  for (const f of fs.readdirSync(src)) {
    fs.copyFileSync(path.join(src, f), path.join(dstBuild, f));
  }
  console.log('Synchronized Slot 1 to build/Saves/Slot 1');
}
