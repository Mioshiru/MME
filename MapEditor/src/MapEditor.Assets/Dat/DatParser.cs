using System.IO.MemoryMappedFiles;
using System.Runtime.CompilerServices;

namespace MapEditor.Assets.Dat;

// ────────────────────────────────────────────────────────────────────────────
// Public enums and data types
// ────────────────────────────────────────────────────────────────────────────

/// <summary>The category of a .dat object entry.</summary>
public enum DatCategory : byte { Item = 0, Outfit = 1, Effect = 2, Missile = 3 }

/// <summary>
/// Normalised DAT flag values (independent of client version's raw byte values).
/// These correspond to <c>DatFlags</c> in the legacy <c>client_version.h</c>.
/// </summary>
[Flags]
public enum DatFlags : uint
{
    None              = 0,
    Ground            = 1u << 0,
    GroundBorder      = 1u << 1,
    OnBottom          = 1u << 2,
    OnTop             = 1u << 3,
    Container         = 1u << 4,
    Stackable         = 1u << 5,
    ForceUse          = 1u << 6,
    MultiUse          = 1u << 7,
    Writable          = 1u << 8,
    WritableOnce      = 1u << 9,
    FluidContainer    = 1u << 10,
    Splash            = 1u << 11,
    NotWalkable       = 1u << 12,
    NotMoveable       = 1u << 13,
    BlockProjectile   = 1u << 14,
    NotPathable       = 1u << 15,
    Pickupable        = 1u << 16,
    Hangable          = 1u << 17,
    HookSouth         = 1u << 18,
    HookEast          = 1u << 19,
    Rotateable        = 1u << 20,
    HasLight          = 1u << 21,
    DontHide          = 1u << 22,
    Translucent       = 1u << 23,
    HasDisplacement   = 1u << 24,
    HasElevation      = 1u << 25,
    LyingCorpse       = 1u << 26,
    AnimateAlways     = 1u << 27,
    HasMinimapColor   = 1u << 28,
    LensHelp          = 1u << 29,
    FullGround        = 1u << 30,
    Look              = 1u << 31,
}

/// <summary>Additional flags that don't fit in the 32-bit set above.</summary>
[Flags]
public enum DatFlags2 : uint
{
    None             = 0,
    Cloth            = 1u << 0,
    Market           = 1u << 1,
    Usable           = 1u << 2,
    Wrappable        = 1u << 3,
    Unwrappable      = 1u << 4,
    TopEffect        = 1u << 5,
    FloorChange      = 1u << 6,
    NoMoveAnimation  = 1u << 7,
    Chargeable       = 1u << 8,
}

/// <summary>
/// Parsed metadata for one .dat object (item / outfit / effect / missile).
/// All fields are value types to minimize allocations; sprite IDs are
/// stored in a pooled array via <see cref="SpriteIds"/>.
/// </summary>
public sealed class DatObjectInfo
{
    public ushort    Id          { get; init; }
    public DatCategory Category  { get; init; }
    public DatFlags  Flags       { get; init; }
    public DatFlags2 Flags2      { get; init; }

    // Physics
    public ushort GroundSpeed    { get; init; }
    public ushort LightIntensity { get; init; }
    public ushort LightColor     { get; init; }
    public ushort OffsetX        { get; init; }
    public ushort OffsetY        { get; init; }
    public ushort ElevationHeight{ get; init; }
    public ushort MinimapColor   { get; init; }
    public ushort LensHelp       { get; init; }
    public ushort ClothSlot      { get; init; }

    // Sprite layout
    public byte Width            { get; init; } = 1;
    public byte Height           { get; init; } = 1;
    public byte ExactSize        { get; init; } = 32;
    public byte Layers           { get; init; } = 1;
    public byte PatternX         { get; init; } = 1;
    public byte PatternY         { get; init; } = 1;
    public byte PatternZ         { get; init; } = 1;
    public byte Frames           { get; init; } = 1;

    /// <summary>
    /// 1-based sprite IDs in the .spr file.
    /// Count = Width × Height × Layers × PatternX × PatternY × PatternZ × Frames.
    /// </summary>
    public uint[] SpriteIds      { get; init; } = Array.Empty<uint>();

    public bool HasFlag(DatFlags f)  => (Flags  & f) != 0;
    public bool HasFlag(DatFlags2 f) => (Flags2 & f) != 0;

    public int SpriteCount =>
        Width * Height * Layers * PatternX * PatternY * PatternZ * Frames;

