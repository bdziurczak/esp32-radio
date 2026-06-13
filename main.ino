#include <IRrecv.h>
#include <Wire.h>
#include "Arduino.h"
#include "WiFiMulti.h"
#include "Audio.h"
#include "config.h"
#include "RadioLogic.h"

extern WebServer server;
Audio audio;
WiFiMulti wifiMulti;
IRrecv irrecv(IR_SIGNAL_PIN);
decode_results IR_results;

ButtonMapping remoteMap[] = 
{
    { Buttons::ON_OFF, RadioActions::on_off },
    { Buttons::RARRW_3,   RadioActions::next_station },
    { Buttons::LARRW_3,   RadioActions::previous_station },
    { Buttons::UPARRW,     RadioActions::vol_up },
    { Buttons::DWNARRW,   RadioActions::vol_down },
    { Buttons::YELLOW_CIRCARRW,   RadioActions::reset }
};

void setup() 
{
  Serial.begin(115200);
  pinMode(IR_SIGNAL_PIN, INPUT_PULLUP);
  irrecv.enableIRIn();
  RadioActions::init_system();
}

void loop()
{
  server.handleClient();
  audio.loop();

  if (irrecv.decode(&IR_results)) 
  {
    uint32_t val = IR_results.value;
    
    if (val > 0x100000 && val != 0xFFFFFFFF) 
    {
      state.lastReceivedCode = val;
      Serial.print("Valid Code Captured: 0x");
      Serial.println(val, HEX);

      for (const auto& entry : remoteMap)
      {
          if (val == entry.code)
          {
              entry.action();
              break;
          }
      }  
    }
    else if (val != 0) 
    {
      // Optional: uncomment to see what noise looks like, then comment out
      // Serial.print("Filtered Noise: 0x"); Serial.println(val, HEX);
    }

    irrecv.resume();
  } 
}
