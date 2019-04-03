#define USE_US_TIMER    // Necessary for enabling ETSTimer's microsecond accuracy

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "sigma_delta.h"
}

#include <WifiManager.h>
#include <OSCManager.h>
#include <LEDPin.h>
#include <Sequencer.h>

/* This pointer can point at the serial port if we're developing and debugging, or
 * NULL if we're done working and want to deploy without wasting time printing */
//Stream *debug = &Serial;  // Use this for development
Stream *debug = NULL;     // Use this for deployment

/* Back-end classes that do all the heavy lifting */
LEDPin wifi_led(LED_BUILTIN, 20);     // WiFi Status and UDP/TCP I/O Indicator LED
WifiManager wifi(LED_BUILTIN, debug); // WiFi Manager
OSCManager osc(debug);                // Open Sound Control Manager

/* Sample timer; calls the audio render callback function at a specified rate */
ETSTimer sample_timer;                          // Sensor sample timer
const float sample_rate = 16000;                // Sensor sample rate (Hz)
const float sample_period = 1e6 / sample_rate;  // Sensor sample period (microseconds)

// 8-bit sequencer, outputs (0-255)
SEQ8 seq;

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
  osc.dispatch("/sequence", osc_handle_sequence);
  osc.dispatch("/steptime", osc_handle_steptime);
  osc.dispatch("/glidetime", osc_handle_glidetime);
  osc.dispatch("/timedsequence", osc_handle_timedsequence);
  osc.dispatch("/append", osc_handle_append);
  osc.dispatch("/clear", osc_handle_clear);
  osc.dispatch("/gate", osc_handle_gate);
  osc.dispatch("/reset", osc_handle_reset);
 
  // Sequencer setup
  // ===============
  // Set end-of-sequence handler
  seq.set_eos_handler(end_of_sequence, NULL);
  
  // Initial sequencer step length
  float steptime_ms = 500.0;
  seq.set_step_length((uint32_t)(steptime_ms/1000.0 * sample_rate));
  
  // Initial sequence
  uint8_t val = 31;
  for (int i = 0; i < 8; i++) {
    seq.append_step(val);
    val += 32;
  }
  // ===============
  
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
  sigmaDeltaWrite(0, seq.render());   // Write CV to channel 0
}

// WiFi Connect Handler:
// =====================
/* This function is called by WifiManager when it successfully connects to a network.
 * We use it to open a UDP port with the number specified in the WiFi config settings.
 */
void wifi_connected(void *userdata) {
  osc.open_port(wifi.get_iot_port());  
}

// End-of-Sequence Handler
// =======================
void end_of_sequence(void *userdata) {
  OSCMessage msg("/eos"); 
  osc.send(msg);          // Send a message back to Max/MSP
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
 * /sequence <int/float> <int/float> ... <int/float>
 * 
 * Set up to 512 sequencer steps [0-255]
 */
void osc_handle_sequence(OSCMessage &msg) {
  
  uint16_t n_args = msg.size();
  uint8_t stepval;
  for (int i = 0; i < n_args; i++) {
    
    // Get integer or convert float to int; ignore other types
    if (msg.isInt(i))
      stepval = msg.getInt(i);
    else if (msg.isFloat(i)) 
      stepval = (uint8_t)msg.getFloat(i);
    else {
      n_args--;
      continue;
    }
    
    // If the sequencer already has a step for this index, set it
    if (i < seq.num_steps())
      seq.set_step(i, stepval);
    
    // Otherwise append a new step
    else 
      seq.append_step(stepval);
  }
}

/*
 * /steptime <int/float>
 * 
 * Sets a global step duration in miliseconds; also configures the sequencer
 * to use global duration instead of individual step durations
 */
void osc_handle_steptime(OSCMessage &msg) {
  float time_ms;
  if (msg.isInt(0))
    time_ms = (float)msg.getInt(0);
  else if (msg.isFloat(0)) 
    time_ms = msg.getFloat(0);
  else return;
  seq.set_step_length((uint32_t)(time_ms/1000.0 * sample_rate));
  seq.uniform_step = true;
} 

/*
 * /glidetime <int/float>
 * 
 * Set the sequencer's glide (portamento) time in miliseconds
 */
void osc_handle_glidetime(OSCMessage &msg) {
  float time_ms;
  if (msg.isInt(0))
    time_ms = (float)msg.getInt(0);
  else if (msg.isFloat(0)) 
    time_ms = msg.getFloat(0);
  else return;
  seq.set_glide_length((uint32_t)(time_ms/1000.0 * sample_rate));
} 

/*
 * /timedsequence <int/float> <int/float> ... <int/float>
 * 
 * Set up to 512 sequencer steps and times using pairs of step values [0-255]
 * and step times in miliseconds; 
 */
void osc_handle_timedsequence(OSCMessage &msg) {
  
  uint16_t n_args = msg.size();
  uint8_t stepval;
  float steptime_ms;
  for (int i = 0; i < n_args; i++) {

    // Get integer value or convert float to int;
    // otherwise, ignore this argument pair
    if (msg.isInt(i))
      stepval = msg.getInt(i);
    else if (msg.isFloat(i))
      stepval = (uint8_t)msg.getFloat(i);
    else {
      i++;
      n_args -= 2;  
      continue;
    }

    // Convert integer time(ms) to float or get float; 
    // otherwise ignore this argument pair
    if (msg.isInt(i))
      steptime_ms = (float)msg.getInt(i);
    else if (msg.isFloat(i))
      steptime_ms = msg.getFloat(i);
    else {
      i++;
      n_args -= 2;
      continue;
    }
    
    // If the sequencer already has a step for this index, set it
    if (i < seq.num_steps())
      seq.set_step(i, stepval, (uint32_t)(steptime_ms/1000.0 * sample_rate));
    
    // Otherwise append a new step
    else 
      seq.append_step(stepval, (uint32_t)(steptime_ms/1000.0 * sample_rate));

    seq.uniform_step = false;
  }
}

/*
 * /append <int/float> 
 * 
 * Adds a new sequencer step [0-255] to the end of the current sequence
 * 
 * /append <int/float> <int/float> 
 * 
 * Adds a new sequencer step [0-255] and duration (in miliseconds); also
 * sets the sequencer to use individual durations per step
 */
void osc_handle_append(OSCMessage &msg) {

  uint8_t stepval;
  float steptime_ms;

  // Get the step value
  if (msg.isInt(0))
    stepval = msg.getInt(0);
  else if (msg.isFloat(0))
    stepval = (uint8_t)msg.getFloat(0);

  // Check for a step duration
  if (msg.size() > 1) {
    if (msg.isInt(1))
      steptime_ms = (float)msg.getInt(1);
    else if (msg.isFloat(1))
      steptime_ms = msg.getFloat(1);
    seq.append_step(stepval, (uint32_t)(steptime_ms/1000.0 * sample_rate));
    seq.uniform_step = false;
  }
  else
    seq.append_step(stepval);
}

/*
 * /clear
 * 
 * Clears current sequence
 */
void osc_handle_clear(OSCMessage &msg) {
  seq.clear();
}

/*
 * /gate <int>
 * 
 * Turns the sequencer off (zero) or on (nonzero)
 */
void osc_handle_gate(OSCMessage &msg) {
  if (msg.isInt(0)) 
    seq.gate(msg.getInt(0) != 0);
}

/*
 * /reset
 * 
 * Resets sequence to the first step
 */
void osc_handle_reset(OSCMessage &msg) {
  seq.reset();
}