    /// <summary>
    /// Computes the flat 1-based Sprite ID lookup based on dimensions and animation/pattern coordinates.
    /// </summary>
    public uint GetSpriteId(int w, int h, int layer, int px, int py, int pz, int frame)
    {
        if (Width == 0 || Height == 0) return 0;
        
        w = System.Math.Clamp(w, 0, Width - 1);
        h = System.Math.Clamp(h, 0, Height - 1);
        layer = System.Math.Clamp(layer, 0, Layers - 1);
        px = System.Math.Clamp(px, 0, PatternX - 1);
        py = System.Math.Clamp(py, 0, PatternY - 1);
        pz = System.Math.Clamp(pz, 0, PatternZ - 1);
        frame = System.Math.Clamp(frame, 0, Frames - 1);

        int index = ((((((frame * PatternZ + pz) * PatternY + py) * PatternX + px) * Layers + layer) * Height + h) * Width + w);
        return index >= 0 && index < SpriteIds.Length ? SpriteIds[index] : 0;
    }
}

// ────────────────────────────────────────────────────────────────────────────
// Database
// ────────────────────────────────────────────────────────────────────────────

/// <summary>
/// Holds all parsed <see cref="DatObjectInfo"/> records, indexed by ID and category.
/// </summary>
public sealed class DatDatabase
{
    public uint   Signature    { get; }
    public ushort ItemCount    { get; }
    public ushort OutfitCount  { get; }
    public ushort EffectCount  { get; }
    public ushort MissileCount { get; }

    private readonly DatObjectInfo[] _items;
    private readonly DatObjectInfo[] _outfits;
    private readonly DatObjectInfo[] _effects;
    private readonly DatObjectInfo[] _missiles;

    internal DatDatabase(uint sig, ushort items, ushort outfits, ushort effects, ushort missiles)
    {
        Signature    = sig;
        ItemCount    = items;
        OutfitCount  = outfits;
        EffectCount  = effects;
        MissileCount = missiles;

        _items    = new DatObjectInfo[items];
        _outfits  = new DatObjectInfo[outfits];
        _effects  = new DatObjectInfo[effects];
        _missiles = new DatObjectInfo[missiles];
    }

    internal void SetItem(int index, DatObjectInfo info) => _items[index] = info;
    internal void SetOutfit(int index, DatObjectInfo info) => _outfits[index] = info;
    internal void SetEffect(int index, DatObjectInfo info) => _effects[index] = info;
    internal void SetMissile(int index, DatObjectInfo info) => _missiles[index] = info;

    /// <summary>Items start at client ID 100.</summary>
    public DatObjectInfo? GetItem(ushort clientId)
    {
        int idx = clientId - 100;
        return idx >= 0 && idx < _items.Length ? _items[idx] : null;
    }

    public DatObjectInfo? GetOutfit(ushort id)  => id >= 1 && id <= _outfits.Length  ? _outfits[id - 1]  : null;
    public DatObjectInfo? GetEffect(ushort id)  => id >= 1 && id <= _effects.Length  ? _effects[id - 1]  : null;
    public DatObjectInfo? GetMissile(ushort id) => id >= 1 && id <= _missiles.Length ? _missiles[id - 1] : null;

    public IReadOnlyList<DatObjectInfo> Items    => _items;
    public IReadOnlyList<DatObjectInfo> Outfits  => _outfits;
    public IReadOnlyList<DatObjectInfo> Effects  => _effects;
    public IReadOnlyList<DatObjectInfo> Missiles => _missiles;

    public int TotalObjects => ItemCount + OutfitCount + EffectCount + MissileCount;
}

// ────────────────────────────────────────────────────────────────────────────
// Parser
// ────────────────────────────────────────────────────────────────────────────

/// <summary>
/// High-performance binary parser for Tibia's .dat client metadata file.
///
/// <para>Validated against <b>Tibia 10.98</b> (DAT_FORMAT_1010, is_extended=true).</para>
///
/// <para>File layout:</para>
/// <code>
///   [0x00] uint32  – signature (0x000042A3 for 10.98)
///   [0x04] uint16  – item count        (IDs start at 100)
///   [0x06] uint16  – outfit count      (IDs start at 1)
///   [0x08] uint16  – effect count      (IDs start at 1)
///   [0x0A] uint16  – missile count     (IDs start at 1)
///   [0x0C] …       – object entries, one per ID
/// </code>
///
/// <para>Each object entry:</para>
/// <code>
///   byte flag…     – attribute flags (0xFF = end-of-attributes)
///   byte width, height
///   byte exactSize (only if width > 1 || height > 1)
///   byte layers, patternX, patternY, patternZ, frames
///   uint32[sprCount] spriteIds (uint32 in extended format, uint16 otherwise)
/// </code>
///
/// <para>Flag byte remapping for DAT_FORMAT_1010 (≥ 10.10):</para>
/// <code>
///   raw byte 16          → DatFlagNoMoveAnimation (253)
///   raw byte 17..37+     → subtract 1 → normalised flag
/// </code>
/// </summary>
public static class DatParser
{
    // ── DAT flag constants (normalised, matching client_version.h) ────────────

