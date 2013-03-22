#include "OneWire.h"

extern void DelayuS(vu32 nCount);

void OWInit(OWire* owire, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
  uint32_t PeriphClock;

  if (GPIOx == GPIOA)
  {
    PeriphClock = RCC_APB2Periph_GPIOA;
  }
  else if (GPIOx == GPIOB)
  {
    PeriphClock = RCC_APB2Periph_GPIOB;
  }
  else if (GPIOx == GPIOC)
  {
    PeriphClock = RCC_APB2Periph_GPIOC;
  }
  else if (GPIOx == GPIOD)
  {
    PeriphClock = RCC_APB2Periph_GPIOD;
  }
  else if (GPIOx == GPIOE)
  {
    PeriphClock = RCC_APB2Periph_GPIOE;
  }
  /*else if (GPIOx == GPIOF)
  {
    PeriphClock = RCC_APB2Periph_GPIOF;
  }
  else
  {
    if (GPIOx == GPIOG)
    {
      PeriphClock = RCC_APB2Periph_GPIOG;
     }
  }*/

  RCC_APB2PeriphClockCmd(PeriphClock, ENABLE);

  GPIO_InitTypeDef  GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

  GPIO_Init(GPIOx, &GPIO_InitStructure);

  uint32_t pinpos = 0x00, pos = 0x00, currentpin = 0x00;

  owire->m_BitMask = GPIO_Pin;
  owire->m_Port = GPIOx;

  uint8_t RegShift;

  if((GPIO_Pin & (uint32_t)0x00FF) > 0)
  {
    owire->m_Register = &GPIOx->CRL;

    for (pinpos = 0x00; pinpos < 0x08; pinpos++)
    {
      pos = ((uint32_t)0x01) << pinpos;
      /* Get the port pins position */
      currentpin = (uint16_t)((GPIO_Pin) & pos);
      if (currentpin == pos)
      {
        RegShift = (pinpos*4);
        owire->m_RegMask = ((uint32_t)0x0F) << (pinpos*4);
        break;
      }
     }
  }
  else
  {
    owire->m_Register = &GPIOx->CRH;

    for (pinpos = 0x00; pinpos < 0x08; pinpos++)
    {
      pos = ((uint32_t)0x01) << (pinpos + 0x08);
      /* Get the port pins position */
      currentpin = (uint16_t)((GPIO_Pin) & pos);
      if (currentpin == pos)
      {
        RegShift = (pinpos*4);
        owire->m_RegMask = ((uint32_t)0x0F) << (pinpos*4);
        break;
      }
    }
  }

  owire->m_InputMask = (((GPIO_Mode_IN_FLOATING) << RegShift) & owire->m_RegMask);
  owire->m_OutputMask = (((uint32_t)GPIO_Mode_Out_OD|(uint32_t)GPIO_Speed_50MHz) << RegShift) & owire->m_RegMask;

#ifdef ONEWIRE_SEARCH
	OWReset_search(owire);
#endif
}

void OWInput(OWire* owire)
{
  *owire->m_Register &= ~owire->m_RegMask;
  //*owire->m_Register |= (((GPIO_Mode_IN_FLOATING) << owire->m_RegShift) & owire->m_RegMask);
  *owire->m_Register |= owire->m_InputMask;
}
void OWOutput(OWire* owire)
{
  *owire->m_Register &= ~owire->m_RegMask;
  //*owire->m_Register |= ((((uint32_t)GPIO_Mode_Out_OD|(uint32_t)GPIO_Speed_50MHz) << owire->m_RegShift) & owire->m_RegMask);
  *owire->m_Register |= owire->m_OutputMask;

}

uint8_t OWReadPin(OWire* owire)
{
  return (uint8_t)((owire->m_Port->IDR & owire->m_BitMask) > 0 ? 1 : 0);
}

void OWWriteHigh(OWire* owire)
{
  owire->m_Port->BSRR = owire->m_BitMask;
}

void OWWriteLow(OWire* owire)
{
  owire->m_Port->BRR = owire->m_BitMask;
}

static void OWNoInterrupts(void)
{
  __asm("cpsid i");
}

static void OWInterrupts(void)
{
  __asm("cpsie i");
}

