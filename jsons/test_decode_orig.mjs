import fs from 'fs';
import path from 'path';

const sprPath = path.resolve('data/1310/Tibia.spr.original');
const buf = fs.readFileSync(sprPath);

const signature = buf.readUInt32LE(0);
const spriteCount = buf.readUInt32LE(4);
console.log('Signature:', '0x' + signature.toString(16));
console.log('Sprite count:', spriteCount);

// Test decoding Sprite #2
const off = buf.readUInt32LE(4 + 2 * 4);
console.log('Sprite #2 offset:', off);
const rKey = buf.readUInt8(off);
const gKey = buf.readUInt8(off + 1);
const bKey = buf.readUInt8(off + 2);
const rleSize = buf.readUInt16LE(off + 3);
console.log(`Key: (${rKey}, ${gKey}, ${bKey}), RLE size: ${rleSize}`);

const dump = buf.subarray(off + 5, off + 5 + rleSize);
console.log('First 20 bytes of dump:', [...dump.subarray(0, 20)].map(b => b.toString(16).padStart(2, '0')).join(' '));

let read = 0;
let totalPixels = 0;
let transparentPixels = 0;
let coloredPixels = 0;

while (read < rleSize && totalPixels < 1024) {
  const trans = dump.readUInt16LE(read);
  read += 2;
  transparentPixels += trans;
  totalPixels += trans;
  if (read >= rleSize) break;
  
  const colored = dump.readUInt16LE(read);
  read += 2;
  coloredPixels += colored;
  totalPixels += colored;
  
  // Let's see if each colored pixel is 3 bytes (RGB) or 4 bytes (RGBA):
  // Let's check remaining bytes vs expected
  console.log(`Chunk: trans=${trans}, colored=${colored}, readPos=${read}, rleSize=${rleSize}`);
  read += colored * 3; // test 3 bytes
}

console.log(`Total pixels: ${totalPixels}, transparent: ${transparentPixels}, colored: ${coloredPixels}, readFinal=${read}/${rleSize}`);
