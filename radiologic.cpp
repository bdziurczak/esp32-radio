#include "WiFiMulti.h"
#include "RadioLogic.h"
#include "Audio.h"
#include "config.h"

extern ButtonMapping remoteMap[];
extern Audio audio;
extern WiFiMulti wifiMulti;

Display radioDisplay(0x27, 20, 4);
RadioState state = {true, 0, 3, 0};
Preferences preferences;
WebServer server(80);

RadioStation stations[MAX_STATIONS];
int stationsCount = 0;

void Display::init() {
  LCD2004.init();
  LCD2004.backlight();
  LCD2004.setCursor(0, 0);
  LCD2004.print("Radio Gra");
  update();
}

void Display::update() {
  LCD2004.setCursor(0, 1);
  LCD2004.print("                    "); // Clear line
  LCD2004.setCursor(0, 1);
  if (stationsCount > 0) {
    LCD2004.print(stations[state.radioIndex].name);
  } else {
    LCD2004.print("No stations.");
  }
}

void RadioActions::load_settings() {
  preferences.begin("net", true);
  String ssid = preferences.getString("ssid", default_ssid);
  String pass = preferences.getString("pass", default_password);
  

  bool staticIP = preferences.getChar("static", 0) == 1;
  
  if (staticIP) {
    IPAddress local_IP, gateway, subnet;
    local_IP.fromString(preferences.getString("local_ip", "192.168.1.200"));
    gateway.fromString(preferences.getString("gateway", "192.168.1.1"));
    subnet.fromString(preferences.getString("subnet", "255.255.255.0"));
    WiFi.config(local_IP, gateway, subnet);
  }
  preferences.end();

  wifiMulti.addAP(ssid.c_str(), pass.c_str());

  Serial.print("Connecting WiFi");
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected. IP: " + WiFi.localIP().toString());


  preferences.begin("buttons", true);
  remoteMap[0].code = preferences.getUInt("btn0", Buttons::ON_OFF);
  remoteMap[1].code = preferences.getUInt("btn1", Buttons::RARRW_3);
  remoteMap[2].code = preferences.getUInt("btn2", Buttons::LARRW_3);
  remoteMap[3].code = preferences.getUInt("btn3", Buttons::UPARRW);
  remoteMap[4].code = preferences.getUInt("btn4", Buttons::DWNARRW);
  remoteMap[5].code = preferences.getUInt("btn5", Buttons::YELLOW_CIRCARRW);
  preferences.end();

  preferences.begin("stations", true);
  stationsCount = preferences.getInt("count", 0);
  
  if (stationsCount == 0) { 
    preferences.end();
    preferences.begin("stations", false);
    struct Temp { const char* u; const char* n; } defaults[] = {
      {"http://www.emsoft.ct8.pl/inne/time.m3u?id=5990", "ESKA ROCK"},
      {"http://stream3.polskieradio.pl:8904/listen.pls", "Trojka"},
      {"http://stream3.nadaje.com:9116/radiokrakow.m3u", "Radio Krakow"}
    };
    stationsCount = 3;
    preferences.putInt("count", stationsCount);
    for(int i = 0; i < stationsCount; i++) {
      String uKey = "u" + String(i); String nKey = "n" + String(i);
      preferences.putString(uKey.c_str(), defaults[i].u);
      preferences.putString(nKey.c_str(), defaults[i].n);
    }
  }

  for (int i = 0; i < stationsCount; i++) {
    String uKey = "u" + String(i); String nKey = "n" + String(i);
    strncpy(stations[i].url, preferences.getString(uKey.c_str(), "").c_str(), 127);
    strncpy(stations[i].name, preferences.getString(nKey.c_str(), "").c_str(), 31);
  }
  preferences.end();
}

void RadioActions::init_system() {
  load_settings();

  server.on("/", HTTP_GET, handle_web_config);
  server.on("/save_buttons", HTTP_POST, handle_save_buttons);
  server.on("/save_stations", HTTP_POST, handle_save_stations);
  server.on("/save_network", HTTP_POST, handle_save_network);
  server.begin();

  audio.setPinout(DAC_BCLK, DAC_LRC, DAC_DOUT);
  audio.setVolume(state.volumeLevel);
  if (stationsCount > 0) {
    audio.connecttohost(stations[state.radioIndex].url);
  }
  radioDisplay.init();
}

