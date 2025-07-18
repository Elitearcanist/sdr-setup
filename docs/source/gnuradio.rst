GNU Radio
============================
1. Install GNU Radio (Installation instructions at the `gnu radio wiki <https://wiki.gnuradio.org/index.php/InstallingGR>`_)
    On Linux: ``sudo apt-get install gnuradio``

    On Windows: If you want to use the Adalm Pluto, use the `wsl instructions <https://wiki.gnuradio.org/index.php/WindowsInstall#WSL_|_Ubuntu>`_

2. Install any blocks necessary for your SDR
    Lime devices:
        install gr-limesdr
            On Linux, installation instructions can be found on the `github page <https://github.com/myriadrf/gr-limesdr/tree/gr-3.8>`_.
            On Windows, installation instructions can be found on the `older github page <https://github.com/myriadrf/gr-limesdr/>`_.
    Adalm-Pluto:
        Not available on Mac. Comes with GNU Radio 3.10 on Linux, for Windows follow `these instructions <https://wiki.analog.com/resources/tools-software/linux-software/gnuradio#windows_support>`_.
    USRP:
        Chances are, you have the blocks for this installed with GNU radio.
        If you have not downloaded the software for using USRP devices previously, you may have to do a little extra setup:
        ``sudo /usr/local/lib/uhd/utils/uhd_images_downloader.py``
        This command installs the images that let it connect to the board. Once it finishes installing, you can test that it worked using the instructions on the `USRP tutorial <sdr/usrpB210.rst>`_
        

        .. tip::

            If building GNU radio from source (Linux), you will see instructions to install UHD before proceeding with the rest of installation.
    RTL-SDR:
        On Linux, run ``sudo apt-get install rtl-sdr soapysdr-module-rtlsdr``. You can check that this worked with ``SoapySDRUtil --info``. You should see a message which includes:

        ``Available factories... <other_factories>, rtlsdr``

3. Ensure that your radio is plugged in with an antenna that supports VHF

4. Open GNU Radio Companion

5. Open the `FM receiver example <https://github.com/myriadrf/LimeSuiteNG/blob/develop/plugins/gr-limesuiteng/examples/FM_receiver.grc>`_ (pulled from the github for gnuradio plugin for lime devices).

6. Use CTRL+F to search for a receiver block for your radio:
    .. list-table:: SDR Receiver blocks
        :widths: 200 200
        :header-rows: 1

        *   - SDR
            - Block name
        *   - Lime
            - LimeSDR Source (RX)
        *   - Pluto
            - PlutoSDR Source
        *   - RTL-SDR
            - Soapy RTLSDR Source
        *   - USRP
            - UHD: USRP Source

7. Drag and drop the block into the workspace. Connect the "out" port to the "in" port of the Low Pass Filter block.

8. Configure the block:
    .. |Lime Config| image:: ./images/GNURadio/limeFMReceiver.png
        :width: 450 px
        :alt: Image displaying LimeSDR Source (RX) properties window, on general tab. RF frequency has been set to baseband*1e6, Sample rate has been set to samp_rate, and all other fields are left as defaults.
    
    .. |RTL-SDR Config| image:: ./images/GNURadio/rtlsdr-config.png 
        :width: 450 px
        :alt: Soapy RTLSDR Source properties window, on RF Options tab. Center Freq (Hz) field is highlighted and has been set to baseband*1e6, and AGC has been changed to True. All other fields are default."
    .. |Pluto Config| image:: ./images/GNURadio/pluto-config.png 
        :width: 450 px
        :alt: PlutoSDR Source properties window, on General tab. LO Frequency has been set to int(baseband*1e6) and Sample Rate is set to int(samp_rate). All other fields are default.

    .. |USRP Config| image:: ./images/GNURadio/usrpConfig.png 
        :width: 450 px
        :alt: UHD: USRP Source properties window, on RF Options tab. Ch0: Center Freq (Hz) has been set to baseband*1e6 and Ch0: AGC has been set to Enabled

    .. list-table:: SDR block Configuration
        :widths: 500 500
        :header-rows: 1

        *   - Lime
            - Pluto
        *   -   Under the General tab:

                - RF frequency - baseband*1e6
                - Sample rate - samp_rate
            -   Under the General tab:

                - LO Frequency - int(baseband*1e6)
                - Sample Rate - int(samp_rate)
        *   - |Lime Config|
            - |Pluto Config|

    .. list-table:: 
        :widths: 500 500
        :header-rows: 1

        *   - RTL-SDR
            - USRP
        *   -   Under the RF Options tab:

                - Center Freq (Hz) - baseband*1e6
                - AGC - True

                Under the General tab:

                - Sample Rate autopopulates
            -   Under the RF Options tab:

                - Ch0: Center Freq (Hz) - baseband*1e6
                - Ch0: AGC - Enabled

                Under the General tab:

                - Samp rate autopopulates
        *   - |RTL-SDR Config|
            - |USRP Config|

9. Press the play button and in the new window that opens, change the baseband field to an FM radio station.

    .. image:: ./images/GNURadio/LimeFMGraph.png
        :width: 450 px