#include "gvret_comm.h"
#include "config.h"
#include "can_manager.h"

GVRET_Comm_Handler::GVRET_Comm_Handler()
{
    step = 0;
    state = IDLE;
}

void GVRET_Comm_Handler::processIncomingByte(uint8_t in_byte)
{
    uint32_t busSpeed = 0;
    uint32_t now = micros(); // used for timestamp responses

    uint8_t temp8;
    uint16_t temp16;

    switch (state)
    {
    case IDLE:
        // Waiting for start of command
        if (in_byte == 0xF1)
        {
            state = GET_COMMAND; // command packet start
        }
        else if (in_byte == 0xE7)
        {
            // Switch to binary serial comm
            settings.useBinarySerialComm = true;
        }
        else
        {
            // Ignore unrelated data in idle state
        }
        break;

    case GET_COMMAND:
        // Command type byte
        switch (in_byte)
        {
        case PROTO_BUILD_CAN_FRAME:
            state = BUILD_CAN_FRAME; // next bytes will define a CAN frame
            buff[0] = 0xF1;
            step = 0;
            break;

        case PROTO_TIME_SYNC:
            // Send current microsecond counter
            state = TIME_SYNC;
            step = 0;
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 1;
            transmitBuffer[transmitBufferLength++] = (uint8_t)(now & 0xFF);
            transmitBuffer[transmitBufferLength++] = (uint8_t)(now >> 8);
            transmitBuffer[transmitBufferLength++] = (uint8_t)(now >> 16);
            transmitBuffer[transmitBufferLength++] = (uint8_t)(now >> 24);
            break;

        case PROTO_DIG_INPUTS:
            // Return placeholder for digital inputs
            temp8 = 0;
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 2;
            transmitBuffer[transmitBufferLength++] = temp8;
            temp8 = checksumCalc(buff, 2);
            transmitBuffer[transmitBufferLength++] = temp8;
            state = IDLE;
            break;

        case PROTO_ANA_INPUTS:
            // Return placeholder for analog inputs (all zero)
            temp16 = 0;
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 3;
            for (int k = 0; k < 7; k++)
            {
                transmitBuffer[transmitBufferLength++] = (uint8_t)(temp16 & 0xFF);
                transmitBuffer[transmitBufferLength++] = (uint8_t)(temp16 >> 8);
            }
            temp8 = checksumCalc(buff, 9);
            transmitBuffer[transmitBufferLength++] = temp8;
            state = IDLE;
            break;

        case PROTO_SET_DIG_OUT:
            state = SET_DIG_OUTPUTS; // prepare to receive digital output state
            buff[0] = 0xF1;
            break;

        case PROTO_SETUP_CANBUS:
            state = SETUP_CANBUS; // prepare to receive CAN bus setup parameters
            step = 0;
            buff[0] = 0xF1;
            break;

        case PROTO_GET_CANBUS_PARAMS:
            // Send parameters for CAN0 and CAN1
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 6;
            // Bus 0
            transmitBuffer[transmitBufferLength++] = settings.canSettings[0].enabled +
                ((unsigned char)settings.canSettings[0].listenOnly << 4);
            transmitBuffer[transmitBufferLength++] = settings.canSettings[0].nomSpeed;
            transmitBuffer[transmitBufferLength++] = settings.canSettings[0].nomSpeed >> 8;
            transmitBuffer[transmitBufferLength++] = settings.canSettings[0].nomSpeed >> 16;
            transmitBuffer[transmitBufferLength++] = settings.canSettings[0].nomSpeed >> 24;
            // Bus 1
            transmitBuffer[transmitBufferLength++] = settings.canSettings[1].enabled +
                ((unsigned char)settings.canSettings[1].listenOnly << 4);
            transmitBuffer[transmitBufferLength++] = settings.canSettings[1].nomSpeed;
            transmitBuffer[transmitBufferLength++] = settings.canSettings[1].nomSpeed >> 8;
            transmitBuffer[transmitBufferLength++] = settings.canSettings[1].nomSpeed >> 16;
            transmitBuffer[transmitBufferLength++] = settings.canSettings[1].nomSpeed >> 24;
            state = IDLE;
            break;

        case PROTO_GET_DEV_INFO:
            // Send firmware build info
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 7;
            transmitBuffer[transmitBufferLength++] = CFG_BUILD_NUM & 0xFF;
            transmitBuffer[transmitBufferLength++] = (CFG_BUILD_NUM >> 8);
            transmitBuffer[transmitBufferLength++] = 0x20;
            transmitBuffer[transmitBufferLength++] = 0;
            transmitBuffer[transmitBufferLength++] = 0;
            transmitBuffer[transmitBufferLength++] = 0;
            state = IDLE;
            break;

        case PROTO_SET_SW_MODE:
            state = SET_SINGLEWIRE_MODE;
            buff[0] = 0xF1;
            step = 0;
            break;

        case PROTO_KEEPALIVE:
            // Respond with keepalive ack
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 0x09;
            transmitBuffer[transmitBufferLength++] = 0xDE;
            transmitBuffer[transmitBufferLength++] = 0xAD;
            state = IDLE;
            break;

        case PROTO_SET_SYSTYPE:
            state = SET_SYSTYPE;
            buff[0] = 0xF1;
            step = 0;
            break;

        case PROTO_ECHO_CAN_FRAME:
            // Prepare to echo CAN frame back to sender
            state = ECHO_CAN_FRAME;
            buff[0] = 0xF1;
            step = 0;
            break;

        case PROTO_GET_NUMBUSES:
            // Send number of CAN buses available
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 12;
            transmitBuffer[transmitBufferLength++] = SysSettings.numBuses;
            state = IDLE;
            break;

        case PROTO_GET_EXT_BUSES:
            // Extended bus info (placeholder)
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 13;
            for (int u = 2; u < 17; u++)
                transmitBuffer[transmitBufferLength++] = 0;
            state = IDLE;
            break;

        case PROTO_SET_EXT_BUSES:
            // Prepare to receive extended bus settings (SWCAN, LIN)
            state = SETUP_EXT_BUSES;
            step = 0;
            buff[0] = 0xF1;
            break;
        }
        break;

    case BUILD_CAN_FRAME:
        // Build a CAN frame from incoming bytes
        buff[1 + step] = in_byte;
        switch (step)
        {
        case 0: build_out_frame.id = in_byte; break;
        case 1: build_out_frame.id |= in_byte << 8; break;
        case 2: build_out_frame.id |= in_byte << 16; break;
        case 3:
            build_out_frame.id |= in_byte << 24;
            if (build_out_frame.id & (1 << 31))
            {
                build_out_frame.id &= 0x7FFFFFFF;
                build_out_frame.extended = true;
            }
            else build_out_frame.extended = false;
            break;
        case 4: out_bus = in_byte & 3; break;
        case 5:
            build_out_frame.length = in_byte & 0xF;
            if (build_out_frame.length > 8)
                build_out_frame.length = 8;
            break;
        default:
            if (step < build_out_frame.length + 6)
            {
                build_out_frame.data.uint8[step - 6] = in_byte;
            }
            else
            {
                state = IDLE;
                build_out_frame.rtr = 0;
                if (out_bus < NUM_BUSES)
                    canManager.sendFrame(canBuses[out_bus], build_out_frame);
            }
            break;
        }
        step++;
        break;

    case TIME_SYNC:
    case GET_DIG_INPUTS:
    case GET_ANALOG_INPUTS:
        // No extra bytes expected — ignore
        state = IDLE;
        break;

    case SET_DIG_OUTPUTS:
        // Accept digital output state byte (no-op if hardware not present)
        buff[1] = in_byte;
        state = IDLE;
        break;

    case SETUP_CANBUS:
        // Receive and apply CAN bus speed and mode settings for bus 0 and 1
        switch (step)
        {
        case 0: build_int = in_byte; break;
        case 1: build_int |= in_byte << 8; break;
        case 2: build_int |= in_byte << 16; break;
        case 3:
            build_int |= in_byte << 24;
            busSpeed = build_int & 0xFFFFF;
            if (busSpeed > 1000000) busSpeed = 1000000;

            if (build_int > 0)
            {
                if (build_int & 0x80000000ul)
                {
                    settings.canSettings[0].enabled = build_int & 0x40000000ul;
                    settings.canSettings[0].listenOnly = build_int & 0x20000000ul;
                }
                else settings.canSettings[0].enabled = true;
                settings.canSettings[0].nomSpeed = busSpeed;
            }
            else settings.canSettings[0].enabled = false;

            if (settings.canSettings[0].enabled)
            {
                canBuses[0]->begin(settings.canSettings[0].nomSpeed, 255);
                canBuses[0]->setListenOnlyMode(settings.canSettings[0].listenOnly);
                canBuses[0]->watchFor();
            }
            else canBuses[0]->disable();
            break;

        case 4: build_int = in_byte; break;
        case 5: build_int |= in_byte << 8; break;
        case 6: build_int |= in_byte << 16; break;
        case 7:
            build_int |= in_byte << 24;
            busSpeed = build_int & 0xFFFFF;
            if (busSpeed > 1000000) busSpeed = 1000000;

            if (build_int > 0 && SysSettings.numBuses > 1)
            {
                if (build_int & 0x80000000ul)
                {
                    settings.canSettings[1].enabled = build_int & 0x40000000ul;
                    settings.canSettings[1].listenOnly = build_int & 0x20000000ul;
                }
                else settings.canSettings[1].enabled = true;
                settings.canSettings[1].nomSpeed = busSpeed;
            }
            else settings.canSettings[1].enabled = false;

            if (settings.canSettings[1].enabled)
            {
                canBuses[1]->begin(settings.canSettings[1].nomSpeed, 255);
                canBuses[1]->setListenOnlyMode(settings.canSettings[1].listenOnly);
                canBuses[1]->watchFor();
            }
            else canBuses[1]->disable();

            state = IDLE;
            break;
        }
        step++;
        break;

    case SET_SINGLEWIRE_MODE:
        // Not implemented — just accept byte
        state = IDLE;
        break;

    case SET_SYSTYPE:
        // Update system type setting
        settings.systemType = in_byte;
        state = IDLE;
        break;

    case ECHO_CAN_FRAME:
        // Echo back a CAN frame without sending to bus
        buff[1 + step] = in_byte;
        switch (step)
        {
        case 0: build_out_frame.id = in_byte; break;
        case 1: build_out_frame.id |= in_byte << 8; break;
        case 2: build_out_frame.id |= in_byte << 16; break;
        case 3:
            build_out_frame.id |= in_byte << 24;
            if (build_out_frame.id & (1 << 31))
            {
                build_out_frame.id &= 0x7FFFFFFF;
                build_out_frame.extended = true;
            }
            else build_out_frame.extended = false;
            break;
        case 4: out_bus = in_byte & 1; break;
        case 5:
            build_out_frame.length = in_byte & 0xF;
            if (build_out_frame.length > 8) build_out_frame.length = 8;
            break;
        default:
            if (step < build_out_frame.length + 6)
            {
                build_out_frame.data.bytes[step - 6] = in_byte;
            }
            else
            {
                state = IDLE;
                canManager.displayFrame(build_out_frame, 0);
            }
            break;
        }
        step++;
        break;

    case SETUP_EXT_BUSES:
        // Accept extended bus setup bytes (currently unused)
        switch (step)
        {
        case 0: build_int = in_byte; break;
        case 1: build_int |= in_byte << 8; break;
        case 2: build_int |= in_byte << 16; break;
        case 3: build_int |= in_byte << 24; break;
        case 4: build_int = in_byte; break;
        case 5: build_int |= in_byte << 8; break;
        case 6: build_int |= in_byte << 16; break;
        case 7: build_int |= in_byte << 24; break;
        case 8: build_int = in_byte; break;
        case 9: build_int |= in_byte << 8; break;
        case 10: build_int |= in_byte << 16; break;
        case 11: build_int |= in_byte << 24; state = IDLE; break;
        }
        step++;
        break;
    }
}

// Simple XOR checksum
uint8_t GVRET_Comm_Handler::checksumCalc(uint8_t *buffer, int length)
{
    uint8_t valu = 0;
    for (int c = 0; c < length; c++)
        valu ^= buffer[c];
    return valu;
}
