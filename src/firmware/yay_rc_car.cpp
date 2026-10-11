#include <SD_MMC.h>
#include "radio_hw.h"
 
static float   g_freq  = 915.0;
static float   g_bw    = 125.0;
static uint8_t g_sf    = 9;
static uint8_t g_cr    = 7;        // 4/7
static int8_t  g_power = 10;
static bool    g_sdOk  = false;
 
static bool     txPending = false;
static uint32_t txCount = 0, rxCount = 0;
 
static void logLine(const String &line) {
  if (!g_sdOk) return;
  File f = SD_MMC.open("/lora_log.csv", FILE_APPEND);
  if (f) {
    f.println(line);
    f.close();
  }
}
 
static bool initSD() {
  SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0, PIN_SD_D1, PIN_SD_D2, PIN_SD_D3);
  if (SD_MMC.begin("/sdcard", false)) {
    Serial.println("SD: mounted (4-bit)");
  } else if (SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD: mounted (1-bit)");
  } else {
    Serial.println("SD: mount failed");
    return false;
  }
  Serial.printf("SD: %.1f GB\n", SD_MMC.cardSize() / 1e9);
  if (!SD_MMC.exists("/lora_log.csv"))
    logLine("ms,dir,freq_mhz,rssi_dbm,snr_db,len,payload");
  return true;
}
 
static bool applyRadioConfig() {
  bool hf = g_freq >= 1900.0;
  if (hf && g_bw < 200.0) g_bw = 812.5;   // 2.4 GHz LoRa BW: 203.125 / 406.25 / 812.5
  if (!hf && g_bw > 500.0) g_bw = 125.0;
  if (hf && g_power > 13) g_power = 13;   // 2.4 GHz PA max ~13 dBm
 
  int st = radioConfigure(g_freq, g_bw, g_sf, g_cr, g_power);
  if (st != RADIOLIB_ERR_NONE) {
    Serial.printf("radio config failed, code %d\n", st);
    return false;
  }
  txPending = false;
  radio.startReceive();
  Serial.printf("radio: %.3f MHz  BW %.1f  SF%u  CR4/%u  %d dBm\n", g_freq, g_bw, g_sf, g_cr, g_power);
  return true;
}
 
static bool initRadio() {
  int st = radioInit(g_freq, g_bw, g_sf, g_cr, g_power);
  if (st != RADIOLIB_ERR_NONE) {
    Serial.printf("LR1121 init failed, code %d (check SPI wiring / power / VBAT_RF)\n", st);
    return false;
  }
  Serial.println("LR1121 init ok");
  return applyRadioConfig();
}
 
static void sendPacket(const String &msg) {
  txPending = true;
  radioIrq = false;
  int st = radio.startTransmit(msg.c_str());
  if (st != RADIOLIB_ERR_NONE) {
    Serial.printf("TX start failed, code %d\n", st);
    txPending = false;
    radio.startReceive();
    return;
  }
  Serial.printf("TX> %s\n", msg.c_str());
}
 
static void handleRadioEvent() {
  if (txPending) {
    radio.finishTransmit();
    txPending = false;
    txCount++;
    logLine(String(millis()) + ",tx," + String(g_freq, 3) + ",,,,");
    radio.startReceive();
    return;
  }
  String s;
  int st = radio.readData(s);
  if (st == RADIOLIB_ERR_NONE) {
    rxCount++;
    float rssi = radio.getRSSI(), snr = radio.getSNR();
    Serial.printf("RX< \"%s\"  RSSI %.1f dBm  SNR %.1f dB\n", s.c_str(), rssi, snr);
    String p = s;
    p.replace(",", ";");
    p.replace("\n", " ");
    logLine(String(millis()) + ",rx," + String(g_freq, 3) + "," + String(rssi, 1) + "," +
            String(snr, 1) + "," + String(s.length()) + "," + p);
  } else if (st == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println("RX: CRC error");
  } else {
    Serial.printf("RX failed, code %d\n", st);
  }
  radio.startReceive();
}
 
static void printHelp() {
  Serial.println(
      "Commands:\n"
      "  t <text>   (send)   transmit text\n"
      "  f <MHz>    (freq)   e.g. f 868.1  or  f 2450\n"
      "  p <dBm>    (power)  TX power\n"
      "  s <5-12>   (sf)     spreading factor\n"
      "  b <kHz>    (bw)     bandwidth\n"
      "  r <5-8>    (cr)     coding rate 4/x\n"
      "  i          (stats)  status\n"
      "  c          (dump)   print SD log\n"
      "  h          (help)");
}
 