uint8_t OWReset(OWire* owire)
{
	uint8_t r;
	uint8_t retries = 125;

	OWNoInterrupts();
	OWInput(owire);
	OWInterrupts();
	// wait until the wire is high... just in case
	do {
		if (--retries == 0) return 0;
		DelayuS(2);//DelayuS(2);
	} while ( !OWReadPin(owire));

	OWNoInterrupts();
	OWWriteLow(owire);
	OWOutput(owire);	// drive output low
	OWInterrupts();
	DelayuS(500);
	OWNoInterrupts();
	OWInput(owire);	// allow it to float
	DelayuS(80);
	r = !OWReadPin(owire);
	OWInterrupts();
	DelayuS(420);
	return r;
}

void OWWrite_bit(OWire* owire, uint8_t v)
{
	if (v & 1) {
		OWNoInterrupts();
		OWWriteLow(owire);
		OWOutput(owire);	// drive output low
		DelayuS(10);
		OWWriteHigh(owire);	// drive output high
		OWInterrupts();
		DelayuS(55);
	} else {
		OWNoInterrupts();
		OWWriteLow(owire);
		OWOutput(owire);	// drive output low
		DelayuS(65);
		OWWriteHigh(owire);	// drive output high
		OWInterrupts();
		DelayuS(5);
	}
}

uint8_t OWRead_bit(OWire* owire)
{
	uint8_t r;

	OWNoInterrupts();
        OWWriteLow(owire);
	OWOutput(owire);
	DelayuS(3);
	OWInput(owire);	// let pin float, pull up will raise
	DelayuS(10);
	r = OWReadPin(owire);
	OWInterrupts();
	DelayuS(53);
	return r;
}

void OWWrite(OWire* owire, uint8_t v) {
    uint8_t bitMask;

    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
	OWWrite_bit(owire, (bitMask & v)?1:0);
    }

    OWNoInterrupts();
    OWInput(owire);
    OWWriteLow(owire);
    OWInterrupts();
}

void OWWrite_bytes(OWire* owire, const uint8_t *buf, uint16_t count) {
  uint16_t i;
  for (i = 0 ; i < count ; i++)
    OWWrite(owire,buf[i]);


  OWNoInterrupts();
  OWInput(owire);
  OWWriteLow(owire);
  OWInterrupts();

}

uint8_t OWRead(OWire* owire) {
    uint8_t bitMask;
    uint8_t r = 0;

    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
	if ( OWRead_bit(owire)) r |= bitMask;
    }
    return r;
}

void OWRead_bytes(OWire* owire, uint8_t *buf, uint16_t count) {
  uint16_t i;
  for (i = 0 ; i < count ; i++)
    buf[i] = OWRead(owire);
}

void OWSelect(OWire* owire, uint8_t rom[8])
{
    int i;

    OWWrite(owire, 0x55);           // Choose ROM

    for( i = 0; i < 8; i++) OWWrite(owire, rom[i]);
}

void OWSkip(OWire* owire)
{
    OWWrite(owire,0xCC);           // Skip ROM
}

void OWDepower(OWire* owire)
{
	OWNoInterrupts();
	OWInput(owire);
	OWInterrupts();
}

#ifdef ONEWIRE_SEARCH

void OWReset_search(OWire* owire)
{
  int i;
  // reset the search state
  owire->LastDiscrepancy = 0;
  owire->LastDeviceFlag = FALSE;
  owire->LastFamilyDiscrepancy = 0;
  for(i = 7; ; i--)
  {
    owire->ROM_NO[i] = 0;
    if ( i == 0) break;
  }
}

