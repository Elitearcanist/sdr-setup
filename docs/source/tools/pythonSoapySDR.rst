SoapySDR in Python
==========================
Setting up `SoapySDR <https://github.com/pothosware/SoapySDR/wiki>`__
in Python is fairly simple but can have difficult to resolve problems.
According to the `Python Support page <https://github.com/pothosware/SoapySDR/wiki/PythonSupport>`__
the Python module gives acces to the C++ API, most calls are identical.

Install Dependencies
-------------------------

Debian/Ubuntu:
""""""""""""""""

.. code-block:: console

    sudo apt-get install python-dev swig


.. note::

    Newer versions of linux may not be able to install :code:`python-dev`
    however, it can be substituted with :code:`python-dev-is-python3`.


Windows+MSVC:
""""""""""""""""

Install `Windows Python <https://www.python.org/downloads/windows/>`__
and `swigwin <https://www.swig.org/download.html>`__ from prebuilt installers.


Basic Example
---------------

.. admonition:: Soapy Modules
    :class: warning

    Before running any code SoapySDR must be able to connect to your device!
    Ensure that the correct :ref:`SDR module <tools/soapysdr:modules>`
    has been installed.

.. code-block:: python
    :caption: Example Code to Pull Samples from an SDR Device

    import SoapySDR
    from SoapySDR import *  # SOAPY_SDR_ constants
    import numpy  # use numpy for buffers

    # enumerate devices
    results = SoapySDR.Device.enumerate()
    for result in results:
        print(result)

    # create device instance
    # args can be user defined or from the enumeration result
    args = dict(name="LimeSDR Mini")  # this will search for a device named "LimeSDR Mini"
    # there are many ways of making a device, this one will return a list of devices based on the givin parameters
    sdr = SoapySDR.Device(args)[0]  # the 0 selects the first detected device

    # query device info
    print(sdr.listAntennas(SOAPY_SDR_RX, 0))
    print(sdr.listGains(SOAPY_SDR_RX, 0))
    freqs = sdr.getFrequencyRange(SOAPY_SDR_RX, 0)
    for freqRange in freqs:
        print(freqRange)

    # apply settings
    sdr.setSampleRate(SOAPY_SDR_RX, 0, 1e6)
    sdr.setFrequency(SOAPY_SDR_RX, 0, 912.3e6)  # set freq to 912.3 MHZ

    # setup a stream (complex floats)
    rxStream = sdr.setupStream(SOAPY_SDR_RX, SOAPY_SDR_CF32)
    sdr.activateStream(rxStream)  # start streaming

    # create a re-usable buffer for rx samples
    buff = numpy.array([0] * 1024, numpy.complex64)

    # receive some samples
    for i in range(10):
        sr = sdr.readStream(rxStream, [buff], len(buff))
        print(sr.ret)  # num samples or error code
        print(sr.flags)  # flags set by receive operation
        print(sr.timeNs)  # timestamp for receive buffer

    # shutdown the stream
    sdr.deactivateStream(rxStream)  # stop streaming
    sdr.closeStream(rxStream)

.. admonition:: Error: No module named 'SoapySDR'
    :class: error

    If running the Python code returns this error then the SoapySDR file
    probably isn't in the Python Path.
    In order to get SoapySDR working it may need to be added.
    The make command when installing SoapySDR lists where
    the python dist package is installed.
    For example it could be installed here:
    :code:`/usr/local/lib/python3.12/dist-packages`

    There are two options (and possibly others) for fixing this:
     1. Use sys to add it to the Python files path
     Before the :code:`import SoapySDR` line add the following

        .. code-block:: python

           import sys
           sys.path.append("/path/to/SoapySDR/dist/directory")


     2. Add SoapySDR to the Python Path
     The command below will add the dist-packages
     **temporarily** (will reset on console restart) to the Python path.


        .. code-block:: console

            export PYTHONPATH="$PYTHONPATH:/path/to/SoapySDR/dist/directory"

     To make this **permanent** the environment file will need to be edited.
     On linux systems the file is in the :code:`/etc` directory.
     Add a newline in the file pointing to the Python path:
     :code:`PYTHONPATH=/path/to/SoapySDR/dist/directory`.
