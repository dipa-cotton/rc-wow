#pragma once
// LR1121 hardware layer shared by bringup / TX / RX
#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "pins.h"
#include "rc_link.h"

static const uint32_t rfswitch_pins[] = {
    RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6, RADIOLIB_LR11X0_DIO7,
    RADIOLIB_NC, RADIOLIB_NC};

static const Module::RfSwitchMode_t rfswitch_table[] = {
    //                         U5    U7    U8
    {LR11x0::MODE_STBY,  {LOW,  LOW,  LOW,  LOW, LOW}},
    {LR11x0::MODE_RX,    {LOW,  LOW,  HIGH, LOW, LOW}},
    {LR11x0::MODE_TX,    {HIGH, LOW,  HIGH, LOW, LOW}},
    {LR11x0::MODE_TX_HP, {LOW,  HIGH, LOW,  LOW, LOW}},
    {LR11x0::MODE_TX_HF, {LOW,  LOW,  LOW,  LOW, LOW}},
    {LR11x0::MODE_GNSS,  {LOW,  LOW,  HIGH, LOW, LOW}},
    {LR11x0::MODE_WIFI,  {LOW,  LOW,  HIGH, LOW, LOW}},
    END_OF_MODE_TABLE,
};

static Module lrModule(PIN_LR_NSS, PIN_LR_DIO1, PIN_LR_RESET, PIN_LR_BUSY);
static LR1121 radio(&lrModule);

static volatile bool radioIrq = false;
static void IRAM_ATTR onRadioIrq() { radioIrq = true; }

static int radioInit(float freq, float bw, uint8_t sf, uint8_t cr, int8_t pwr,
                     uint16_t preamble = 8) {
  SPI.begin(PIN_LR_SCK, PIN_LR_MISO, PIN_LR_MOSI);
  radio.setRfSwitchTable(rfswitch_pins, rfswitch_table);
  int st = radio.begin(freq, bw, sf, cr, 0x12 /* private sync word */, pwr, preamble, 0.0f, false);
  if (st != RADIOLIB_ERR_NONE) return st;
  radio.setIrqAction(onRadioIrq);
  return RADIOLIB_ERR_NONE;
}

static int radioConfigure(float freq, float bw, uint8_t sf, uint8_t cr, int8_t pwr) {
  radio.standby();
  int st = radio.setFrequency(freq);
  if (st == RADIOLIB_ERR_NONE) st = radio.setBandwidth(bw);
  if (st == RADIOLIB_ERR_NONE) st = radio.setSpreadingFactor(sf);
  if (st == RADIOLIB_ERR_NONE) st = radio.setCodingRate(cr);
  if (st == RADIOLIB_ERR_NONE) st = radio.setOutputPower(pwr);
  radioIrq = false;
  return st;
}

static int radioSetBand(Band b) {
  const BandCfg &c = BANDS[b];
  return radioConfigure(c.freq, c.bw, c.sf, LINK_CR, c.power);
}



#pragma once
#include <stdint.h>
#include <stddef.h>

// ---- link settings ----
#define LINK_KEY          0x4C52C0DEUL  // change this per car/transmitter pair
#define LINK_PERIOD_MS    20            // packet rate (20 ms = 50 Hz)
#define LINK_FAILSAFE_MS  300           // no valid packet this long -> failsafe
#define LINK_CR           5             // LoRa coding rate 4/5 (shortest airtime)
#define LINK_PREAMBLE     8
#define SCAN_DWELL_MS     150           // RX band-scan dwell while link is lost

#define RC_US_MIN   1000
#define RC_US_MID   1500
#define RC_US_MAX   2000
#define FAILSAFE_STEER_US    RC_US_MID
#define FAILSAFE_THROTTLE_US RC_US_MID  // neutral for a forward/reverse ESC

// ---- radio profiles ----
#ifndef LF_POWER_DBM
#define LF_POWER_DBM 10
#endif
#ifndef HF_POWER_DBM
#define HF_POWER_DBM 10          // 2.4 GHz PA max is ~13 dBm
#endif
#define LF_FREQ_MHZ 915.0f
#define LF_BW_KHZ   500.0f
#define LF_SF       6
#define HF_FREQ_MHZ 2450.0f
#define HF_BW_KHZ   812.5f
#define HF_SF       5

enum Band : uint8_t { BAND_LF = 0, BAND_HF = 1 };
struct BandCfg { float freq; float bw; uint8_t sf; int8_t power; };
static const BandCfg BANDS[2] = {
  { LF_FREQ_MHZ, LF_BW_KHZ, LF_SF, LF_POWER_DBM },
  { HF_FREQ_MHZ, HF_BW_KHZ, HF_SF, HF_POWER_DBM },
};

// ---- packet (8 bytes) ----
struct __attribute__((packed)) RcPacket {
  uint8_t  id;      // derived from LINK_KEY
  uint8_t  seq;     // increments every packet (RX counts gaps)
  uint16_t steer;   // 1000..2000 us
  uint16_t thr;     // 1000..2000 us
  uint8_t  flags;   // bit0 = aux, bit1 = TX is on 2.4 GHz
  uint8_t  crc;     // CRC-8 seeded from LINK_KEY
};
static_assert(sizeof(RcPacket) == 8, "RcPacket must be 8 bytes");

static inline uint8_t rcCrc8(const uint8_t *d, size_t n, uint8_t crc) {
  while (n--) {
    crc ^= *d++;
    for (uint8_t i = 0; i < 8; i++) crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
  }
  return crc;
}
static inline uint8_t rcLinkId()   { return (uint8_t)((LINK_KEY >> 24) ^ (LINK_KEY >> 8)); }
static inline uint8_t rcCrcSeed()  { return (uint8_t)((LINK_KEY >> 16) ^ LINK_KEY); }
static inline void rcSeal(RcPacket &p) {
  p.id = rcLinkId();
  p.crc = rcCrc8((const uint8_t *)&p, sizeof(p) - 1, rcCrcSeed());
}
static inline bool rcValid(const RcPacket &p) {
  return p.id == rcLinkId() &&
         p.crc == rcCrc8((const uint8_t *)&p, sizeof(p) - 1, rcCrcSeed());
}

#pragma once

// ---- LR1121 (SPI) ----
#define PIN_LR_NSS    8
#define PIN_LR_SCK    5
#define PIN_LR_MOSI   6
#define PIN_LR_MISO   7
#define PIN_LR_RESET  3
#define PIN_LR_BUSY   4
#define PIN_LR_DIO1   9   // IRQ

// ---- microSD (SDMMC 4-bit, external 10k pull-ups) ----
#define PIN_SD_CLK    21
#define PIN_SD_CMD    48
#define PIN_SD_D0     17
#define PIN_SD_D1     18
#define PIN_SD_D2     46  // strapping pin
#define PIN_SD_D3     47

#define PIN_CS        39
#define PIN_INT       38
#define PIN_ADD       40
#define PIN_SDA       41
#define PIN_SCL       42
#define PIN_SIG       45  // strapping pin: keep input at boot
#define PIN_ANT_NET   10  // ANT

#define PIN_BOOT_BTN  0   // SW2
