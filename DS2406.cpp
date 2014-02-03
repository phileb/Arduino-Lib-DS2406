#include <DS2406.h>

/*****************************************************************************
 * Function    : DS2406                                                      *
 *...........................................................................*
 * Description : Duration .. ms max                                          *
 *****************************************************************************/
DS2406::DS2406(OneWire *b, uint8_t *addr) 
{
    // Store one wire bus
    bus = b;

    // Store DS2406 address
    for(int i = 0; i<8; i++) 
    {
        address[i] = addr[i];
    }

    // Init outputs to "not activated"
    bPioAMemo = false;
    bPioBMemo = false;
}

/*****************************************************************************
 * Function    : init                                                        *
 *...........................................................................*
 * Description : Duration 13 ms max                                          *
 *****************************************************************************/
void DS2406::init(void)
{
    // Deactivate transistor of the 2 outputs
    // PioA and B can be used as input
    setPioOutputs(false, false);
}

/*****************************************************************************
 * Function    : setPioAOutput                                               *
 *...........................................................................*
 * Description : Duration 12 ms max                                          *
 *****************************************************************************/
int8_T DS2406::setPioAOutput(bool bPioAState)
{
    int8_T s8Result;

    // Affect PioA output by conserving PioB status
    s8Result = setPioOutputs(bPioAState, bPioBMemo);

    // Check Pio status
    if(s8Result > 0)
    {
        return ((s8Result >> 1) & 0x01);
    }
    return s8Result;
}

/*****************************************************************************
 * Function    : setPioBOutput                                               *
 *...........................................................................*
 * Description : Duration 12 ms max                                          *
 *****************************************************************************/
int8_T DS2406::setPioBOutput(bool bPioBState)
{
    int8_T s8Result;

    // Affect PioB output by conserving PioA status
    s8Result = setPioOutputs(bPioAMemo, bPioBState);

    // Check Pio status
    if(s8Result > 0)
    {
        return ((s8Result >> 1) & 0x01);
    }
    return s8Result;
}

/*****************************************************************************
 * Function    : setPioOutputs                                               *
 *...........................................................................*
 * Description : Duration 12 ms max                                          *
 *****************************************************************************/
int8_T DS2406::setPioOutputs(bool bPioAState, bool bPioBState)
{
    uint8_t   u8State;
    uint8_t   u8Buffer[4];
    uint16_T  u16Crc = 0;
    uint16_T  u16CrcCalcul = 0;
    uint8_T   u8Tmp;

    // Store the state of outputs
    bPioAMemo = bPioAState;
    bPioBMemo = bPioBState;

    // Launch writing
    u8State = ((bPioAState==false) << 5) | ((bPioBState==false) << 6) | 0xf;
    bus->reset();
    bus->select(address);
    bus->write(DS2406_WRITE_STATUS);
    bus->write(0x07);
    bus->write(0);
    bus->write(u8State);

    // Read Crc
    u16Crc = bus->read();
    u16Crc |= ((unsigned int)(bus->read())) << 8;

    // Check if communication is ok
    if(u16Crc == 0xFFFF)
    {
        return cDs2406NoCom;
    }

    // Build Buffer for Crc calculation
    u8Buffer[0] = DS2406_WRITE_STATUS;
    u8Buffer[1] = 0x07;
    u8Buffer[2] = 0x00;
    u8Buffer[3] = u8State;

    // Calculate Crc
    u16CrcCalcul = bus->crc16(u8Buffer, 4);
    u16CrcCalcul = ~u16CrcCalcul;

    // Crc comparison checking
    if(u16CrcCalcul != u16Crc)
    {
        return cDs2406WrongCrc;
    }

    // Bus Master Tx 0xFF
    bus->write(0xFF,1);

    // Bus Master Rx byte from volatile status
    u8Tmp = bus->read();

    // Successful operation
    return cDs2406Success;
}

/*****************************************************************************
 * Function    : getPioAInput                                                *
 *...........................................................................*
 * Description : Duration 11 ms max                                          *
 *****************************************************************************/
int8_T DS2406::getPioAInput(void)
{
    int8_T s8Result;

    // Request Pios Inputs
    s8Result = getPioInputs();

    // Check Pio status
    if(s8Result > 0)
    {
        return (s8Result & 0x01);
    }
    return s8Result;
}

/*****************************************************************************
 * Function    : getPioBInput                                                *
 *...........................................................................*
 * Description : Duration 11 ms max                                          *
 *****************************************************************************/
int8_T DS2406::getPioBInput(void)
{
    int8_T s8Result;

    // Request Pios Inputs
    s8Result = getPioInputs();

    // Check Pio status
    if(s8Result > 0)
    {
        return ((s8Result >> 1) & 0x01);
    }
    return s8Result;
}

/*****************************************************************************
 * Function    : getPioInputs                                                *
 *...........................................................................*
 * Description : Duration 11 ms max                                          *
 *****************************************************************************/
int8_T DS2406::getPioInputs(void)
{
    unsigned char u8InfoByte;
    unsigned char u8Bits;
    unsigned char u8Buffer[5];
    unsigned int  u16Crc = 0;
    unsigned int  u16CrcCalcul = 0;

    // Read channel info byte
    bus->reset();
    bus->select(address);
    bus->write(DS2406_ACCESS_READ);  // Command
    bus->write(0x4D);                // Control Byte 1
    bus->write(0xFF);                // Control Byte 2

    // Read Channel Info Byte
    u8InfoByte = bus->read();

    // Read Bits from PioA and PioB
    u8Bits = bus->read();

    // Read Crc
    u16Crc = bus->read();
    u16Crc |= ((unsigned int)(bus->read())) << 8;

    // Check if communication is ok
    if((u8InfoByte == 0xFF) && (u8Bits == 0xFF) &&  (u16Crc == 0xFFFF))
    {
        return cDs2406NoCom;
    }

    // Build Buffer for Crc calculation
    u8Buffer[0] = DS2406_ACCESS_READ;
    u8Buffer[1] = 0x4D;
    u8Buffer[2] = 0xFF;
    u8Buffer[3] = u8InfoByte;
    u8Buffer[4] = u8Bits;

    // Calculate Crc
    u16CrcCalcul = bus->crc16(u8Buffer, 5);
    u16CrcCalcul = ~u16CrcCalcul;

    // Crc comparison checking
    if(u16CrcCalcul != u16Crc)
    {
        return cDs2406WrongCrc;
    }

    // Check that the 4 reading are identical, else escape
    if((((u8Bits & 0xAA) != 0xAA) && ((u8Bits & 0xAA) != 0x00)) ||
       (((u8Bits & 0x55) != 0x55) && ((u8Bits & 0x55) != 0x00)))
    {
        return cDs2406NotStable;
    }

    // Reset needed to disable continuous reading
    bus->reset();
  
    // Invert logic (1 = gnd switched, 0 = not switched)
    // Then return PioA (bit 0) and PioB (bit 1)
    return ((~u8Bits) & 0x03);
}

/*****************************************************************************
 * Function    : readStatus                                                  *
 *...........................................................................*
 * Description : Duration .. ms max                                          *
 *****************************************************************************/
void DS2406::readStatus(uint8_t *buffer) 
{
    // Launch read status
    bus->reset();
    bus->select(address);
    bus->write(DS2406_READ_STATUS, 1);
    bus->write(0, 1);
    bus->write(0, 1);

    // Read all bytes
    for(int i = 0; i<10; i++) {
        buffer[i] = bus->read();
    }
    bus->reset();
}
