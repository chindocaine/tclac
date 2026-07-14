_(Translated from Russian original by AI)_

External component for TCL air conditioners and analogs for Home Assistant using ESPHome.
TAC-07CHSA type air conditioners and similar are supported. Alas, it is almost impossible to predict exactly
whether it will be possible to connect the air conditioner or not due to the huge variation in equipment:
even the same model, literally letter-for-letter, may, for example, not have a native WiFI module,
not have a cable with a USB connector, or the UART connector may not be soldered on the control board at all.
However, in general, with or without soldering, the following air conditioners have been tested:
- Axioma ASX09H1/ASB09H1
- Ballu BSAI-12HN1_15Y
- Ballu Discovery DC BSVI-07HN8
- Ballu Discovery DC BSVI-09HN8
- Ballu Discovery DC BSVI-12HN8
- Daichi AIR20AVQ1/AIR20FV1
- Daichi AIR25AVQS1R-1/AIR25FVS1R-1
- Daichi AIR35AVQS1R-1/AIR35FVS1R-1
- Daichi DA35EVQ1-1/DF35EV1-1
- Dantex RK-12SATI/RK-12SATIE
- Ecostar Radium KVS-RAD09CH
- Royal Clima Gloria Inverter
- Royal Clima Pandora RC-PDC28HN
- Tesla TT27TP61S-0932IAWUV
- TCL ELI ONF 12
- TCL Liferise ONF 09
- TCL TAC-CT09INV/R
- TCL One Inverter TACM-09HRID/E1 (possible different pinout)
- TCL TAC-07CHSA/TPG-W
- TCL TAC-09CHSA/TPG
- TCL TAC-09CHSA/DSEI-W
- TCL TAC-09HRID/E1
- TCL TAC-12CHSA/TPG
- TCL TAC-12CHSA/TPGI
- TCL TAC-XAL24I
- TCL TPG31IHB

The component requires Home Assistant and ESPHome version at least 2025.11.0!
____
This is all for working EXCLUSIVELY with Home Assistant and ESPHome. If you are interested in other options or the possibility of connecting the air conditioner
some other way to some other systems, then I have something to offer:
[Option for connection via MQTT](https://github.com/pavel211/TCL-TAC-07-WiFi)
____
An article on the project is located [on my Zen channel](https://dzen.ru/a/ZmdoyUNswXWnulhg)

Everything works, even stably. Any bugs I saw- I fixed, any desires I had- I implemented. Of course, not everything, I would also like a sports car..
Using the component right now you no longer risk your mental health, but sudden bugs may well attack. If suddenly this
happens to you- please let me know on Zen, I will take measures.
A detailed description will gradually appear [on my Zen channel](https://dzen.ru/a/ZmdoyUNswXWnulhg), I will post
the most important things here as much as I can.
____
Sample configuration for ESPHome is in the file TCL-Conditioner.yaml, a simplified version of the configuration is Sample_conf.yaml. Download it to your machine
and use it in ESPHome, or just copy the entire configuration from it and paste it instead of yours, however, not forgetting to edit
all fields. There are tips for each field in the file.

Questions may arise with 2 moments: platform (chip or module) and included files. I'll try to explain.

## Platform Setup
The platform is configured exactly as it is supposed to be configured in ESPHome. For example, this is what a piece of code for ESP-01S looks like:
```yaml
esp8266:
  board: esp01_1m
```
And this is what a piece of code for the Hommyn HDN/WFN-02-01 module from the first article about the air conditioner looks like:
```yaml
esp32:
  board: esp32-c3-devkitm-1
  framework:
    type: arduino
```
You can also connect the platform through the main config. Here is an example for Esp32 WROOM32, proposed by an [alpha version tester](https://github.com/kai-zer-ru):
```yaml
esphome:
  platform: ESP32
  board: nodemcu-32s
```
And this is an example for wemos D1 Mini nodemcu esp12f:
```yaml
esphome:
  platform: ESP8266
  board: esp12e
```
In general- everything is the same as usual, the option for your platform is easily searched for on the Internet.

**!It is important not to forget to comment out or delete lines of other platforms!**

## Configuration of Included Files
To add or remove certain parts of the config, I decided to use included files- they are loaded by ESPHome automatically,
if the server with Home Assistant has internet access. This approach allows you to edit and update not the entire config as a block,
but in parts, without touching what works. So, this is what the block of included files looks like:
```yaml
packages:
  remote_package:
    url: https://github.com/chindocaine/tclac.git
    ref: master
    files:
      - packages/core.yaml # The core of all things
    refresh: 30s
```
`packages/core.yaml` is the only required file.
