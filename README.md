# IoT Music Workshop

This codebase and example project set is designed to run on the ESP8266-based NodeMCU microcontroller and build using the Arduino IDE.

Install in your Arduino libraries folder:

### In OS X
* /User/Documents/Arduino/libraries

### In Windows 10
* /Documents/My Documents/Arduino/Libraries

## Dependencies

### esp8266 boards package

1. Open Arduino Preferences
1. Under 'Additional Boards Manager URLs', enter http://arduino.esp8266.com/stable/package_esp8266com_index.json
1. Open the Boards Manager under Tools->Board->Boards Manager
1. Search 'esp8266' and install

### [esp8266-OSC](https://github.com/sandeepmistry/esp8266-OSC)

1. Open the Arduino Library Manager under Sketch->Include Library->Manage Libraries
1. Search 'esp8266-OSC' and install

## Example Sketches (CV, LFO, ADSR, Sequencer)

The first time you program the device, it will fail to connect to a network (none is specified by default), and open as an access point. You can then connect to it (check your list of WiFi networks for `ap-device-1`) using the password `iotconfig` to configure it with a network name, password, device identifier, node identifier, and port number.

To broadcast OSC to any IoT device(s) on your local network, use the Max/MSP examples packaged with each Arduino example sketch, or create the object 

`[udpsend 255.255.255.255 <portnumber>]`

To receive OSC from the device, use 

`[udpreceive <portnumber>]`

##### OSC Messages
`/config` causes the device to enter access point mode for configuration
* Connect to it  using password `iotconfig`

`/ping` causes the device to respond to the sender with `/pong <deviceID> <nodeID> <IPAddress>`

* Use this IP address to send OSC messages directly to specific devices

##### Analog (PWM) Output
Each example writes an 8-bit value [0-255] to pin D1, corresponding to [0-3.3] Volts.

## CV
Writes the specified cv

##### OSC Messages
`/cv <int/float>` sets the value [0-255]

## LFO
Low-frequency oscillator with continuously variable *Ramp-Triangle-Ramp* shape

##### OSC Messages
`/rate <int/float>` sets the rate in Hz

`/dutycycle <float>` sets the shape [0-1]

## ADSR
Attack/Decay/Sustain/Release Envelope Generator

##### OSC Messages
`/attack <int/float>` sets the attack time in miliseconds

`/decay <int/float>` sets the decay time in miliseconds

`/sustain <int/float>` sets the sustain level [0-255]

`/release <int/float>` sets the time in miliseconds

`/dutycycle <int/float>` sets the shape [0-1]

`/gate <int>` gate OFF with 0, and ON with any non-zero integer

`/retrigger <int>` set to retrigger on end of decay (non-zero integer)

## Sequencer
Step sequencer with up to 512 steps and portamento (glide)

##### OSC Messages (basic)
`/sequence <int/float> <int/float> ... <int/float>` set up to 512 sequencer steps [0-255]

`/steptime <int/float>` sets the time between steps (miliseconds)

`/glidetime <int/float>` sets portamento time (miliseconds)

`/append <int/float>` adds a step to the sequence [0-255]

`/clear` clears all steps

`/gate <int>` gate OFF with 0, and ON with any non-zero integer

`/reset` restarts sequence from first step

##### OSC Messages (advanced)
The above messages each assume a uniform step time. Use the following messages to assign step times to each step individually:

`/timedsequence <int/float> <int/float> ... <int/float>` sets step [0-255] and time (ms) using each pair of arguments

`/append <int/float> <int/float>` adds a step to the sequence [0-255] with specified time (ms)

Using these messages sets the sequencer in non-uniform step mode. Any use of `/steptime` will put the sequencer back into uniform step mode.