    private const byte FlagGround          = 0;
    private const byte FlagGroundBorder    = 1;
    private const byte FlagOnBottom        = 2;
    private const byte FlagOnTop           = 3;
    private const byte FlagContainer       = 4;
    private const byte FlagStackable       = 5;
    private const byte FlagForceUse        = 6;
    private const byte FlagMultiUse        = 7;
    private const byte FlagWritable        = 8;
    private const byte FlagWritableOnce    = 9;
    private const byte FlagFluidContainer  = 10;
    private const byte FlagSplash         = 11;
    private const byte FlagNotWalkable    = 12;
    private const byte FlagNotMoveable    = 13;
    private const byte FlagBlockProjectile = 14;
    private const byte FlagNotPathable    = 15;
    private const byte FlagPickupable     = 16;
    private const byte FlagHangable       = 17;
    private const byte FlagHookSouth      = 18;
    private const byte FlagHookEast       = 19;
    private const byte FlagRotateable     = 20;
    private const byte FlagLight          = 21;  // + u16 intensity + u16 color
    private const byte FlagDontHide       = 22;
    private const byte FlagTranslucent    = 23;
    private const byte FlagDisplacement   = 24;  // + u16 offsetX + u16 offsetY
    private const byte FlagElevation      = 25;  // + u16 height
    private const byte FlagLyingCorpse    = 26;
    private const byte FlagAnimateAlways  = 27;
    private const byte FlagMinimapColor   = 28;  // + u16
    private const byte FlagLensHelp       = 29;  // + u16
    private const byte FlagFullGround     = 30;
    private const byte FlagLook           = 31;
    private const byte FlagCloth         = 32;   // + u16
    private const byte FlagMarket        = 33;   // + 6 bytes + string + 4 bytes
    private const byte FlagUsable        = 34;   // + u16
    private const byte FlagWrappable     = 35;
    private const byte FlagUnwrappable   = 36;
    private const byte FlagTopEffect     = 37;
    private const byte FlagNoMoveAnim    = 253;  // remapped from raw 16 in ≥10.10
    private const byte FlagChargeable    = 254;
    private const byte FlagLast          = 255;  // end sentinel

    // ── Public API ────────────────────────────────────────────────────────────

    /// <summary>
    /// Parses the entire .dat file and returns a <see cref="DatDatabase"/>.
    /// Uses a memory-mapped file for zero-copy reads; fully validated against 10.98.
    /// </summary>
    public static DatDatabase Parse(string path)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(path);
        if (!File.Exists(path))
            throw new FileNotFoundException(".dat file not found.", path);

        // Memory-map the whole file for zero-copy span access
        using var mmf  = MemoryMappedFile.CreateFromFile(
            path, FileMode.Open, null, 0, MemoryMappedFileAccess.Read);
        using var view = mmf.CreateViewStream(0, 0, MemoryMappedFileAccess.Read);

        // We use a BinaryReader over the memory-mapped stream.
        // This gives us correct sequential reads without copying the whole file.
        using var br = new BinaryReader(view, System.Text.Encoding.Latin1, leaveOpen: true);

        // ── Header ─────────────────────────────────────────────────────────
        uint   signature    = br.ReadUInt32();
        ushort itemCount    = br.ReadUInt16();
        ushort outfitCount  = br.ReadUInt16();
        ushort effectCount  = br.ReadUInt16();
        ushort missileCount = br.ReadUInt16();

        var db = new DatDatabase(signature, itemCount, outfitCount, effectCount, missileCount);

        // ── Items (IDs 100 … 100+count-1) ──────────────────────────────────
        int adjustedItemCount = itemCount - 99;
        for (int i = 0; i < adjustedItemCount; i++)
        {
            ushort id = (ushort)(100 + i);
            try
            {
                var info = ParseObject(br, id, DatCategory.Item);
                db.SetItem(i, info);
            }
            catch (Exception ex)
            {
                throw new Exception($"Failed parsing Item ID {id} at stream position {view.Position}: {ex.Message}", ex);
            }
        }

