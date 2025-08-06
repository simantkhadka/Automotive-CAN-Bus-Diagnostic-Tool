#include "commbuffer.h"
#include "Logger.h"
#include "gvret_comm.h"

CommBuffer::CommBuffer()
{
    transmitBufferLength = 0; // start with an empty TX buffer
}

// Return how many bytes are currently buffered and ready to send
size_t CommBuffer::numAvailableBytes()
{
    return transmitBufferLength;
}

// Reset/empty the buffer (does not zero memory; just resets the length)
void CommBuffer::clearBufferedBytes()
{
    transmitBufferLength = 0;
}

// Direct pointer to the internal buffer (read-only usage expected by callers)
uint8_t *CommBuffer::getBufferedBytes()
{
    return transmitBuffer;
}

// Copy a block of bytes into the buffer, but never overflow it
void CommBuffer::sendBytesToBuffer(uint8_t *bytes, size_t length)
{
    size_t room = WIFI_BUFF_SIZE - transmitBufferLength;
    size_t toCopy = (length <= room) ? length : room;
    if (toCopy > 0)
    {
        memcpy(&transmitBuffer[transmitBufferLength], bytes, toCopy);
        transmitBufferLength += toCopy;
    }
}

// Queue a single byte if there is still room left
void CommBuffer::sendByteToBuffer(uint8_t byt)
{
    if (transmitBufferLength < WIFI_BUFF_SIZE)
    {
        transmitBuffer[transmitBufferLength++] = byt;
    }
}

// Convenience: queue an Arduino String by converting to C-string first
void CommBuffer::sendString(String str)
{
    char buff[300];
    str.toCharArray(buff, 300);
    sendCharString(buff);
}

// Queue a null-terminated C-string, truncated if buffer is near full
void CommBuffer::sendCharString(char *str)
{
    char *p = str;
    int i = 0;
    while (*p && transmitBufferLength < WIFI_BUFF_SIZE)
    {
        transmitBuffer[transmitBufferLength++] = *p++;
        i++;
    }
    Logger::debug("Queued %i bytes", i);
}

// --------- small helpers to make appends safer/clearer -----------

// Remaining space in the TX buffer
static inline size_t _roomLeft(size_t used)
{
    return (used < WIFI_BUFF_SIZE) ? (WIFI_BUFF_SIZE - used) : 0;
}

// Append one byte if there is room
static inline bool _appendByte(uint8_t *buf, int &len, uint8_t b)
{
    if ((size_t)len >= WIFI_BUFF_SIZE)
        return false;
    buf[len++] = b;
    return true;
}

// Append a 32-bit value in little-endian order
static inline bool _appendU32LE(uint8_t *buf, int &len, uint32_t v)
{
    return _appendByte(buf, len, (uint8_t)(v & 0xFF)) &&
           _appendByte(buf, len, (uint8_t)((v >> 8) & 0xFF)) &&
           _appendByte(buf, len, (uint8_t)((v >> 16) & 0xFF)) &&
           _appendByte(buf, len, (uint8_t)((v >> 24) & 0xFF));
}
// -----------------------------------------------------------------

// Queue a classic CAN frame in either binary or ASCII format
void CommBuffer::sendFrameToBuffer(CAN_FRAME &frame, int whichBus)
{
    uint8_t temp;
    size_t writtenBytes; // kept for parity with original structure

    if (settings.useBinarySerialComm)
    {
        // Binary packet: 0xF1,cmd,time(4),id(4),len|bus(1),data(N),checksum(1)
        // Total needed = 12 + payload
        size_t need = 12 + frame.length;
        size_t room = _roomLeft(transmitBufferLength);
        if (room < need)
        {
            // Not enough buffer space: drop this frame silently
            return;
        }

        // Mark extended ID by setting top bit of the ID field
        if (frame.extended)
            frame.id |= 1u << 31;

        // Build binary packet
        int w = (int)transmitBufferLength;
        _appendByte(transmitBuffer, w, 0xF1);
        _appendByte(transmitBuffer, w, 0x00); // command: classic CAN frame
        uint32_t now = micros();
        _appendU32LE(transmitBuffer, w, now);
        _appendU32LE(transmitBuffer, w, frame.id);
        _appendByte(transmitBuffer, w, (uint8_t)(frame.length + (uint8_t)(whichBus << 4)));
        for (int c = 0; c < frame.length; c++)
            _appendByte(transmitBuffer, w, frame.data.uint8[c]);
        temp = 0; // checksum placeholder (kept for compatibility)
        _appendByte(transmitBuffer, w, temp);

        // Commit write
        transmitBufferLength = (size_t)w;
    }
    else
    {
        // ASCII packet: "<time> - <id> <X|S> <bus> <len> <data...>\r\n"
        // Build piecewise with snprintf and guard remaining space each step
        size_t room = _roomLeft(transmitBufferLength);
        if (room == 0)
            return;

        int n = snprintf((char *)&transmitBuffer[transmitBufferLength], room, "%d - %x", micros(), frame.id);
        if (n <= 0 || (size_t)n >= room)
            return;
        transmitBufferLength += (size_t)n;

        room = _roomLeft(transmitBufferLength);
        if (room == 0)
            return;
        n = snprintf((char *)&transmitBuffer[transmitBufferLength], room, (frame.extended ? " X " : " S "));
        if (n <= 0 || (size_t)n >= room)
            return;
        transmitBufferLength += (size_t)n;

        room = _roomLeft(transmitBufferLength);
        if (room == 0)
            return;
        n = snprintf((char *)&transmitBuffer[transmitBufferLength], room, "%i %i", whichBus, frame.length);
        if (n <= 0 || (size_t)n >= room)
            return;
        transmitBufferLength += (size_t)n;

        // Append each data byte as hex
        for (int c = 0; c < frame.length; c++)
        {
            room = _roomLeft(transmitBufferLength);
            if (room == 0)
                return;
            n = snprintf((char *)&transmitBuffer[transmitBufferLength], room, " %x", frame.data.uint8[c]);
            if (n <= 0 || (size_t)n >= room)
                return;
            transmitBufferLength += (size_t)n;
        }

        // Line ending
        room = _roomLeft(transmitBufferLength);
        if (room == 0)
            return;
        n = snprintf((char *)&transmitBuffer[transmitBufferLength], room, "\r\n");
        if (n <= 0 || (size_t)n >= room)
            return;
        transmitBufferLength += (size_t)n;
    }
}