uint8_t OWSearch(OWire* owire, uint8_t *newAddr)
{
   uint8_t id_bit_number;
   uint8_t last_zero, rom_byte_number, search_result;
   uint8_t id_bit, cmp_id_bit;
   int i;
   unsigned char rom_byte_mask, search_direction;

   // initialize for search
   id_bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_result = 0;

   // if the last call was not the last one
   if (!owire->LastDeviceFlag)
   {
      // 1-Wire reset
      if (!OWReset(owire))
      {
         // reset the search
         owire->LastDiscrepancy = 0;
         owire->LastDeviceFlag = FALSE;
         owire->LastFamilyDiscrepancy = 0;
         return FALSE;
      }

      // issue the search command
      OWWrite(owire, 0xF0);

      // loop to do the search
      do
      {
         // read a bit and its complement
         id_bit = OWRead_bit(owire);
         cmp_id_bit = OWRead_bit(owire);

         // check for no devices on 1-wire
         if ((id_bit == 1) && (cmp_id_bit == 1))
            break;
         else
         {
            // all devices coupled have 0 or 1
            if (id_bit != cmp_id_bit)
               search_direction = id_bit;  // bit write value for search
            else
            {
               // if this discrepancy if before the Last Discrepancy
               // on a previous next then pick the same as last time
               if (id_bit_number < owire->LastDiscrepancy)
                  search_direction = ((owire->ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
               else
                  // if equal to last pick 1, if not then pick 0
                  search_direction = (id_bit_number == owire->LastDiscrepancy);

               // if 0 was picked then record its position in LastZero
               if (search_direction == 0)
               {
                  last_zero = id_bit_number;

                  // check for Last discrepancy in family
                  if (last_zero < 9)
                     owire->LastFamilyDiscrepancy = last_zero;
               }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1)
              owire->ROM_NO[rom_byte_number] |= rom_byte_mask;
            else
              owire->ROM_NO[rom_byte_number] &= ~rom_byte_mask;

            // serial number search direction write bit
            OWWrite_bit(owire, search_direction);

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (rom_byte_mask == 0)
            {
                rom_byte_number++;
                rom_byte_mask = 1;
            }
         }
      }
      while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

      // if the search was successful then
      if (!(id_bit_number < 65))
      {
         // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
         owire->LastDiscrepancy = last_zero;

         // check for last device
         if (owire->LastDiscrepancy == 0)
            owire->LastDeviceFlag = TRUE;

         search_result = TRUE;
      }
   }

   // if no device found then reset counters so next 'search' will be like a first
   if (!search_result || !owire->ROM_NO[0])
   {
      owire->LastDiscrepancy = 0;
      owire->LastDeviceFlag = FALSE;
      owire->LastFamilyDiscrepancy = 0;
      search_result = FALSE;
   }
   for (i = 0; i < 8; i++) newAddr[i] = owire->ROM_NO[i];
   return search_result;
  }
#endif

#ifdef ONEWIRE_CRC
// The 1-Wire CRC scheme is described in Maxim Application Note 27:
// "Understanding and Using Cyclic Redundancy Checks with Maxim iButton Products"
//

#ifdef ONEWIRE_CRC8_TABLE
// This table comes from Dallas sample code where it is freely reusable,
// though Copyright (C) 2000 Dallas Semiconductor Corporation
static const uint8_t dscrc_table[] = {
      0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
     35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
     70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
     17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
     50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
     87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

//
// Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM
// and the registers.  (note: this might better be done without to
// table, it would probably be smaller and certainly fast enough
// compared to all those delayMicrosecond() calls.  But I got
// confused, so I use this table from the examples.)
//
uint8_t OWCrc8( uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;

	while (len--) {
		crc = dscrc_table[(crc ^ *addr++)];
	}
	return crc;
}
#else
//
// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but much smaller, than the lookup table.
//
uint8_t OWCrc8( uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;
	
	while (len--) {
		uint8_t inbyte = *addr++;
		for (uint8_t i = 8; i; i--) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	return crc;
}
#endif

#ifdef ONEWIRE_CRC16
uint8_t OWCheck_crc16(uint8_t* input, uint16_t len, uint8_t* inverted_crc)
{
    uint16_t crc = ~OWCrc16(input, len);
    return (crc & 0xFF) == inverted_crc[0] && (crc >> 8) == inverted_crc[1];
}

uint16_t OWCrc16(uint8_t* input, uint16_t len)
{
    static const uint8_t oddparity[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };
    uint16_t crc = 0;    // Starting seed is zero.
    uint16_t i;
    for (i = 0 ; i < len ; i++) {
      // Even though we're just copying a byte from the input,
      // we'll be doing 16-bit computation with it.
      uint16_t cdata = input[i];
      cdata = (cdata ^ (crc & 0xff)) & 0xff;
      crc >>= 8;

      if (oddparity[cdata & 0x0F] ^ oddparity[cdata >> 4])
          crc ^= 0xC001;

      cdata <<= 6;
      crc ^= cdata;
      cdata <<= 1;
      crc ^= cdata;
    }
    return crc;
}
#endif

#endif