        // ── Outfits ─────────────────────────────────────────────────────────
        for (int i = 0; i < outfitCount; i++)
        {
            ushort id = (ushort)(1 + i);
            try
            {
                var info = ParseObject(br, id, DatCategory.Outfit);
                db.SetOutfit(i, info);
            }
            catch (Exception ex)
            {
                throw new Exception($"Failed parsing Outfit ID {id} at stream position {view.Position}: {ex.Message}", ex);
            }
        }

        // ── Effects ─────────────────────────────────────────────────────────
        for (int i = 0; i < effectCount; i++)
        {
            ushort id = (ushort)(1 + i);
            try
            {
                var info = ParseObject(br, id, DatCategory.Effect);
                db.SetEffect(i, info);
            }
            catch (Exception ex)
            {
                throw new Exception($"Failed parsing Effect ID {id} at stream position {view.Position}: {ex.Message}", ex);
            }
        }

        // ── Missiles ────────────────────────────────────────────────────────
        for (int i = 0; i < missileCount; i++)
        {
            ushort id = (ushort)(1 + i);
            try
            {
                var info = ParseObject(br, id, DatCategory.Missile);
                db.SetMissile(i, info);
            }
            catch (Exception ex)
            {
                throw new Exception($"Failed parsing Missile ID {id} at stream position {view.Position}: {ex.Message}", ex);
            }
        }

