> **Warning**: 
> This project is under development and not useable. I recently started a complete re-write (this repo). The original one is not disclosed at this point.
> More details below (slightly stream of consciousness right now...)



# Hakkou

`Hakkou` (from the Japanese 発酵 \[はっこう\], fermentation) is a small personal project to develop a temperature- (and in future humidity-) controlled fermentation box.
Please note that this is a experimental repository that I only work on once in while and is not intended for "general" usage, replication, or as guide.
**Features are incomplete, not working, not safe, or just undocumented for now.**

> **Note**: For most people I would recommend to just buy one of the very popular and very affordable ready-made temperature controllers (e.g., by INKBIRD), that can savely switch 1-2kW heaters (e.g., just an old lamp in an old fridge).
> There are plenty of youtube videos out there on how to build fermentation chambers with it.
> This project here is for self-education and fun. That's all. Not a way to save money ;).


## General Design
This project saw several experimental iterations, including a Python-based version running on a Raspi.
Overall, many choices, both software and hardware, are motivated by me wanting to experiment, learn a new tool, trick, or technique.
So the decisions taken are not necessarily taken because they are the best.

This brief disclaimer out of the way, the general goals I set are as follows:
* **Temperature control**: Ability to stabiliy control the temperature in reasonably sized environment to around <0.5°C precision. (Yes, the sensors list below are not the most reliable....)
   * Ability to measure both the ambient temperature as well as the temperature of the food, as after some time T the metabolic process ramps up and will produce heat itself.
  * Optionally: Ability to control the humidity in the long run. For example, to provide better environments for *koji* etc..
* **Safety**: Both in HW and SW different layers of safety should be supported. Even if the SW fails, the hardware should never cause a fire ;).
* **LCD**: I wanted to know the state of the device without need to my smartphone or webservice
* **Controllability**: Either controllable by a, e.g., NEC, remote control or buttons on the device. See point above.
* **Manual and Auto modes**: For some projects it might be interesting to not have a constant temperature, hence ability to run programs is relevant. Also the feature to be notified if actions need to be taken.

Many other DIY-projects for fermentation utilize, as they are using industrial-grade controlling devices, lamps for heating.
Instead I wanted to utilize a resistive heating element that I can directly connect to a radiator for faster distribution of the temperature to the environment.... and because I had most of it lying around.
I assume that, at least to some degree, it increases the lag time of the system, but felt a bit less dodgy than an old-school 40W bulb glowing away in fridge.

The hardware and software is discussed in a bit more detail below.

## Software
> **Warning**: 
> This is my first project using an ESP32, ESP-IDF, and FreeRTOS for development; actually my first embedded project in a long time as well.
> So there might be plenty of bad practices that I didn't resolve yet.
> Happy to get feedback tho :).


## Hardware
Here is a brief overview of the hardware used for this project.
Sadly, there is currently no in-depth documentation of the circuit combining everything.
However, you will find tutorials for most things somewhere on the internet.
Given that we control some sort of heating element in some fashion, e.g., through a MOSFET or relais, just make sure you really know what you are doing before frying your hardware or yourself, or burning down your house.
For this reason, I am also feeling a bit reluctant to give too detailed, yet too incomplete information.

### Components
Most of the components are off-the-shelf components, dev-boards, pre-made.
Still it's not total plug'n'play and especially the heater part requires careful decisions.
* **ESP-32 Dev board**
* **LCD**: 4x20 HD44780 for some very basic UI
    * Note: It's alright. It's pretty cheap and has good driver support. However, if I didn't have it in the first place, there are so many alternative options by now with similar pricing but more features. Not sure how much software complexity would increase (e.g., computational cost and size of binary), but well, maybe version 2 ;).
* **IR sensor and NEC control**:
    * Note: Chose a sensor for the appropriate kHz range.
    * Note 2: I think I wouldn't support an IR remote anymore. GPIO expander board and few physical buttons on the case would have been **much** better. Less hardware complexity, less software complexity, no remote flying around, etc.. Looks cool, but is kind of really stupid. Lesson learned. Also given the "ease" of using BLE or WIFI with the ESP, that could also be an alternative if there are enough resources left. Definitely something I'll explore in the future.
* **Sensors**:
    * BME-280; pre-made board (for ambient temperature and humidity)
    * Several DS18X20 ("waterproofed"-version) (as food temperature probes); to be replaced?
* **Heater** (+ control and fan):
    * A resistive heating element glued between two old CPU cooling blocks
    * 3/4 Wire fan: One of the cooling blocks is an old pure copper Zalman CPU cooler with integrated 4 Wire fan
        * A 3 wire fan would also do the trick. I think its worthwhile to be able to see the current RPM, but PWM control is even more optional.
        * Why? Have a WDT monitor if the RPM drops or fan stalls. Currently, there is only a pure software stall detection, but it restarts the ESP, resets GPIOs, and add another safety level when it comes to malfunction/overheating. It seems to work in practice, but is not reliable, as the task itself could stall unless it has a WDT on it (TODO).
        * The PWM line of the FAN is cool, if you want to ensure it runs quietly when your heating isn't very high.
    * Temperature protection (2-fold)
        * A self-resetting temperature switch
        * A non-resettable thermal fuse temperature protection, both thermally connected to the heating element
        * Note: I had not much experience with either when I started the project, so I chose to include a self-resetting temperature protection with considerably lower rating than the non-resettable one (to have a few chances ;). Depending on your system design and your actual hardware one or both could make sense. I definitely suggest to have a software-independent protection, that if for whatever magic reason the heater stays on, it wouldn't catch on fire.
        * Note2: I'd like to try to do the same with a hardware-based WDT, but thats for future experimentation.
    * A MOSFET capable of safely switching the heating element. Read the datasheet, calculate the thermal resistances and power dissipation. Make sure that its sufficiently cooled if necessary.
    * A MOSFET driver. While there are some TTL-compatible MOSFET capable of switching 12V stuff, I don't think you will be doing yourself any favors. Especially if you are planning to use PWM etc., you probably won't be able to safely switch the MOSFET on/of fast enough and it will just release some *magic smoke*.
    * NOTE: Make sure everything is connected such that when the pin is floating etc., you have the correct pull-up/down for your circuit and the heater is switched off unless your software tells otherwise.
* **Status LEDs**: 4x WS2812B on  strip
    * Note: What a pain. They are very flimsy. Definitely benefit from ESD protection. The unexperienced me fried a few before learning some lessons here...
* **SD Card**: Pre-made board Currently not used, but will be used in the future for logging, storing of programs, etc.


## References
This project was inspired, supported, extended, etc. by several other projects. For those with legal implications, please also refer to the `<<TODO: Used Projects>>` file (and the respective source files).

For everything else an unordered list follows:
* [https://www.youtube.com/watch?v=6pXhQ28FVlU](**Modern C++: C++ Patterns to Make Embedded Programming More Productive**)
    > A CppCon2022 talk by Steve Bush, with companion repo [here](https://github.com/sgbush/cppcon2022/). Has some ideas on strong-typing to make the calls to the typical C functions more 
