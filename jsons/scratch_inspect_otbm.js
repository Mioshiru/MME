const fs = require('fs');
const path = 'c:/Users/weber/Dokumente/Projekt/In Arbeit/Map Editor/dist/Saves/Slot 1/atlantica.otbm';

const buf = fs.readFileSync(path);
console.log('File size:', buf.length);
console.log('First 32 bytes:', buf.slice(0, 32));

// In OTBM node tree:
// 4 bytes: 0 0 0 0
// 1 byte: 0xFE (NODE_START)
// 1 byte: type
// 4 bytes: version (u32)
// 2 bytes: width (u16)
// 2 bytes: height (u16)
// 4 bytes: otb major (u32)
// 4 bytes: otb minor / client version (u32)

let offset = 4;
if (buf[offset] === 0xFE) offset++; // NODE_START
const nodeType = buf[offset++];
const mapVersion = buf.readUInt32LE(offset); offset += 4;
const width = buf.readUInt16LE(offset); offset += 2;
const height = buf.readUInt16LE(offset); offset += 2;
const otbMajor = buf.readUInt32LE(offset); offset += 4;
const otbMinor = buf.readUInt32LE(offset); offset += 4;

console.log({
  nodeType,
  mapVersion,
  width,
  height,
  otbMajor,
  otbMinor,
  otbMinorHex: '0x' + otbMinor.toString(16)
});
