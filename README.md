## Automatic Filtered Water Dispenser

### Project Description 

The purpose of this project was to create a water filtration system that dispenses water automatically when a glass is placed under the tap. It should fade on LEDs when dispensing water, and should log how much water I drink and alert me when to change the filter.

### Wiring Diagram and Parts List

[wiring diagram](https://github.com/StorageB/Water-Dispenser/blob/master/wiring-diagram.pdf)

[parts list](https://github.com/StorageB/Water-Dispenser/blob/master/parts-list.md)

### Build Notes and Considerations


#### Valve
1.  The water valve should be a fail close valve. If the power goes out or signal is otherwise lost from the microcontroller, the valve should close or remain closed.
2.  You can find cheaper solenoid valves on Amazon or other sites, but keep in mind the following when selecting a valve:
      - Safety: It needs to be safe to use for beverage applications (should use lead free brass and safe to drink from plastics). 
      - Reliability: This is a commercial valve made specifically for this application. I would not trust a $3 valve from Amazon when installing this permanently in my house.
3.  The sound of the solenoid valve is pretty noticeable. It's no louder than the valve on a refrigerator's water dispenser, but I may consider building an enclosure around it to reduce the noise.
4.  It is a good idea to have a delay after the valve is open before it is allowed to close. This will prevent rapid or partial switching of the valve that may lead to early failure.

#### IR Sensor

1. I chose an infrared sensor over an ultrasonic sensor for this application for a few reasons. 
   - Simplicity: The sensor used simply outputs a low signal when an object is detected, and a high signal otherwise.
   - Size: The sensor is much smaller than an ultrasonic sensor module and will be easier to hide under the cabinet where it is mounted.
   - Health: There is a bit of research that suggests long term exposure to ultrasonic waves, although out of our hearing range, may have a negative impact on people. And since this will be running 24/7, I prefer using an IR sensor. Additionally, the frequency of an ultrasonic sensor module is in the hearing range of dogs.  
2. Sensor range: I chose a sensor with a short range of between 2 cm and 10 cm (0.8" and 4"). Because it is mounted above the kitchen sink, I did not want it to be accidentally triggered when using the sink.
3. Ghost Detection: Occasionally an IR sensor may give you false triggers based on what the light may be reflecting on (such as dust). I was having this problem but solved it but adding a simple 100 ms delay after it was triggered. After the delay, the system checks again to see if the sensor is still triggered. This also reduced rapid on/off switching of the valve if an object was just on the edge of detection.
4. IR sensors do not work well with glass. The sensor had to be positioned to detect a hand holding a glass. This resulted in the sensor not being quite as responsive as I'd like. I may consider adding a second sensor in the future for better detection.

#### NeoPixel LED strip

Because the ESP module uses 3.3V for the GPIO pins, a level shifter was required for the 5V NeoPixel strip. Although you can get by without a level shifter for most applications, to do very quick changes such as fading all of the LEDs on/off the level shifter was required. Without it, the strip had unstable and erratic behavior.

Using addressable LEDs was overkill for this project, but used mostly for learning purposes. This could have easily been completed with regular LEDs. Maybe I'll add some light animations in the future.

Adafruit has an excellent guide for how to get started with NeoPixels: https://learn.adafruit.com/adafruit-neopixel-uberguide


#### Fading LEDs

When fading LEDs with PWM, the light output levels do not scale linearly. Therefore, a logarithm curve is required. This post gives a great explanation on this along with some example Arduino code that I used in this project: https://diarmuid.ie/blog/pwm-exponential-led-fading-on-arduino-or-other-platforms

#### Handling Malfunctions

I added code to shut off the valve in the case that it was open for an abnormally long amount of time. This could be the result of a faulty sensor, disconnected sensor signal wire, stuck switch, electrical short, blocked sensor, etc. In addition, the LEDs will display an error (flashing red), and the system will have to be manually reset before it is allowed to be used again.

#### MQTT Data Logging

Future work will include logging water usage to keep track of how much I am drinking and to alert me when it is time to change the filter. MQTT messaging is currently built into the code and working, but work still needs to be done one the server side to log the data and display usage metrics on a web page.

Digi-Key had the best guide that I could find to get started with MQTT: https://www.digikey.com/en/maker/projects/send-and-receive-messages-to-your-iot-devices-using-mqtt/39ed5690cc46473abe8904c8f960341f