static void handleCommand(String line) {
  line.trim();
  if (!line.length()) return;
  int sp = line.indexOf(' ');
  String cmd = (sp < 0) ? line : line.substring(0, sp);
  String arg = (sp < 0) ? String("") : line.substring(sp + 1);
  arg.trim();
  cmd.toLowerCase();
 
  if (cmd == "t" || cmd == "send")        sendPacket(arg);
  else if (cmd == "f" || cmd == "freq")   { g_freq = arg.toFloat();  applyRadioConfig(); }
  else if (cmd == "p" || cmd == "power")  { g_power = arg.toInt();   applyRadioConfig(); }
  else if (cmd == "s" || cmd == "sf")     { g_sf = arg.toInt();      applyRadioConfig(); }
  else if (cmd == "b" || cmd == "bw")     { g_bw = arg.toFloat();    applyRadioConfig(); }
  else if (cmd == "r" || cmd == "cr")     { g_cr = arg.toInt();      applyRadioConfig(); }
  else if (cmd == "i" || cmd == "stats") {
    Serial.printf("freq %.3f  bw %.1f  sf %u  cr 4/%u  pwr %d  tx %lu  rx %lu  sd %s\n",
                  g_freq, g_bw, g_sf, g_cr, g_power, (unsigned long)txCount,
                  (unsigned long)rxCount, g_sdOk ? "ok" : "none");
  } else if (cmd == "c" || cmd == "dump") {
    if (g_sdOk) {
      File f = SD_MMC.open("/lora_log.csv");
      while (f && f.available()) Serial.write(f.read());
      if (f) f.close();
    }
  } else {
    printHelp();
  }
}
 
void setup() {
  pinMode(PIN_SIG, INPUT);
  pinMode(PIN_ANT_NET, INPUT);
  pinMode(PIN_BOOT_BTN, INPUT);   // external pull-up (R4)
 
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 2000) delay(10);
  Serial.println("\nESP32-S3 + LR1121 bring-up");
 
  g_sdOk = initSD();
  if (!initRadio()) Serial.println("Radio not running.");
  printHelp();
}
 
void loop() {
  if (radioIrq) {
    radioIrq = false;
    handleRadioEvent();
  }
 
  static String buf;
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      handleCommand(buf);
      buf = "";
    } else {
      buf += ch;
    }
  }
 
  static bool last = true;
  static uint32_t lastMs = 0;
  bool now = digitalRead(PIN_BOOT_BTN);
  if (last && !now && millis() - lastMs > 300) {
    lastMs = millis();
    sendPacket("Hello from LR1121!");
  }
  last = now;
}
#include <Arduino.h>
#include <Preferences.h>
#include "radio_hw.h"
 
#define STEER_INVERT     false
#define THROTTLE_INVERT  false
#define DEADBAND_COUNTS  80      // of 4095
 
static Preferences prefs;
static Band     band = BAND_LF;
static bool     txBusy = false;
static uint8_t  seq = 0, aux = 0;
static uint32_t lastTxMs = 0, lastPrintMs = 0, txCount = 0;
static float    fSteer = 0, fThr = 0;
static int      cSteer = 2048, cThr = 2048;
static uint16_t lastSteerUs = RC_US_MID, lastThrUs = RC_US_MID;
 
static int sampleAxis(uint8_t pin) {
  uint32_t s = 0;
  for (int i = 0; i < 4; i++) s += analogRead(pin);
  return (int)(s / 4);
}
 
static uint16_t axisToUs(float raw, int center, bool invert) {
  float v = 0;
  if (raw > center + DEADBAND_COUNTS)
    v = (raw - center - DEADBAND_COUNTS) / fmaxf(1.0f, 4095.0f - center - DEADBAND_COUNTS);
  else if (raw < center - DEADBAND_COUNTS)
    v = (raw - center + DEADBAND_COUNTS) / fmaxf(1.0f, (float)(center - DEADBAND_COUNTS));
  v = constrain(v, -1.0f, 1.0f);
  if (invert) v = -v;
  return (uint16_t)constrain(RC_US_MID + (int)(v * 500.0f), RC_US_MIN, RC_US_MAX);
}
 
