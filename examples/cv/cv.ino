#define USE_US_TIMER    // Necessary for enabling ETSTimer's microsecond accuracy

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "sigma_delta.h"
}

#include <WifiManager.h>
#include <OSCManager.h>
#include <LEDPin.h>

/* This pointer can point at the serial port if we're developing and debugging, or
 * NULL if we're done working and want to deploy without wasting time printing */
Stream *debug = &Serial;  // Use this for development
//Stream *debug = NULL;     // Use this for deployment

/* Back-end classes that do all the heavy lifting */
LEDPin wifi_led(LED_BUILTIN, 20);     // WiFi Status and UDP/TCP I/O Indicator LED
WifiManager wifi(LED_BUILTIN, debug); // WiFi Manager
OSCManager osc(debug);                // Open Sound Control Manager

/* Sample timer; calls the audio render callback function at a specified rate */
ETSTimer sample_timer;                          // Sensor sample timer
const float sample_rate = 16000;                // Sensor sample rate (Hz)
const float sample_period = 1e6 / sample_rate;  // Sensor sample period (microseconds)

// 8-bit CV value (0-255)
uint8_t cv = 127;

// Main Setup
// ==========
void setup() {

  if (debug) 
    Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  // Set callback function for successful connection
  wifi.set_connect_handler(wifi_connected, NULL);
  
  // Initilize and connect WiFi or open access point if we fail to connect
  if (!wifi.init() || !wifi.connect()) 
      wifi.open_access_point();

  // Configure OSC Handlers
  osc.dispatch("/ping", osc_handle_ping);
  osc.dispatch("/config", osc_handle_config);
  osc.dispatch("/cv", osc_handle_cv);

  // Sigma delta setup
  sigmaDeltaEnable();
  sigmaDeltaSetup(0, 240000);   // Set up channel 0 at PWM freq. of 240,000Hz
  sigmaDeltaAttachPin(D1, 0);   // Use pin D1 on channel 0
  // Note: sigma delta on ESP8266 is limited to 8 bits (0-255)
  
  // Sensor sampling timer setup
  system_timer_reinit();
  ets_timer_setfn(&sample_timer, render, NULL);
  ets_timer_arm_new(&sample_timer, sample_period, true, 0); 
}

// Main Loop:
// ==========
void loop() {
  wifi.loop();          // Maintains WiFi connection
  if (osc.loop())       // Parses any incoming UDP packets
    wifi_led.blink();   // Blink the LED if we handled an OSC message
  wifi_led.loop();      // Turns the LED back on if we blinked it over 20ms ago
}

// CV Render Callback:
// ===================
/* This function is called by ETSTimer at our specified sample rate. We use it to
 * generate our PWM signals (usually with analogWrite() on other Arduinos, but for
 * the ESP8266 we need to use sigmaDeltaWrite() 
 */
void render(void *p_arg) {
  sigmaDeltaWrite(0, cv);   // Write CV to channel 0
}

// WiFi Connect Handler:
// =====================
/* This function is called by WifiManager when it successfully connects to a network.
 * We use it to open a UDP port with the number specified in the WiFi config settings. 
 */
void wifi_connected(void *userdata) {
  osc.open_port(wifi.get_iot_port());  
}

// OSC Handlers:
// ============
/* 
 * /ping
 *  
 * This function responds to a /ping message with this IoT device's device ID, node ID, 
 * and IP address. We also 'connect' this device's UDP client to the IP address that 
 * sent the /ping, so that any OSC messages sent from this device are sent to that 
 * address on the IoT port 
 */
void osc_handle_ping(OSCMessage &msg) {
  // Make the response message          
  OSCMessage response("/pong");
  char buff[32];
  wifi.get_dev_id(buff);
  response.add(buff);
  wifi.get_node_id(buff);
  response.add(buff);
  response.add(wifi.get_local_address().toString().c_str());
  // Set the /ping sender as the destination address
  osc.set_dest(osc.remote_addr(), wifi.get_iot_port());
  // Send the response
  osc.send(response);
}

/* 
 * /config
 *  
 * Open access point to configure network settings and device/node identifiers
 */
void osc_handle_config(OSCMessage &msg) {
  wifi.open_access_point();
}

/* Set the CV output value as the received integer or rounded float; ignore other types */
void osc_handle_cv(OSCMessage &msg) {
  if (msg.isInt(0))     
    cv = msg.getInt(0);
  else if (msg.isFloat(0))
    cv = round(msg.getFloat(0));
}