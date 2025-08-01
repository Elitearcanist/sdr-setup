# Add uhd to site packages
import sys

sys.path.insert(
    0, "/usr/local/lib/python3.12/site-packages"
)  # package location for uhd

# Imports
import uhd
import numpy as np
import threading


##### DEVICE CONSTANTS #####
CENTER_FREQ = 100e6  # Hz
SAMPLE_RATE = 16e6  # Hz, both Tx and Rx
CLOCK_RATE = SAMPLE_RATE  # Hz, should be equal to sample rate
BANDWIDTH = 56e6

TX_GAIN = 50  # Default is 50 max is 89.75
RX_GAIN = 40  # Default was 30 max is 76

CHANNELS_TX = [0]  # SDR channels
CHANNELS_RX = [0]  # SDR channels

##### CHIRP CONSTANTS #####
NUM_CHIRPS = 10
NUM_SAMPS = 10000  # number of samples
CHIRP_DELAY = 0.5  # Time between chirps

##### FILES #####
DATA_FILE_NAME = "pythonData.bin"
NULL_DATA_FILE_NAME = "pythonNullData.bin"
CONFIG_FILE_NAME = "radarConfig.txt"

# Create the usrp device
usrp = uhd.usrp.MultiUSRP()


def write_config():
    config_file = open(CONFIG_FILE_NAME, "wt")

    config_file.write(
        f"{SAMPLE_RATE},{BANDWIDTH},{NUM_SAMPS},{NUM_CHIRPS},{CHIRP_DELAY}"
    )

    config_file.close()


# Set up and configure the usrp device
def setup_device():
    # Configure TX
    usrp.set_tx_rate(SAMPLE_RATE, 0)
    usrp.set_tx_freq(uhd.libpyuhd.types.tune_request(CENTER_FREQ), 0)
    usrp.set_tx_gain(TX_GAIN, 0)

    # Configure RX
    usrp.set_rx_rate(SAMPLE_RATE, 0)
    usrp.set_rx_freq(uhd.libpyuhd.types.tune_request(CENTER_FREQ), 0)
    usrp.set_rx_gain(RX_GAIN, 0)


# Set up the TX and RX streams
def setup_stream(is_tx: bool):
    # Set up the stream
    st_args = uhd.usrp.StreamArgs("fc32", "sc16")

    # Direction designates whether it is TX (True) or RX (False)
    if is_tx == True:
        st_args.channels = CHANNELS_TX
        streamer = usrp.get_tx_stream(st_args)
    else:
        st_args.channels = CHANNELS_RX
        streamer = usrp.get_rx_stream(st_args)

    return streamer


# Calculates an appropriate length and creates a buffer
# if it is TX it also fills buffer with an upchirp
def create_buffer(is_tx: bool, num_channels: int) -> np.ndarray:

    buffer = np.zeros((num_channels, NUM_SAMPS), dtype=np.complex64)

    # If it is tx, fill the buffer with an upchirp
    if is_tx == True:
        # Chirp constants
        chirpsPerSec: np.float32 = (
            SAMPLE_RATE / NUM_SAMPS
        )  # Enough to have one chirp in the contiguous buffer.
        chirpStartFreq: int = 0
        chirpEndFreq: int = SAMPLE_RATE
        chirpSlope: np.float32 = np.float32(
            (chirpEndFreq - chirpStartFreq) * chirpsPerSec
        )

        # Generate the up-chirp
        for i in range(NUM_SAMPS):
            t = np.float32(i / SAMPLE_RATE)
            angle = (2 * np.pi * t) * (chirpStartFreq + t * chirpSlope / 2)
            buffer[0][i] = np.cos(angle) + np.sin(angle) * 1j

    return buffer


def transmit(timestamp, tx_buffer, tx_streamer):
    # Transmit
    tx_status = usrp.send_waveform(
        waveform_proto=tx_buffer,
        duration=NUM_SAMPS / SAMPLE_RATE,  # Set to transmit samples equal to NUM_SAMPS
        freq=CENTER_FREQ,
        rate=SAMPLE_RATE,
        channels=CHANNELS_TX,
        gain=TX_GAIN,
        start_time=timestamp,
        streamer=tx_streamer,
    )
    print(tx_status)


def transmit_handler(first_timestamp):

    tx_streamer = setup_stream(True)

    tx_buffer = create_buffer(True, 1)

    for chirp_num in range(NUM_CHIRPS):
        timestamp = first_timestamp + CHIRP_DELAY * chirp_num

        transmit(timestamp, tx_buffer, tx_streamer)

    for chirp_num in range(NUM_CHIRPS):
        timestamp = first_timestamp + CHIRP_DELAY * chirp_num

        transmit(timestamp, tx_buffer, tx_streamer)


def receive(timestamp, rx_buffer, rx_streamer, data_file):
    # Receive
    # rx_buffer = usrp.recv_num_samps(
    #     num_samps=NUM_SAMPS,
    #     freq=CENTER_FREQ,
    #     rate=SAMPLE_RATE,
    #     channels=CHANNELS_RX,
    #     gain=RX_GAIN,
    #     start_time=timestamp,
    #     streamer=rx_streamer,
    # )

    metadata = uhd.types.RXMetadata()

    # Start Stream
    stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.start_cont)
    stream_cmd.stream_now = False
    stream_cmd.time_spec = timestamp
    rx_streamer.issue_stream_cmd(stream_cmd)

    # Receive Samples
    recv_buffer = np.zeros((1, 2000), dtype=np.complex64)
    for i in range(NUM_SAMPS // 2000):
        rx_streamer.recv(recv_buffer, metadata)
        rx_buffer[0][i * 2000 : (i + 1) * 2000] = recv_buffer[0]

    # Stop Stream
    stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont)
    rx_streamer.issue_stream_cmd(stream_cmd)

    data_file.write(rx_buffer)


def receive_handler(first_timestamp):
    data_file = open(DATA_FILE_NAME, mode="wb")
    null_data_file = open(NULL_DATA_FILE_NAME, mode="wb")

    rx_streamer = setup_stream(False)

    rx_buffer = create_buffer(False, 1)

    for chirp_num in range(NUM_CHIRPS):
        timestamp = first_timestamp + CHIRP_DELAY * chirp_num

        receive(timestamp, rx_buffer, rx_streamer, data_file)

    for chirp_num in range(NUM_CHIRPS):
        timestamp = first_timestamp + CHIRP_DELAY * chirp_num

        receive(timestamp, rx_buffer, rx_streamer, null_data_file)

    data_file.close()


def main():

    setup_device()

    write_config()

    first_timestamp = (
        usrp.get_time_now() + 1
    )  # give the system time to synchronize requests

    t_tx = threading.Thread(
        target=transmit_handler,
        args=(first_timestamp,),
    )
    t_rx = threading.Thread(
        target=receive_handler,
        args=(first_timestamp,),
    )
    t_tx.start()
    t_rx.start()

    t_tx.join()
    t_rx.join()


if __name__ == "__main__":
    main()