static void applyBand() {
  int st = radioSetBand(band);
  txBusy = false;
  Serial.printf("band: %s (%.1f MHz) %s\n", band == BAND_LF ? "sub-GHz" : "2.4 GHz",
                BANDS[band].freq, st == RADIOLIB_ERR_NONE ? "ok" : "CONFIG FAILED");
}
 
static void handleButton(uint32_t now) {
  static bool down = false;
  static uint32_t t0 = 0;
  bool pressed = digitalRead(PIN_BOOT_BTN) == LOW;
  if (pressed && !down) {
    down = true;
    t0 = now;
  } else if (!pressed && down) {
    down = false;
    uint32_t held = now - t0;
    if (held >= 1500) {
      band = (band == BAND_LF) ? BAND_HF : BAND_LF;
      prefs.putUChar("band", (uint8_t)band);
      applyBand();
    } else if (held >= 30) {
      aux ^= 1;
    }
  }
}
 
void setup() {
  pinMode(PIN_SIG, INPUT);
  pinMode(PIN_ANT_NET, INPUT);
  pinMode(PIN_BOOT_BTN, INPUT);
 
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 1000) delay(10);
  Serial.println("\nRC TX");
 
  prefs.begin("rc", false);
  band = prefs.getUChar("band", 0) ? BAND_HF : BAND_LF;
 
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_STEER_IN, ADC_11db);
  analogSetPinAttenuation(PIN_THROTTLE_IN, ADC_11db);
  uint32_t a = 0, b = 0;
  for (int i = 0; i < 32; i++) { a += analogRead(PIN_STEER_IN); b += analogRead(PIN_THROTTLE_IN); delay(2); }
  cSteer = a / 32;
  cThr = b / 32;
  fSteer = cSteer;
  fThr = cThr;
  Serial.printf("centers: steer %d  throttle %d\n", cSteer, cThr);
 
  const BandCfg &c = BANDS[band];
  int st = radioInit(c.freq, c.bw, c.sf, LINK_CR, c.power, LINK_PREAMBLE);
  if (st != RADIOLIB_ERR_NONE) {
    Serial.printf("LR1121 init failed, code %d\n", st);
    while (true) delay(1000);
  }
  applyBand();
}
 
void loop() {
  uint32_t now = millis();
 
  if (radioIrq) {
    radioIrq = false;
    if (txBusy) {
      radio.finishTransmit();
      txBusy = false;
    }
  }
 
  handleButton(now);
 
  fSteer += 0.3f * (sampleAxis(PIN_STEER_IN) - fSteer);
  fThr   += 0.3f * (sampleAxis(PIN_THROTTLE_IN) - fThr);
 
  if (now - lastTxMs >= LINK_PERIOD_MS) {
    if (txBusy && now - lastTxMs > 50) {   // TX-done IRQ never came: recover
      radio.finishTransmit();
      txBusy = false;
    }
    if (!txBusy) {
      RcPacket p;
      p.seq = seq++;
      lastSteerUs = axisToUs(fSteer, cSteer, STEER_INVERT);
      lastThrUs = axisToUs(fThr, cThr, THROTTLE_INVERT);
      p.steer = lastSteerUs;
      p.thr = lastThrUs;
      p.flags = (aux & 1) | ((band == BAND_HF) ? 2 : 0);
      rcSeal(p);
      int st = radio.startTransmit((uint8_t *)&p, sizeof(p));
      if (st == RADIOLIB_ERR_NONE) {
        txBusy = true;
        txCount++;
      } else {
        Serial.printf("TX start failed, code %d\n", st);
      }
      lastTxMs = now;
    }
  }
 
  if (now - lastPrintMs >= 1000) {
    lastPrintMs = now;
    Serial.printf("%s  steer %u  thr %u  aux %u  sent %lu\n",
                  band == BAND_LF ? "915" : "2.4G", lastSteerUs, lastThrUs, aux & 1,
                  (unsigned long)txCount);
  }
}
#define PWM_FREQ 50
#define PWM_BITS 14

static Band     band = BAND_LF;
static uint32_t lastGoodMs = 0, lastScanMs = 0, lastPrintMs = 0;
static bool     linked = false, haveSeq = false;
static uint8_t  lastSeq = 0;
static uint16_t steerUs = FAILSAFE_STEER_US, thrUs = FAILSAFE_THROTTLE_US;
static uint8_t  auxOn = 0;
static uint32_t rxGood = 0, rxBad = 0, rxLost = 0;
static float    lastRssi = 0, lastSnr = 0;

