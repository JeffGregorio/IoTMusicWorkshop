# IoT Music Workshop

This codebase and example project set is designed to run on the ESP8266-based [NodeMCU](https://www.amazon.com/s?k=nodemcu&ref=nb_sb_noss_1) microcontroller and build using the [Arduino IDE](https://www.arduino.cc/en/main/software). After programming the IoT devices, we control them with [Max/MSP](https://cycling74.com/).

After installing the Arduino IDE and Max/MSP:

1. Download this library using the green button near the upper right corner of this page (Clone or Download->Download Zip) 
2. Install in your Arduino libraries folder
	*  **In OS X**: `/User/Documents/Arduino/libraries`
	* **In Windows 10** `/Documents/My Documents/Arduino/Libraries`

## Dependencies

### esp8266 boards package

1. Open Arduino Preferences
1. Under 'Additional Boards Manager URLs', enter the following URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json
1. Open the Boards Manager under Tools->Board->Boards Manager
1. Search 'esp8266' and install

### [esp8266 USB Driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers)

Note: OS X will block the installation of the driver, which you can enable (after begining the driver installation) in System Preferences->Security & Privacy.


### [esp8266-OSC](https://github.com/sandeepmistry/esp8266-OSC)

1. Open the Arduino Library Manager under Sketch->Include Library->Manage Libraries
1. Search 'esp8266-OSC' and install
1. If the library doesn't appear, you can also download it manually from the library's Github page by using the above link.

### [FixedPoints](https://github.com/Pharap/FixedPointsArduino)

1. Open the Arduino Library Manager under Sketch->Include Library->Manage Libraries
1. Search 'FixedPoints' and install
1. If the library doesn't appear, you can also download it manually from the library's Github page by using the above link.

## Example Sketches (CV, LFO, ADSR, Sequencer)

The first time you program the device, it will fail to connect to a network (none is specified by default), and open as an access point. You can then connect to it (check your list of WiFi networks for `ap-device-1`) using the password `iotconfig` to configure it with a network name, password, device identifier, node identifier, and port number.

To broadcast OSC to any IoT device(s) on your local network, use the Max/MSP examples packaged with each Arduino example sketch, or create the object 

`[udpsend 255.255.255.255 <portnumber>]`

To receive OSC from the device, use 

`[udpreceive <portnumber>]`

#### OSC Messages
`/config` causes the device to enter access point mode for configuration
* Connect to it  using password `iotconfig`

`/ping` causes the device to respond to the sender with `/pong <deviceID> <nodeID> <IPAddress>`

* Use this IP address to send OSC messages directly to specific devices

#### Analog (PWM) Output
Each example writes an 8-bit value [0-255] to pin D1, corresponding to [0-3.3] Volts.

See the included slides for notes on building a simple reconstruction filter. You can send the CV output of this filter to a modular synthesizer by making a breadboard to 3.5mm audio cable adapter using a female 3.5mm [jack](https://www.amazon.com/3-5mm-Stereo-Female-terminal-connector/dp/B077XPSKQD/ref=sr_1_1?keywords=3.5mm+female+audio+jack&qid=1554317831&s=gateway&sr=8-1). 

While the simple RC filter will work, the buffered version will help you avoid voltage drop, which is especially useful if you're trying to generate specific pitches. As noted in the slides, use a low voltage, rail-to-rail op amp like the dual [TLV2372](https://www.mouser.com/ProductDetail/Texas-Instruments/TLV2372IP?qs=sGAEpiMZZMtCHixnSjNA6P3Ssczg4flJKDjN5gpxXKE%3D) or quad [TLV2374](https://www.mouser.com/ProductDetail/Texas-Instruments/TLV2374IN?qs=sGAEpiMZZMtCHixnSjNA6KeLSdc1HUsPa9T7qjxWeeI%3D).

## CV
Bare bones example; writes the specified cv

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