#ifndef _ONEWIRE_
#define _ONEWIRE_

#include "stm32f10x.h"

#define BOOL uint8_t

#define FALSE 0
#define TRUE  1

// you can exclude onewire_search by defining that to 0
#ifndef ONEWIRE_SEARCH
#define ONEWIRE_SEARCH 1
#endif

// You can exclude CRC checks altogether by defining this to 0
#ifndef ONEWIRE_CRC
#define ONEWIRE_CRC 1
#endif

// Select the table-lookup method of computing the 8-bit CRC
// by setting this to 1.  The lookup table enlarges code size by
// about 250 bytes.  It does NOT consume RAM (but did in very
// old versions of OneWire).  If you disable this, a slower
// but very compact algorithm is used.
#ifndef ONEWIRE_CRC8_TABLE
#define ONEWIRE_CRC8_TABLE 1
#endif

// You can allow 16-bit CRC checks by defining this to 1
// (Note that ONEWIRE_CRC must also be 1.)
#ifndef ONEWIRE_CRC16
#define ONEWIRE_CRC16 1
#endif

typedef struct{
  GPIO_TypeDef*         m_Port;
  __IO uint32_t*        m_Register;
  uint32_t              m_RegMask;
  uint32_t              m_InputMask;
  uint32_t              m_OutputMask;
  uint16_t              m_BitMask;
#ifdef ONEWIRE_SEARCH
  // global search state
  unsigned char ROM_NO[8];
  uint8_t LastDiscrepancy;
  uint8_t LastFamilyDiscrepancy;
  uint8_t LastDeviceFlag;
#endif
}OWire;

void OWInit(OWire* owire, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

void OWInput(OWire* owire);
void OWOutput(OWire* owire);
uint8_t OWReadPin(OWire* owire);
void OWWriteHigh(OWire* owire);
void OWWriteLow(OWire* owire);


static void OWInterrupts(void);
static void OWNoInterrupts(void);

uint8_t OWReset(OWire* owire);

#ifdef ONEWIRE_SEARCH
void OWReset_search(OWire* owire);
uint8_t OWSearch(OWire* owire, uint8_t *newAddr);
#endif

void OWWrite_bit(OWire* owire, uint8_t v);

void OWWrite(OWire* owire, uint8_t v);

void OWWrite_bytes(OWire* owire, const uint8_t *buf, uint16_t count);

uint8_t OWRead(OWire* owire);

void OWRead_bytes(OWire* owire, uint8_t *buf, uint16_t count);

void OWSelect(OWire* owire, uint8_t rom[8]);

void OWSkip(OWire* owire);

void OWDepower(OWire* owire);

#ifdef ONEWIRE_CRC
uint8_t OWCrc8( uint8_t *addr, uint8_t len);
#ifdef ONEWIRE_CRC16
uint8_t OWCheck_crc16(uint8_t* input, uint16_t len, uint8_t* inverted_crc);
uint16_t OWCrc16(uint8_t* input, uint16_t len);
#endif

#endif

#endif
