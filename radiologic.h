#ifndef ACTIONS_H
#define ACTIONS_H
#include "Arduino.h"
#include <LiquidCrystal_I2C.h>

struct RadioState
{
  bool isPlaying;
  uint32_t radioIndex;
  uint8_t volumeLevel;
  uint32_t lastReceivedCode;
};

struct RadioActions
{
    static void init_system();
    static void on_off();
    static void next_station();
    static void previous_station();
    static void vol_up();
    static void vol_down();
    static void handle_web_config();
    static void handle_save_buttons();
    static void handle_save_stations();
    static void handle_save_network();
    static void reset();
    static void load_settings(); 
};

struct Display
{
  LiquidCrystal_I2C LCD2004;
  Display(uint8_t addr, uint8_t cols, uint8_t rows) : LCD2004(addr, cols, rows) {}
  void init();
  void update();
};

extern RadioState state;
extern Display radioDisplay;

#endif