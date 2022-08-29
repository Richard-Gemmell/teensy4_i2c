# Troubleshooting I2C

# Ideas
* create flowchart for people to follow

## General Strategy
* lower clock speeds are more likely to work

## Device Compatibility
* slaves on the same bus must have unique addresses
* master clock speed must be <= maximum supported by slaves
  * most slaves support 100k and 400k but not 1M
* slaves must not stretch the clock unless the master supports clock stretching
* don't put more than 1 master on the same bus unless they **all** have
multi-master support

## Electrical Issues
* excessive rise times
  - symptoms 
    * one or more devices don't respond at all
    * bus sticks frequently
    * bus works intermittently
  - problem may be triggered by
    * extending bus wires
    * adding a new device to the bus
    * rearranging wiring
    * connecting pins to the bus
  - improved by reducing pullup resistance
    * either add an external pullup resistor
    * or reduce resistance of existing pullup
    * it's not necessary to disable any internal pullups
    * you can burn out a pin if the pullup resistance is too low
      * Standard and Fast mode buses must allow 3mA
        * resistance >= 1100 Ω on a 3.3V bus
      * Fast Mode Plus devices **should** allow 20 mA but many don't
        * resistance >= 165 Ω on a 3.3V bus
  - improved by reducing capacitance
    * disconnect any unused I2C devices
    * remove any unnecessary wiring
* electrical noise
  * slave sometimes fails to respond
  * bus sticks occasionally
  * fixed by enabling glitch filters
* 

## Oscilloscope
If you've got an oscilloscope
* check high and low logic levels
  * ideally high and low voltages should be close to 3.3V and 0V
  * stable logic levels:
    * 3.0V < V high < 3.3V
    * 0V <= V low <= 0.4V
* check for excessive voltage spikes
  * undervoltage must not fall below -0.5V
  * overvoltage must not rise above 3.8V