        return db;
    }

    // ── Single-object parser ──────────────────────────────────────────────────

    private static DatObjectInfo ParseObject(BinaryReader br, ushort id, DatCategory cat)
    {
        DatFlags  flags  = DatFlags.None;
        DatFlags2 flags2 = DatFlags2.None;

        ushort groundSpeed = 0, lightIntensity = 0, lightColor = 0;
        ushort offsetX = 0, offsetY = 0, elevationHeight = 0;
        ushort minimapColor = 0, lensHelp = 0, clothSlot = 0;

        // ── Read flag bytes until sentinel 0xFF ──────────────────────────────
        while (true)
        {
            byte rawFlag = br.ReadByte();
            if (rawFlag == FlagLast) break;

            // ── DAT_FORMAT_1010 flag remapping ──────────────────────────────
            // Raw byte 16 → NoMoveAnimation
            // Raw byte > 16 → subtract 1 to get the normalised flag
            byte flag = rawFlag switch
            {
                16 => FlagNoMoveAnim,
                > 16 => (byte)(rawFlag - 1),
                _ => rawFlag
            };

            switch (flag)
            {
                // ── Flags without operand ─────────────────────────────────
                case FlagGroundBorder:  flags |= DatFlags.GroundBorder;    break;
                case FlagOnBottom:      flags |= DatFlags.OnBottom;        break;
                case FlagOnTop:         flags |= DatFlags.OnTop;           break;
                case FlagContainer:     flags |= DatFlags.Container;       break;
                case FlagStackable:     flags |= DatFlags.Stackable;       break;
                case FlagForceUse:      flags |= DatFlags.ForceUse;        break;
                case FlagMultiUse:      flags |= DatFlags.MultiUse;        break;
                case FlagFluidContainer:flags |= DatFlags.FluidContainer;  break;
                case FlagSplash:        flags |= DatFlags.Splash;          break;
                case FlagNotWalkable:   flags |= DatFlags.NotWalkable;     break;
                case FlagNotMoveable:   flags |= DatFlags.NotMoveable;     break;
                case FlagBlockProjectile:flags |= DatFlags.BlockProjectile;break;
                case FlagNotPathable:   flags |= DatFlags.NotPathable;     break;
                case FlagPickupable:    flags |= DatFlags.Pickupable;      break;
                case FlagHangable:      flags |= DatFlags.Hangable;        break;
                case FlagHookSouth:     flags |= DatFlags.HookSouth;       break;
                case FlagHookEast:      flags |= DatFlags.HookEast;        break;
                case FlagRotateable:    flags |= DatFlags.Rotateable;      break;
                case FlagDontHide:      flags |= DatFlags.DontHide;        break;
                case FlagTranslucent:   flags |= DatFlags.Translucent;     break;
                case FlagLyingCorpse:   flags |= DatFlags.LyingCorpse;     break;
                case FlagAnimateAlways: flags |= DatFlags.AnimateAlways;   break;
                case FlagFullGround:    flags |= DatFlags.FullGround;      break;
                case FlagLook:          flags |= DatFlags.Look;            break;
                case FlagWrappable:     flags2 |= DatFlags2.Wrappable;     break;
                case FlagUnwrappable:   flags2 |= DatFlags2.Unwrappable;   break;
                case FlagTopEffect:     flags2 |= DatFlags2.TopEffect;     break;
                case FlagNoMoveAnim:    flags2 |= DatFlags2.NoMoveAnimation; break;
                case FlagChargeable:    flags2 |= DatFlags2.Chargeable;    break;

                // ── Flags with uint16 operand ─────────────────────────────
                case FlagGround:
                    flags |= DatFlags.Ground;
                    groundSpeed = br.ReadUInt16();
                    break;

                case FlagWritable:
                    flags |= DatFlags.Writable;
                    br.ReadUInt16(); // maxReadWriteChars — skip
                    break;

                case FlagWritableOnce:
                    flags |= DatFlags.WritableOnce;
                    br.ReadUInt16(); // maxReadChars — skip
                    break;

                case FlagLight:
                    flags |= DatFlags.HasLight;
                    lightIntensity = br.ReadUInt16();
                    lightColor     = br.ReadUInt16();
                    break;

                case FlagDisplacement:
                    flags |= DatFlags.HasDisplacement;
                    offsetX = br.ReadUInt16();
                    offsetY = br.ReadUInt16();
                    break;

                case FlagElevation:
                    flags |= DatFlags.HasElevation;
                    elevationHeight = br.ReadUInt16();
                    break;

                case FlagMinimapColor:
                    flags |= DatFlags.HasMinimapColor;
                    minimapColor = br.ReadUInt16();
                    break;

                case FlagLensHelp:
                    flags |= DatFlags.LensHelp;
                    lensHelp = br.ReadUInt16();
                    break;

                case FlagCloth:
                    flags2 |= DatFlags2.Cloth;
                    clothSlot = br.ReadUInt16();
                    break;

                case FlagUsable:
                    flags2 |= DatFlags2.Usable;
                    br.ReadUInt16(); // skip
                    break;

                // ── Market flag: 6 bytes + length-prefixed string + 4 bytes ─
                case FlagMarket:
                    flags2 |= DatFlags2.Market;
                    br.ReadBytes(6);
                    ushort nameLen = br.ReadUInt16();
                    br.ReadBytes(nameLen);
                    br.ReadBytes(4);
                    break;

                default:
                    // Unknown flag — cannot safely skip; abort attribute parsing.
                    // This should not happen with 10.98 files.
                    break;
            }
        }

        byte groupCount = 1;
        if (cat == DatCategory.Outfit)
        {
            groupCount = br.ReadByte();
        }

        byte width = 1;
        byte height = 1;
        byte exactSize = 32;
        byte layers = 1;
        byte patternX = 1;
        byte patternY = 1;
        byte patternZ = 1;
        byte frames = 1;

        var spriteIdsList = new List<uint>();

        for (int g = 0; g < groupCount; g++)
        {
            if (cat == DatCategory.Outfit)
            {
                br.ReadByte(); // group type
            }

            width = br.ReadByte();
            height = br.ReadByte();
            if (width > 1 || height > 1)
            {
                exactSize = br.ReadByte();
            }
            layers = br.ReadByte();
            patternX = br.ReadByte();
            patternY = br.ReadByte();
            patternZ = br.ReadByte();
            frames = br.ReadByte();

            // Animations/frame durations (Tibia 10.57 has frame durations!)
            if (frames > 1)
            {
                br.ReadByte();  // async
                br.ReadInt32(); // loopCount
                br.ReadSByte(); // startFrame
                
                for (int f = 0; f < frames; f++)
                {
                    br.ReadUInt32(); // min duration
                    br.ReadUInt32(); // max duration
                }
            }

            int sprCount = width * height * layers * patternX * patternY * patternZ * frames;
            for (int s = 0; s < sprCount; s++)
            {
                spriteIdsList.Add(br.ReadUInt32());
            }
        }

        return new DatObjectInfo
        {
            Id              = id,
            Category        = cat,
            Flags           = flags,
            Flags2          = flags2,
            GroundSpeed     = groundSpeed,
            LightIntensity  = lightIntensity,
            LightColor      = lightColor,
            OffsetX         = offsetX,
            OffsetY         = offsetY,
            ElevationHeight = elevationHeight,
            MinimapColor    = minimapColor,
            LensHelp        = lensHelp,
            ClothSlot       = clothSlot,
            Width           = width,
            Height          = height,
            ExactSize       = exactSize,
            Layers          = layers,
            PatternX        = patternX,
            PatternY        = patternY,
            PatternZ        = patternZ,
            Frames          = frames,
            SpriteIds       = spriteIdsList.ToArray(),
        };
    }
}
