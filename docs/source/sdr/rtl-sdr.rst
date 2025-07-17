RTL-SDR
=======
Hardware Setup
--------------
The RTL-SDR is a USB plug device that is quite large.
Due to this, we recommend using
a USB extension cable when connecting the device to your computer.

Your RTL-SDR may or may not include an antenna
depending on where you purchased it.
You may want to aquire an antenna once you determine what kind of projects
you want to make with your RTL-SDR.

Software Setup
--------------
RTL-SDR is not a plug-and-splay device,
so we will walk you through the steps to install the appropriate drivers.
Installing the drivers will depend on your computer's operating system.

Linux
^^^^^
In the case that you may have any other type of RTL driver
installed on your computer, we recommend purging them from your computer first.
You will not need the device connected for this step.

You can purge the pre-existing drivers by entering
the following commands one by one in your command terminal:

.. code-block:: console

    sudo apt purge ^librtlsdr
    sudo rm -rvf /usr/lib/librtlsdr*
    sudo rm -rvf /usr/include/rtl-sdr*
    sudo rm -rvf /usr/local/lib/librtlsdr*
    sudo rm -rvf /usr/local/include/rtl-sdr*
    sudo rm -rvf /usr/local/include/rtl_*
    sudo rm -rvf /usr/local/bin/rtl_*

Once you have purged any pre-existing drivers,
you are now ready to install the drivers.

The installation requires that you have Git installed on your computer.
If you do not already have Git,
you can install it by following the guide here_. You will need to have a free account set up with Git
which can be done on their website_.

.. _here: https://git-scm.com/book/en/v2/Getting-Started-Installing-Git

.. _website: https://github.com/signup

Enter the following commands in your terminal to install the RTL-SDR drivers:

.. code-block:: console

    sudo apt-get install libusb-1.0-0-dev git cmake pkg-config build-essential
    git clone https://github.com/rtlsdrblog/rtl-sdr-blog
    cd rtl-sdr-blog/
    mkdir build
    cd build
    cmake ../ -DINSTALL_UDEV_RULES=ON
    make
    sudo make install
    sudo cp ../rtl-sdr.rules /etc/udev/rules.d/
    sudo ldconfig

With the drivers installed, you need to unload the
DVB-T drivers. The DVB-T drivers can be *temporarily* unloaded.

There is a more permanent way to do this, but for those who wish
to do so temporarily, use the following command in the terminal:

.. code-block:: console

    sudo rmmod dvb_usb_rtl28xxu

Linux defaults to reuploading the DVB-T drivers every time
the device is plugged back into the computer or when the computer restarts,
which makes the prior solution temporary.
In other words, you will need to re-enter that command
every time you plug in the device or restart the computer.

This can be avoided with a more permanent solution, creating a text file
called "rtlsdr.conf" to handle the DVB-T drivers issue.
To do so, enter the following one-line command in the terminal:

.. code-block:: console

    echo 'blacklist dvb_usb_rtl28xxu' | sudo tee --append /etc/modprobe.d/blacklist-dvb_usb_rtl28xxu.conf

Finally, you can restart your computer and run this
command with the RTL-SDR plugged into the USB port:

.. code-block:: console

    rtl_test

If the test command runs, then you have set up the RTL-SDR correctly.