void RadioActions::handle_web_config() {
  String html = "<html><head><meta charset='UTF-8'><style>body{font-family:sans-serif; background:#f4f4f4; margin:20px;} .card{background:white; padding:20px; margin-bottom:20px; border-radius:8px; box-shadow:0 2px 5px rgba(0,0,0,0.1);}</style></head><body>";
  html += "<h1>ESP32 Radio Dashboard</h1>";

  // Card 1: Network Config
  preferences.begin("net", true);
  html += "<div class='card'><h2>Network Settings</h2><form action='/save_network' method='POST'>";
  html += "SSID: <input type='text' name='ssid' value='" + preferences.getString("ssid", default_ssid) + "'><br><br>";
  html += "Password: <input type='password' name='pass' value='" + preferences.getString("pass", default_password) + "'><br><br>";
  bool isStatic = preferences.getChar("static", 0) == 1;
  html += "<input type='checkbox' name='static' value='1'" + String(isStatic ? " checked" : "") + "> Use Static IP?<br><br>";
  html += "Static IP: <input type='text' name='ip' value='" + preferences.getString("local_ip", "192.168.1.200") + "'><br><br>";
  html += "Gateway: <input type='text' name='gateway' value='" + preferences.getString("gateway", "192.168.1.1") + "'><br><br>";
  html += "Subnet: <input type='text' name='subnet' value='" + preferences.getString("subnet", "255.255.255.0") + "'><br><br>";
  html += "<input type='submit' value='Save Network Settings'>";
  html += "</form></div>";
  preferences.end();

  html += "<div class='card'><h2>Radio Stations Management</h2><form action='/save_stations' method='POST'>";
  html += "<table border='1' cellpadding='5' style='width:100%; border-collapse:collapse;'><tr><th>Name</th><th>Stream URL</th><th>Delete?</th></tr>";
  for (int i = 0; i < stationsCount; i++) {
    html += "<tr>";
    html += "<td><input type='text' name='name" + String(i) + "' value='" + String(stations[i].name) + "'></td>";
    html += "<td><input type='text' name='url" + String(i) + "' value='" + String(stations[i].url) + "' style='width:90%;'></td>";
    html += "<td><input type='checkbox' name='del" + String(i) + "' value='1'> Remove</td>";
    html += "</tr>";
  }
  html += "</table><br>";
  if(stationsCount < MAX_STATIONS) {
    html += "<h3>Add New Station</h3>";
    html += "Name: <input type='text' name='newname'> URL: <input type='text' name='newurl' style='width:50%;'><br><br>";
  }
  html += "<input type='submit' value='Apply Station Updates'>";
  html += "</form></div>";

  html += "<div class='card'><h2>Remote Control Setup</h2>";
  html += "<p>Last Captured RAW IR Hex: <code style='font-size:18px; color:blue;'>0x" + String(state.lastReceivedCode, HEX) + "</code></p>";
  html += "<form action='/save_buttons' method='POST'><table border='1' cellpadding='5'>";
  const char* labels[] = {"On/Off", "Next Station", "Prev Station", "Volume Up", "Volume Down", "Reset Defaults"};
  for (int i = 0; i < 6; i++) {
    html += "<tr><td>" + String(labels[i]) + "</td><td><input type='text' name='code" + String(i) + "' value='0x" + String(remoteMap[i].code, HEX) + "'></td></tr>";
  }
  html += "</table><br><input type='submit' value='Save Keys'></form></div>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void RadioActions::handle_save_network() {
  preferences.begin("net", false);
  preferences.putString("ssid", server.arg("ssid"));
  preferences.putString("pass", server.arg("pass"));
  
  preferences.putChar("static", server.hasArg("static") ? 1 : 0); 
  
  preferences.putString("local_ip", server.arg("ip"));
  preferences.putString("gateway", server.arg("gateway"));
  preferences.putString("subnet", server.arg("subnet"));
  preferences.end();

  server.send(200, "text/html", "<h1>Network Saved! Rebooting...</h1>");
  delay(2000);
  ESP.restart();
}

