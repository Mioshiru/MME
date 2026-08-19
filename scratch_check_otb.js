const fs = require('fs');

function readOTBMaxId(otbPath) {
  const buf = fs.readFileSync(otbPath);
  // OTB parser:
  // Root node: 0 0 0 0 0xFE (NODE_START)
  // iterate child item nodes
  let maxServerId = 0;
  let count = 0;
  let offset = 0;
  while (offset < buf.length - 10) {
    if (buf[offset] === 0xFE) { // NODE_START
      const type = buf[offset + 1];
      if (type === 1) { // OTBM_ITEM or ITEM
        // Look for server id in item node attributes
        // In OTB, each item node has flags (4 bytes) then attribute list
      }
    }
    offset++;
  }
  return { fileSize: buf.length };
}

console.log(readOTBMaxId('c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/data/1098/items.otb'));
