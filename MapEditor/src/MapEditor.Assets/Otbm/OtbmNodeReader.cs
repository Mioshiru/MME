namespace MapEditor.Assets.Otbm;

/// <summary>
/// Low-level sequential reader for OTBM's escape-coded binary node stream.
/// Handles the 0xFD escape byte transparently so callers see clean data.
/// </summary>
internal sealed class OtbmNodeReader
{
    private readonly Memory<byte> _data;
    private int _pos;

    private const byte NodeStart  = 0xFE;
    private const byte NodeEnd    = 0xFF;
    private const byte EscapeByte = 0xFD;

    public OtbmNodeReader(Memory<byte> data) => _data = data;

    // ── Raw byte access ───────────────────────────────────────────────────────

    /// <summary>Reads one decoded byte (handles escape sequences).</summary>
    public byte ReadByte()
    {
        byte b = _data.Span[_pos++];
        if (b == EscapeByte) b = _data.Span[_pos++]; // next byte is the literal
        return b;
    }

    /// <summary>Reads 2-byte little-endian uint16 (decoded).</summary>
    public ushort ReadUInt16() => (ushort)(ReadByte() | (ReadByte() << 8));

    /// <summary>Reads 4-byte little-endian uint32 (decoded).</summary>
    public uint ReadUInt32()
    {
        uint v = ReadByte();
        v |= (uint)ReadByte() << 8;
        v |= (uint)ReadByte() << 16;
        v |= (uint)ReadByte() << 24;
        return v;
    }

    /// <summary>Reads a length-prefixed string (uint16 length + UTF8 bytes).</summary>
    public string ReadString()
    {
        ushort len = ReadUInt16();
        if (len == 0) return string.Empty;
        var chars = new char[len];
        for (int i = 0; i < len; i++) chars[i] = (char)ReadByte();
        return new string(chars);
    }

    // ── Node boundary helpers ─────────────────────────────────────────────────

    /// <summary>Returns true if the next raw byte (no escape) is a node start marker.</summary>
    public bool PeekNodeStart() => _pos < _data.Length && _data.Span[_pos] == NodeStart;

    /// <summary>Returns true if the next raw byte is a node end marker.</summary>
    public bool PeekNodeEnd() => _pos < _data.Length && _data.Span[_pos] == NodeEnd;

    public void ExpectNodeStart()
    {
        if (_data.Span[_pos] != NodeStart)
            throw new InvalidDataException($"Expected NodeStart (0xFE) at pos {_pos}, got 0x{_data.Span[_pos]:X2}");
        _pos++;
    }

    public void ExpectNodeEnd()
    {
        if (_data.Span[_pos] != NodeEnd)
            throw new InvalidDataException($"Expected NodeEnd (0xFF) at pos {_pos}, got 0x{_data.Span[_pos]:X2}");
        _pos++;
    }

    /// <summary>Skips an entire node and its children recursively.</summary>
    public void SkipNode()
    {
        int depth = 1;
        while (depth > 0 && _pos < _data.Length)
        {
            byte b = _data.Span[_pos++];
            if (b == EscapeByte) { _pos++; continue; }
            if (b == NodeStart) depth++;
            else if (b == NodeEnd) depth--;
        }
    }
}