void RadioActions::handle_save_stations() {
  preferences.begin("stations", false);
  int newCount = 0;
  RadioStation tempStations[MAX_STATIONS];

  for (int i = 0; i < stationsCount; i++) {
    if (server.hasArg("del" + String(i))) continue; 

    String nameVal = server.arg("name" + String(i));
    String urlVal = server.arg("url" + String(i));
    
    if(nameVal.length() > 0 && urlVal.length() > 0) {
      strncpy(tempStations[newCount].name, nameVal.c_str(), 31);
      tempStations[newCount].name[31] = '\0';
      strncpy(tempStations[newCount].url, urlVal.c_str(), 127);
      tempStations[newCount].url[127] = '\0';
      newCount++;
    }
  }

  if (server.hasArg("newname") && server.arg("newname").length() > 0) {
    if (newCount < MAX_STATIONS) {
      strncpy(tempStations[newCount].name, server.arg("newname").c_str(), 31);
      tempStations[newCount].name[31] = '\0';
      strncpy(tempStations[newCount].url, server.arg("newurl").c_str(), 127);
      tempStations[newCount].url[127] = '\0';
      newCount++;
    }
  }

  preferences.putInt("count", newCount);
  for (int i = 0; i < newCount; i++) {
    preferences.putString(("u" + String(i)).c_str(), tempStations[i].url);
    preferences.putString(("n" + String(i)).c_str(), tempStations[i].name);
  }
  preferences.end();

  server.send(200, "text/html", "<h1>Stations Updated!</h1><script>setTimeout(function(){window.location.href='/'}, 1500);</script>");
  
  stationsCount = newCount;
  for (int i = 0; i < stationsCount; i++) {
     strncpy(stations[i].url, tempStations[i].url, 127);
     strncpy(stations[i].name, tempStations[i].name, 31);
  }
  state.radioIndex = 0; 
  radioDisplay.update();
}

void RadioActions::handle_save_buttons() {
  preferences.begin("buttons", false);
  for (int i = 0; i < 6; i++) {
    String argName = "code" + String(i);
    if (server.hasArg(argName)) {
      uint32_t code = strtoul(server.arg(argName).c_str(), NULL, 16);
      remoteMap[i].code = code;
      preferences.putUInt(("btn" + String(i)).c_str(), code);
    }
  }
  preferences.end();
  server.send(200, "text/html", "<h1>Buttons Mapped!</h1><script>setTimeout(function(){window.location.href='/'}, 1000);</script>");
}

void RadioActions::on_off() {
  if (stationsCount == 0) return;
  if (state.isPlaying) {
    audio.stopSong(); 
    state.isPlaying = false;
    radioDisplay.LCD2004.setCursor(0, 0);
    radioDisplay.LCD2004.print("Stop       ");
  } else {
    audio.connecttohost(stations[state.radioIndex].url);
    state.isPlaying = true;
    radioDisplay.LCD2004.setCursor(0, 0);
    radioDisplay.LCD2004.print("Radio Gra");
  }
  radioDisplay.update();
}

void RadioActions::next_station() {
  if (stationsCount == 0) return;
  state.radioIndex = (state.radioIndex + 1) % stationsCount;
  audio.connecttohost(stations[state.radioIndex].url);
  radioDisplay.update();
}

void RadioActions::previous_station() {
  if (stationsCount == 0) return;
  state.radioIndex = (state.radioIndex - 1 + stationsCount) % stationsCount;
  audio.connecttohost(stations[state.radioIndex].url);
  radioDisplay.update();
}

void RadioActions::vol_up() { if (state.volumeLevel < 21) state.volumeLevel++; audio.setVolume(state.volumeLevel); }
void RadioActions::vol_down() { if (state.volumeLevel > 0) state.volumeLevel--; audio.setVolume(state.volumeLevel); }

void RadioActions::reset() {
  preferences.begin("buttons", false); preferences.clear(); preferences.end();
  preferences.begin("stations", false); preferences.clear(); preferences.end();
  preferences.begin("net", false); preferences.clear(); preferences.end();
  delay(500);
  ESP.restart();
}