static void writeUs(uint8_t pin, uint16_t us) {
  uint32_t duty = ((uint32_t)us * (1UL << PWM_BITS)) / (1000000UL / PWM_FREQ);
  ledcWrite(pin, duty);
}

static void applyOutputs() {
  static uint16_t ws = 0, wt = 0;
  static uint8_t wa = 255;
  if (steerUs != ws) { writeUs(PIN_STEER_OUT, steerUs); ws = steerUs; }
  if (thrUs != wt)   { writeUs(PIN_THROTTLE_OUT, thrUs); wt = thrUs; }
  if (auxOn != wa)   { digitalWrite(PIN_AUX_OUT, auxOn ? HIGH : LOW); wa = auxOn; }
}

static void listen(Band b) {
  band = b;
  int st = radioSetBand(band);
  if (st != RADIOLIB_ERR_NONE) Serial.printf("band config failed, code %d\n", st);
  radio.startReceive();
}

static void handlePacket() {
  size_t n = radio.getPacketLength();
  if (n == sizeof(RcPacket)) {
    uint8_t buf[sizeof(RcPacket)];
    int st = radio.readData(buf, n);
    RcPacket p;
    memcpy(&p, buf, sizeof(p));
    if (st == RADIOLIB_ERR_NONE && rcValid(p)) {
      if (haveSeq) {
        uint8_t gap = (uint8_t)(p.seq - lastSeq - 1);
        if (gap < 128) rxLost += gap;
      }
      haveSeq = true;
      lastSeq = p.seq;
      steerUs = constrain((int)p.steer, RC_US_MIN, RC_US_MAX);
      thrUs   = constrain((int)p.thr, RC_US_MIN, RC_US_MAX);
      auxOn   = p.flags & 1;
      lastRssi = radio.getRSSI();
      lastSnr  = radio.getSNR();
      lastGoodMs = millis();
      rxGood++;
      if (!linked) {
        linked = true;
        Serial.printf("LINK UP on %s\n", band == BAND_LF ? "sub-GHz" : "2.4 GHz");
      }
    } else {
      rxBad++;
    }
  } else {
    rxBad++;
  }
  radio.startReceive();
}

void setup() {
  // Outputs first, at failsafe, so the ESC sees a valid neutral signal immediately.
  pinMode(PIN_AUX_OUT, OUTPUT);
  digitalWrite(PIN_AUX_OUT, LOW);
  ledcAttach(PIN_STEER_OUT, PWM_FREQ, PWM_BITS);
  ledcAttach(PIN_THROTTLE_OUT, PWM_FREQ, PWM_BITS);
  applyOutputs();

  pinMode(PIN_SIG, INPUT);
  pinMode(PIN_ANT_NET, INPUT);

  Serial.begin(115200);   // no wait: must boot with no USB host attached
  Serial.println("\nRC RX");

  const BandCfg &c = BANDS[band];
  int st = radioInit(c.freq, c.bw, c.sf, LINK_CR, c.power, LINK_PREAMBLE);
  if (st != RADIOLIB_ERR_NONE) {
    Serial.printf("LR1121 init failed, code %d\n", st);
    while (true) {         // outputs stay at failsafe
      delay(1000);
    }
  }
  listen(band);
}

void loop() {
  if (radioIrq) {
    radioIrq = false;
    handlePacket();
  }

  uint32_t now = millis();
  bool ok = lastGoodMs != 0 && (now - lastGoodMs) < LINK_FAILSAFE_MS;

  if (!ok) {
    if (linked) {
      linked = false;
      haveSeq = false;
      Serial.println("LINK LOST - failsafe");
    }
    steerUs = FAILSAFE_STEER_US;
    thrUs = FAILSAFE_THROTTLE_US;
    auxOn = 0;
    if (now - lastScanMs >= SCAN_DWELL_MS) {   // hunt for the TX on the other band
      lastScanMs = now;
      listen(band == BAND_LF ? BAND_HF : BAND_LF);
    }
  }
  applyOutputs();

  if (now - lastPrintMs >= 1000) {
    lastPrintMs = now;
    Serial.printf("%s %s  steer %u  thr %u  aux %u  rssi %.0f  snr %.1f  good %lu bad %lu lost %lu\n",
                  linked ? "LINK" : "----", band == BAND_LF ? "915" : "2.4G", steerUs, thrUs, auxOn,
                  lastRssi, lastSnr, (unsigned long)rxGood, (unsigned long)rxBad, (unsigned long)rxLost);
  }
}