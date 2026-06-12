#ifndef CONFIG_H
#define CONFIG_H
#include <Preferences.h>
#include <WebServer.h>

#define DAC_DOUT 2
#define DAC_BCLK 4
#define DAC_LRC 15
#define IR_SIGNAL_PIN 34
#define LED2004_SCL_PIN 22
#define LED2004_DA_PIN 24

#define MAX_STATIONS 15

struct Buttons {
  static constexpr uint32_t ON_OFF = 0xF7C03F;
  static constexpr uint32_t UPARRW = 0xF700FF;
  static constexpr uint32_t DWNARRW = 0xF7807F;
  static constexpr uint32_t LARRW_3 = 0xF78877;
  static constexpr uint32_t RARRW_3 = 0xF748B7;
  static constexpr uint32_t YELLOW_CIRCARRW = 0xF7D02F;
};

struct ButtonMapping {
  uint32_t code;
  void (*action)();
};

// Fix: Redefine fields explicitly as mutable character arrays
struct RadioStation {
  char url[128];
  char name[32];
};

// Share with other compilation units
extern RadioStation stations[MAX_STATIONS];
extern int stationsCount;

inline const char* default_ssid = "A50159B";
inline const char* default_password = "llxq2409";

#endif