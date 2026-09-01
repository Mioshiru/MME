import fs from 'fs';
import path from 'path';

const sprPath = path.resolve('data/1310/Tibia.spr');
console.log('Reading:', sprPath);
const buf = fs.readFileSync(sprPath);

const signature = buf.readUInt32LE(0);
const spriteCount = buf.readUInt32LE(4);

console.log('Signature: 0x' + signature.toString(16).toUpperCase());
console.log('Sprite count:', spriteCount);
console.log('File size (bytes):', buf.length);

// Check first few offsets
for (let i = 1; i <= 5; i++) {
  const off = buf.readUInt32LE(4 + i * 4);
  console.log(`Sprite #${i} offset: ${off}`);
  if (off > 0 && off < buf.length) {
    const rKey = buf.readUInt8(off);
    const gKey = buf.readUInt8(off + 1);
    const bKey = buf.readUInt8(off + 2);
    const size = buf.readUInt16LE(off + 3);
    console.log(`  ColorKey: (${rKey}, ${gKey}, ${bKey}), RLE size: ${size}`);
  }
}
