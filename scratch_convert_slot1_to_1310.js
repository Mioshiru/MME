const fs = require('fs');
const path = require('path');

function convertOtbmVersion(filePath, targetMinorVersion) {
  if (!fs.existsSync(filePath)) {
    console.log('File not found:', filePath);
    return;
  }
  const buf = fs.readFileSync(filePath);
  let offset = 4;
  if (buf[offset] === 0xFE) offset++;
  const nodeType = buf[offset++];
  const mapVersion = buf.readUInt32LE(offset); offset += 4;
  const width = buf.readUInt16LE(offset); offset += 2;
  const height = buf.readUInt16LE(offset); offset += 2;
  const otbMajor = buf.readUInt32LE(offset); offset += 4;
  const otbMinorOffset = offset;
  const otbMinor = buf.readUInt32LE(otbMinorOffset);

  console.log(`Original ${filePath}: mapVersion=${mapVersion}, otbMajor=${otbMajor}, otbMinor=${otbMinor} (0x${otbMinor.toString(16)})`);

  // Write new minor version
  buf.writeUInt32LE(targetMinorVersion, otbMinorOffset);

  // Backup original
  fs.writeFileSync(filePath + '.bak_version', fs.readFileSync(filePath));
  // Write modified
  fs.writeFileSync(filePath, buf);

  console.log(`Converted ${filePath} to otbMinor=${targetMinorVersion} (13.10) successfully!`);
}

// Convert in dist and build and workspace if present
const paths = [
  'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/dist/Saves/Slot 1/atlantica.otbm',
  'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/build/Saves/Slot 1/atlantica.otbm',
  'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/Saves/Slot 1/atlantica.otbm'
];

for (const p of paths) {
  convertOtbmVersion(p, 65); // 65 = 13.10 in clients.xml
}
