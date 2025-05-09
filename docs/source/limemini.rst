Lime mini
============
Hardware setup
--------------
Use of an ESD strap while handling the Lime mini is recommended to avoid damage to the device.

Additionally, a connector saver/short extension cable will make it harder to damage the USB port.

Software setup
--------------
The Lime mini utilizes Lime Suite for API and GUI access. 
Either the Classic or Next Generation (NG) application can be used assuming a compatible OS, 
however information on use will be provided for the NG application.

NG
    Installation instructions may be found at https://limesuiteng.myriadrf.org/gettingstarted/. 
    Installing wx and then installing LimeSuiteNG from source will allow use of the GUI.

Classic
    Installation instructions may be found at https://wiki.myriadrf.org/Lime_Suite#Installation

Use
---
Information about the tools can be found at https://github.com/myriadrf/LimeSuiteNG/tree/develop

Installation comes with a set of tools allowing use of the SDR, these are

- limeDevice
- limeConfig
- limeTRX
- limeSPI
- limeFLASH

Running any of these with the ``-h`` flag will provide more information (e.g. ``limeConfig -h``).

If the GUI was installed, it can be run with the command ``limeGUI``.

Example
-------
Receiving

```
limeConfig --initialize --samplerate=20e6 --rxen=1 --rxlo=89.1e6 --rxpath=LNAW # rxlow=a local fm radio station
limeGUI
```
.. image:: 