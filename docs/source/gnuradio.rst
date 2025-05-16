GNU Radio
============================
1. Install GNU Radio (Installation instructions at the `gnu radio wiki <https://wiki.gnuradio.org/index.php/InstallingGR>`_)
    (On Linux, ``sudo apt-get install gnuradio``)

2. Install gr-limesdr
    - On Linux, installation instructions can be found on the `github page <https://github.com/myriadrf/gr-limesdr/tree/gr-3.8>`_.
    - On Windows, installation instructions can be found on the `older github page <https://github.com/myriadrf/gr-limesdr/>`_.

3. Ensure that your lime device is plugged in with an antenna that supports VHF

4. Open GNU Radio Companion

5. Open the `FM receiver example <https://github.com/myriadrf/LimeSuiteNG/blob/develop/plugins/gr-limesuiteng/examples/FM_receiver.grc>`_.

6. CTRL+F "LimeSDR Source (RX)"

7. Drag and drop the block into the workspace. Connect the "out" port to the "in" port of the Low Pass Filter block.

8. Configure the block:

    - RF frequency: baseband*1e6
    - Sample rate: samp_rate

    .. image:: ./images/GNURadio/limeFMReceiver.png
        :width: 450 px

9. Press the play button and in the new window that opens, change the baseband field to an FM radio station.

    .. image:: ./images/GNURadio/LimeFMGraph.png
        :width: 450 px