// Format probe — runs against the actual 10.98 files
using System.Runtime.InteropServices;

string datPath = args.Length > 0 ? args[0] : @"data\1098\Tibia.dat";
string sprPath = args.Length > 1 ? args[1] : @"data\1098\Tibia.spr";

Console.OutputEncoding = System.Text.Encoding.UTF8;

// ─── SPR probe ──────────────────────────────────────────────────────────────
{
    using var fs = File.OpenRead(sprPath);
    using var br = new BinaryReader(fs);

    uint sig   = br.ReadUInt32();
    uint count = br.ReadUInt32();  // 10.78+ uses uint32 count
    Console.WriteLine($"=== SPR ===");
    Console.WriteLine($"  Signature : 0x{sig:X8}");
    Console.WriteLine($"  Count     : {count}");
    Console.WriteLine($"  File size : {new FileInfo(sprPath).Length:N0} bytes");

    // First 10 offsets
    var offsets = new uint[Math.Min(count, 200_000)];
    for (int i = 0; i < offsets.Length; i++) offsets[i] = br.ReadUInt32();

    // Find first/second/third non-zero sprite
    int shown = 0;
    for (int i = 0; i < offsets.Length && shown < 3; i++)
    {
        if (offsets[i] == 0) continue;
        shown++;
        uint off = offsets[i];
        fs.Seek(off, SeekOrigin.Begin);
        byte r = br.ReadByte(), g = br.ReadByte(), b = br.ReadByte();
        ushort transp  = br.ReadUInt16();
        ushort colored = br.ReadUInt16();
        Console.WriteLine($"  Sprite #{i+1}: off={off}  colorkey=({r:X2},{g:X2},{b:X2})  transp={transp}  colored={colored}");
    }
    Console.WriteLine($"  Offset table ends at: {8 + (long)count * 4}");
}

// ─── DAT probe ──────────────────────────────────────────────────────────────
{
    using var fs = File.OpenRead(datPath);
    using var br = new BinaryReader(fs);

    uint   sig      = br.ReadUInt32();
    ushort items    = br.ReadUInt16();
    ushort outfits  = br.ReadUInt16();
    ushort effects  = br.ReadUInt16();
    ushort missiles = br.ReadUInt16();

    Console.WriteLine($"\n=== DAT ===");
    Console.WriteLine($"  Signature : 0x{sig:X8}");
    Console.WriteLine($"  Items     : {items}  (IDs 100..{100+items-1})");
    Console.WriteLine($"  Outfits   : {outfits}");
    Console.WriteLine($"  Effects   : {effects}");
    Console.WriteLine($"  Missiles  : {missiles}");
    Console.WriteLine($"  Data starts at byte 12 (0x0C)");

    // Parse the first item (ID 100) manually with two sprite ID size guesses
    long dataStart = fs.Position; // should be 12
    Console.WriteLine($"  Position after header: {dataStart}");

    // Dump raw bytes for visual inspection
    var raw = br.ReadBytes(80);
    Console.Write("  First 80 bytes: ");
    Console.WriteLine(string.Join(" ", raw.Select(x => x.ToString("X2"))));

    // Parse: read attribute bytes until 0xFF
    fs.Seek(dataStart, SeekOrigin.Begin);
    var attrs = new List<byte>();
    while (true)
    {
        byte a = br.ReadByte();
        if (a == 0xFF) break;
        attrs.Add(a);

        // Skip the operand for known 2-byte attrs
        if (a == 0x00) { attrs.Add(br.ReadByte()); attrs.Add(br.ReadByte()); } // ground speed u16
        else if (a == 0x16 || a == 0x17 || a == 0x18 || a == 0x19 || a == 0x1A ||
                 a == 0x1B || a == 0x1C || a == 0x1D || a == 0x1E || a == 0x1F ||
                 a == 0x20 || a == 0x21 || a == 0x22 || a == 0x23 || a == 0x24)
        { attrs.Add(br.ReadByte()); attrs.Add(br.ReadByte()); } // u16 operand
    }

    long afterFF = fs.Position;
    Console.WriteLine($"  Attrs: [{string.Join(",", attrs.Select(x => x.ToString("X2")))}]  -> 0xFF at byte {afterFF - 1}");

    // Read sprite layout (8 or 9 bytes depending on size)
    byte w = br.ReadByte(), h = br.ReadByte();
    byte exactSize = (w > 1 || h > 1) ? br.ReadByte() : (byte)32;
    byte layers = br.ReadByte(), px = br.ReadByte(), py = br.ReadByte(), pz = br.ReadByte(), frames = br.ReadByte();
    int sprCount = w * h * layers * px * py * pz * frames;
    Console.WriteLine($"  Layout: w={w} h={h} exactSize={exactSize} layers={layers} px={px} py={py} pz={pz} frames={frames} -> {sprCount} sprite(s)");

    // Try u32 sprite IDs (10.78+)
    long posBeforeId = fs.Position;
    uint id32a = br.ReadUInt32();
    fs.Seek(posBeforeId, SeekOrigin.Begin);
    ushort id16a = br.ReadUInt16();
    Console.WriteLine($"  First sprite ID as u16={id16a}  as u32={id32a}");

    // Cross-check: 10.78 uses u32 because sprite count > 65535
    // If u32: ID should be <= 338944 and > 0  (or 0 for empty)
    // If u16: it would wrap for IDs > 65535
    Console.WriteLine($"  => Sprite IDs are likely u32 (count={338944} > 65535)");
}