// Queue a CAN FD frame in either binary or ASCII format
void CommBuffer::sendFrameToBuffer(CAN_FRAME_FD &frame, int whichBus)
{
    uint8_t temp;
    size_t writtenBytes; // kept for parity with original structure

    if (settings.useBinarySerialComm)
    {
        // Binary FD packet: 0xF1,cmd,time(4),id(4),len(1),bus(1),data(N),checksum(1)
        // Total needed = 13 + payload
        size_t need = 13 + frame.length;
        size_t room = _roomLeft(transmitBufferLength);
        if (room < need)
        {
            // Not enough buffer space: drop this frame silently
            return;
        }

        if (frame.extended)
            frame.id |= 1u << 31;

        int w = (int)transmitBufferLength;
        _appendByte(transmitBuffer, w, 0xF1);
        _appendByte(transmitBuffer, w, PROTO_BUILD_FD_FRAME);
        uint32_t now = micros();
        _appendU32LE(transmitBuffer, w, now);
        _appendU32LE(transmitBuffer, w, frame.id);
        _appendByte(transmitBuffer, w, frame.length);
        _appendByte(transmitBuffer, w, (uint8_t)whichBus);
        for (int c = 0; c < frame.length; c++)
            _appendByte(transmitBuffer, w, frame.data.uint8[c]);
        temp = 0; // checksum placeholder (kept for compatibility)
        _appendByte(transmitBuffer, w, temp);

        // Commit write
        transmitBufferLength = (size_t)w;
    }
    else
    {
        // ASCII packet for FD: same style as classic, but length can be > 8
        size_t room = _roomLeft(transmitBufferLength);
        if (room == 0)
            return;

        int n = snprintf((char *)&transmitBuffer[transmitBufferLength], room, "%d - %x", micros(), frame.id);
        if (n <= 0 || (size_t)n >= room)
            return;
        transmitBufferLength += (size_t)n;

        room = _roomLeft(transmitBufferLength);
        if (room == 0)
            return;
        n = snprintf((char *)&transmitBuffer[transmitBufferLength], room, (frame.extended ? " X " : " S "));
        if (n <= 0 || (size_t)n >= room)
            return;
        transmitBufferLength += (size_t)n;

        room = _roomLeft(transmitBufferLength);
        if (room == 0)
            return;
        n = snprintf((char *)&transmitBuffer[transmitBufferLength], room, "%i %i", whichBus, frame.length);
        if (n <= 0 || (size_t)n >= room)
            return;
        transmitBufferLength += (size_t)n;

        // Append each data byte as hex
        for (int c = 0; c < frame.length; c++)
        {
            room = _roomLeft(transmitBufferLength);
            if (room == 0)
                return;
            n = snprintf((char *)&transmitBuffer[transmitBufferLength], room, " %x", frame.data.uint8[c]);
            if (n <= 0 || (size_t)n >= room)
                return;
            transmitBufferLength += (size_t)n;
        }

        // Line ending
        room = _roomLeft(transmitBufferLength);
        if (room == 0)
            return;
        n = snprintf((char *)&transmitBuffer[transmitBufferLength], room, "\r\n");
        if (n <= 0 || (size_t)n >= room)
            return;
        transmitBufferLength += (size_t)n;
    }
}
