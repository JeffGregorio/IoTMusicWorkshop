#define USE_US_TIMER    // Necessary for enabling ETSTimer's microsecond accuracy

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "sigma_delta.h"
}

#include <WifiManager.h>
#include <OSCManager.h>
#include <LEDPin.h>
#include <Envelope.h>

/* This pointer can point at the serial port if we're developing and debugging, or
 * NULL if we're done working and want to deploy without wasting time printing */
//Stream *debug = &Serial;  // Use this for debugging
Stream *debug = NULL;     // Use this for deployment

/* Back-end classes that do all the heavy lifting */
LEDPin wifi_led(LED_BUILTIN, 20);     // WiFi Status and UDP/TCP I/O Indicator LED
WifiManager wifi(LED_BUILTIN, debug); // WiFi Manager
OSCManager osc(debug);                // Open Sound Control Manager

/* Sample timer; calls the audio render callback function at a specified rate */
ETSTimer sample_timer;                          // Sensor sample timer
const float sample_rate = 16000;                // Sensor sample rate (Hz)
const float sample_period = 1e6 / sample_rate;  // Sensor sample period (microseconds)

// 8-bit ADSR envelope generator, outputs (0-255)
ADSR8 adsr;

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
  osc.dispatch("/attack", osc_handle_attack);
  osc.dispatch("/decay", osc_handle_decay);
  osc.dispatch("/sustain", osc_handle_sustain);
  osc.dispatch("/release", osc_handle_release);
  osc.dispatch("/gate", osc_handle_gate);
  osc.dispatch("/retrigger", osc_handle_retrigger);

  // ADSR setup
  adsr.set_eod_handler(end_of_decay, NULL);     // Callback function for end of decay
  adsr.set_eor_handler(end_of_release, NULL);   // Callback function for end of release

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

// Main Loop
// =========
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
  sigmaDeltaWrite(0, adsr.render());   // Write CV to channel 0
}

// WiFi Connect Handler:
// =====================
/* This function is called by WifiManager when it successfully connects to a network.
 * We use it to open a UDP port with the number specified in the WiFi config settings.
 */
void wifi_connected(void *userdata) {
  osc.open_port(wifi.get_iot_port());  
}

// ADSR End-of-State Handlers:
// ===========================
/* These functions get called by ADSR8 when decay and release states end. Here, we
 * send a message back to Max/MSP when either happens.
 */
void end_of_decay(void *userdata) {
  OSCMessage msg("/eod"); 
  osc.send(msg);         
}

void end_of_release(void *userdata) {
  OSCMessage msg("/eor"); 
  osc.send(msg);          
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
  // Set the /ping sender's IP as the destination address
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

/* 
 * /attack <int/float>
 * 
 * Set attack time in miliseconds
 */
void osc_handle_attack(OSCMessage &msg) {
  float time_ms;
  if (msg.isInt(0))     
    time_ms = (float)msg.getInt(0);
  else if (msg.isFloat(0))
    time_ms = msg.getFloat(0);
  else return;
  adsr.set_attack((uint32_t)(time_ms/1000.0 * sample_rate));
}

/* 
 * /decay <int/float>
 * 
 * Set decay time in miliseconds
 */
void osc_handle_decay(OSCMessage &msg) {
  float time_ms;
  if (msg.isInt(0))     
    time_ms = (float)msg.getInt(0);
  else if (msg.isFloat(0))
    time_ms = msg.getFloat(0);
  else return;
  adsr.set_decay((uint32_t)(time_ms/1000.0 * sample_rate));
}

/* 
 * /sustain <int/float>
 * 
 * Set sustain level [0-255]
 */
void osc_handle_sustain(OSCMessage &msg) {
  uint8_t level;
  if (msg.isInt(0))     
    level = msg.getInt(0);
  else if (msg.isFloat(0))
    level = (uint8_t)round(msg.getFloat(0));
  else return;
  adsr.set_sustain(level);
}

/* 
 * /release <int/float>
 * 
 * Set release time in miliseconds
 */
void osc_handle_release(OSCMessage &msg) {
  float time_ms;
  if (msg.isInt(0))     
    time_ms = (float)msg.getInt(0);
  else if (msg.isFloat(0))
    time_ms = msg.getFloat(0);
  else return;
  adsr.set_release((uint32_t)(time_ms/1000.0 * sample_rate));
}

/* 
 * /gate <int>
 * 
 * Gate the ADSR on (value != 0) or off (value == 0)
 */
void osc_handle_gate(OSCMessage &msg) {
  if (msg.isInt(0)) 
    adsr.gate(msg.getInt(0) != 0);
}

/* 
 * /retrigger <int>
 * 
 * Set ADSR to retrigger on end of decay (value != 0)
 */
void osc_handle_retrigger(OSCMessage &msg) {
  if (msg.isInt(0)) 
    adsr.set_retrigger(msg.getInt(0) != 0);
}

