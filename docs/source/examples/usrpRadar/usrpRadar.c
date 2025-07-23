/*
Written by Levi Powell with edits from Andrew McDonald, August 2023 updated July 2025
This code implements a basic radar using the LimeSDR Mini SDR.

Compile with: gcc -std=c99 limeRadar.c -lSoapySDR -lm -o limeRadar.out
Compile with memory checking: gcc -std=c99 limeRadar.c -fsanitize=address -fno-omit-frame-pointer -lSoapySDR -lm -o limeRadar.out
Compile with speed optimization (recommended): gcc -std=c99 limeRadar.c -lSoapySDR -lm -Ofast -o limeRadar.out
*/

/*
I had some problems with the compiler not finding the SoapySDR files
adding -L /usr/local/lib to the compile command helped it find the correct SoapySDR files and work correctly
*/

#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdint.h>

#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <SoapySDR/Constants.h>

/////////// User configuration constants ///////////
#define FREQUENCY 3e9                // The carrier frequency (Hz)
#define CLOCK_RATE 16e6              // SDR internal clock rate (Hz)
#define SAMPLE_RATE_TX CLOCK_RATE    // SDR sample rate (Hz)
#define SAMPLE_RATE_RX CLOCK_RATE    // SDR sample rate (Hz)
#define NUM_CHIRPS 20                // Number of chirps to be transmitted
#define CHIRP_DELAY (long long)0.2e9 // Time from start of one chirp to start of next chirp (nanoseconds)
#define SAVE_TO_FILE true            // Save data to file or print to screen

///// Other constants /////
// Note: if the zero delay is off from the actual delay inside the FPGA then the feedthough will not be centered in the FFT
#define ZERO_DELAY_SAMPLES 0 // Constant internal latency correction, orignially 96

#define BANDWIDTH_TX 50e6 // Bandwidth of frontend filters
#define BANDWIDTH_RX 50e6 // Bandwidth of frontend filters

// Note: the target length will be achieved by using the getSreamMTU length and using MTUs until the target is achieved
// an additional buffer is added to be trimmed later to remove trailing zeros
#define CONTIGUOUS_BUFF_TX_LENGTH_TARGET (int)1e5 // Target Length of a single chirp will be slightly larger
#define CONTIGUOUS_BUFF_RX_LENGTH_TARGET (int)1e5 // Target Length of a single chirp

#define CHANNEL_TX 0 // SDR channel
#define CHANNEL_RX 0 // SDR channel

#define STREAM_TIMEOUT_TX 1e6                              // Stream timeout (microseconds)
#define STREAM_TIMEOUT_RX 1e6                              // Stream timeout (microseconds)
#define FLAGS SOAPY_SDR_HAS_TIME                           // Stream configuration flags
#define FLAGS_END SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST // Flags for final transmission

#define CHIRP_BANDWIDTH SAMPLE_RATE_RX // Bandwidth of a chirp

#define GAIN 50    // Default is 50 max is 89.75
#define RX_GAIN 20 // Default was 20 max is 76

#define PI 3.1415926535

// Function declarations
struct SoapySDRDevice *Setup(void);
void DeviceInfo(struct SoapySDRDevice *sdr);
void SetParameters(SoapySDRDevice *sdr);
SoapySDRStream *MakeStream(SoapySDRDevice *sdr, const int direction);
void MakeBuffer(SoapySDRDevice *sdr, SoapySDRStream *stream, complex float *buffer, size_t *contBufferLength, size_t *bufferLength, int targetLength);
void SaveData(FILE *fp, int *sampleNumber, const complex float *buffer, const int length, const bool saveToFile);
void FillBuffer(complex float *buff, size_t length, size_t contBufferTxLength, size_t bufferTxLength);
void TransmitReceive(SoapySDRDevice *sdr, SoapySDRStream *txStream, SoapySDRStream *rxStream, complex float *bufferTx, complex float *bufferRx, complex float *contBufferTx, complex float *contBufferRx, long long transmitTime, long long receiveTime, int *firstSampleIndex, const size_t contBufferTxLength, const size_t contBufferRxLength, const size_t bufferTxLength, const size_t bufferRxLength);
void TrimBuffer(complex float *contBufferRx, int firstSampleIndex, size_t contBufferRxLengt);
void MixSignals(complex float *contBufferTx, complex float *contBufferRx, complex float *mixedSignal, const size_t contBufferRxLength);

int main()
{
    // Setup data and configuration files
    FILE *fp_config;
    FILE *fp_data;
    FILE *fp_nulldata;
    fp_config = fopen("radarConfig.txt", "w");
    fp_data = fopen("radarData.bin", "wb");
    fp_nulldata = fopen("radarNullData.bin", "wb");
    int sampleNumber = 0;

    // Setup SDR
    struct SoapySDRDevice *sdr = Setup();

    // Display SDR information
    DeviceInfo(sdr);

    // Set SDR parameters
    SetParameters(sdr);

    // Create rx and tx streams
    SoapySDRStream *txStream = MakeStream(sdr, SOAPY_SDR_TX);
    SoapySDRStream *rxStream = MakeStream(sdr, SOAPY_SDR_RX);
    printf("Created RX and TX streams.\n");

    ///// Make Tx Buffer /////
    size_t contBufferTxLength; // The full length of the contiguous Tx buffer
    size_t bufferTxLength;     // length of one Tx Maximum Transfer Unit

    complex float *contBufferTx;
    MakeBuffer(sdr, txStream, contBufferTx, &contBufferTxLength, &bufferTxLength, CONTIGUOUS_BUFF_TX_LENGTH_TARGET);

    contBufferTx = malloc(2 * sizeof(float) * contBufferTxLength);
    complex float *bufferTx = contBufferTx;

    ///// Make Rx Buffer /////
    size_t contBufferRxLength; // The full length of the contiguous Rx buffer
    size_t bufferRxLength;     // length of one Rx Maximum Transfer Unit

    complex float *contBufferRx;
    MakeBuffer(sdr, rxStream, contBufferRx, &contBufferRxLength, &bufferRxLength, CONTIGUOUS_BUFF_RX_LENGTH_TARGET);

    contBufferRx = malloc(2 * sizeof(float) * contBufferRxLength);
    complex float *bufferRx = contBufferRx;

    // Initialize to -1 (easy to detect errors)
    for (int i = 0; i < contBufferRxLength; i++)
    {
        contBufferRx[i] = -1 - 1 * I;
    }

    // The shorter length of the saved RX data, the full contiguous buffer length minus one MTU
    size_t contBufferRxSaveLength = contBufferRxLength - bufferRxLength;

    // Load constants into configuration file
    if (fp_config == NULL)
    {
        printf("The configuration file cannot be opened.\n");
    }
    else
    {
        fprintf(fp_config, "%d,%d,%d,%d,%d\n", (int)SAMPLE_RATE_RX, (int)CHIRP_BANDWIDTH, (int)contBufferRxSaveLength, (int)NUM_CHIRPS, (int)CHIRP_DELAY);
    }

    // Allocate a buffer to hold the mixed data
    complex float *mixedSignal = malloc(2 * sizeof(float) * contBufferRxLength);

    // Fill Tx buffer with data to transmit
    FillBuffer(contBufferTx, contBufferTxLength, contBufferTxLength, bufferTxLength);

    // Prapare timed streaming
    const char *timeSource = SoapySDRDevice_getTimeSource(sdr);
    long long currentHardwareTime;
    long long transmitTime;
    long long receiveTime;

    // Print start time
    currentHardwareTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);
    printf("Current hardware time is %lf s.\n", currentHardwareTime / 1.0e9);

    // Store the index of the first valid sample received
    int firstSampleIndex = 0;

    // Save the time when the transmit begins
    long long initTransmitTime;
    initTransmitTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);

    // Transmit and receive
    for (int k = 0; k < NUM_CHIRPS; k++)
    {
        printf("\nReady to transmit/receive chirp %d.\n", k);

        currentHardwareTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);
        printf("Started chirp code at time %lf s\n", currentHardwareTime / 1.0e9);

        transmitTime = CHIRP_DELAY * (k + 1) + initTransmitTime;
        receiveTime = CHIRP_DELAY * (k + 1) + initTransmitTime;

        TransmitReceive(sdr, txStream, rxStream, bufferTx, bufferRx, contBufferTx, contBufferRx, transmitTime, receiveTime, &firstSampleIndex, contBufferTxLength, contBufferRxLength, bufferTxLength, bufferRxLength);

        // Remove garbage values from the beginning of the contiguous buffer
        long long startCleanTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);
        TrimBuffer(contBufferRx, firstSampleIndex, contBufferRxLength);
        long long endCleanTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);

        // Mix Tx/Rx signals
        long long startMixTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);
        MixSignals(contBufferTx, contBufferRx, mixedSignal, contBufferRxLength);
        long long endMixTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);

        // Save data to file
        long long startSaveTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);
        SaveData(fp_nulldata, &sampleNumber, mixedSignal, contBufferRxSaveLength, SAVE_TO_FILE); // Note: contBufferRxSaveLength is shorter to trim off ending zeros
        long long endSaveTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);

        bufferTx = contBufferTx;
        bufferRx = contBufferRx;

        printf("\tTime to clean signal: %lf s\n", (endCleanTime - startCleanTime) / 1.0e9);
        printf("\tTime to mix signals: %lf s\n", (endMixTime - startMixTime) / 1.0e9);
        printf("\tTime to save data to file: %lf s\n", (endSaveTime - startSaveTime) / 1.0e9);

        currentHardwareTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);
        printf("Finished chirp code at time %lf s\n", currentHardwareTime / 1.0e9);
    }

    printf("\nCollected null data. Ready to collect target data. Press any key to continue...\n");
    getchar();

    initTransmitTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);

    // Transmit and receive
    for (int k = 0; k < NUM_CHIRPS; k++)
    {
        printf("\nReady to transmit/receive chirp %d.\n", k);

        currentHardwareTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);
        printf("Started chirp code at time %lf s\n", currentHardwareTime / 1.0e9);

        transmitTime = CHIRP_DELAY * (k + 1) + initTransmitTime;
        receiveTime = CHIRP_DELAY * (k + 1) + initTransmitTime;

        TransmitReceive(sdr, txStream, rxStream, bufferTx, bufferRx, contBufferTx, contBufferRx, transmitTime, receiveTime, &firstSampleIndex, contBufferTxLength, contBufferRxLength, bufferTxLength, bufferRxLength);

        // Remove garbage values from the beginning of the contiguous buffer
        long long startCleanTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);
        TrimBuffer(contBufferRx, firstSampleIndex, contBufferRxLength);
        long long endCleanTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);

        // Mix Tx/Rx signals
        long long startMixTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);
        MixSignals(contBufferTx, contBufferRx, mixedSignal, contBufferRxLength);
        long long endMixTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);

        // Save data to file
        long long startSaveTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);
        SaveData(fp_data, &sampleNumber, mixedSignal, contBufferRxSaveLength, SAVE_TO_FILE); // Note: contBufferRxSaveLength is shorter to trim off ending zeros
        long long endSaveTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);

        bufferTx = contBufferTx;
        bufferRx = contBufferRx;

        printf("\tTime to clean signal: %lf s\n", (endCleanTime - startCleanTime) / 1.0e9);
        printf("\tTime to mix signals: %lf s\n", (endMixTime - startMixTime) / 1.0e9);
        printf("\tTime to save data to file: %lf s\n", (endSaveTime - startSaveTime) / 1.0e9);

        currentHardwareTime = SoapySDRDevice_getHardwareTime(sdr, timeSource);
        printf("Finished chirp code at time %lf s\n", currentHardwareTime / 1.0e9);
    }

    // Clean up memory
    printf("\nFreeing memory...\n");
    free(mixedSignal);
    free(contBufferTx);
    free(contBufferRx);

    printf("Closing streams...\n");
    SoapySDRDevice_closeStream(sdr, txStream);
    SoapySDRDevice_closeStream(sdr, rxStream);
    SoapySDRDevice_unmake(sdr);

    printf("Closing files...\n");
    fclose(fp_config);
    fclose(fp_data);
    fclose(fp_nulldata);

    printf("DONE\n");
    return 0;
}

struct SoapySDRDevice *Setup(void)
{
    // enumerate devices
    size_t length;
    SoapySDRKwargs *results = SoapySDRDevice_enumerate(NULL, &length);
    for (size_t i = 0; i < length; i++)
    {
        printf("[Setup] Found device #%d: ", (int)i);
        for (size_t j = 0; j < results[i].size; j++)
        {
            printf("%s=%s, ", results[i].keys[j], results[i].vals[j]);
        }
        printf("\n");
    }
    SoapySDRKwargsList_clear(results, length);
    // create device instance
    // args can be user defined or from the enumeration result
    SoapySDRKwargs args = {};
    SoapySDRKwargs_set(&args, "driver", "uhd");
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);
    if (sdr == NULL)
    {
        printf("[Setup] SoapySDRDevice_make fail: %s\n", SoapySDRDevice_lastError());
        // return EXIT_FAILURE;
        return NULL;
    }

    return sdr;
}

void DeviceInfo(struct SoapySDRDevice *sdr)
{
    // query device info
    size_t length;
    char **names = SoapySDRDevice_listAntennas(sdr, SOAPY_SDR_RX, 0, &length);
    printf("[DeviceInfo] Rx antennas: ");
    for (size_t i = 0; i < length; i++)
        printf("%s, ", names[i]);
    printf("\n");
    SoapySDRStrings_clear(&names, length);

    names = SoapySDRDevice_listGains(sdr, SOAPY_SDR_RX, 0, &length);
    printf("[DeviceInfo] Rx gains: ");
    for (size_t i = 0; i < length; i++)
        printf("%s, ", names[i]);
    printf("\n");
    SoapySDRStrings_clear(&names, length);

    SoapySDRRange *ranges = SoapySDRDevice_getFrequencyRange(sdr, SOAPY_SDR_RX, CHANNEL_RX, &length);
    printf("[DeviceInfo] Rx freq ranges: ");
    for (size_t i = 0; i < length; i++)
        printf("[%g Hz -> %g Hz], ", ranges[i].minimum, ranges[i].maximum);
    printf("\n");
    free(ranges);

    SoapySDRArgInfo *args = SoapySDRDevice_getSettingInfo(sdr, &length);
    printf("[DeviceInfo] Device Settings: ");
    for (size_t i = 0; i < length; i++)
        printf("Key: %s, Value: %s, Desc: %s], ", args[i].key, args[i].value, args[i].description);
    printf("\n");
    free(args);

    bool automaticEnabled = SoapySDRDevice_getIQBalanceMode(sdr, SOAPY_SDR_RX, CHANNEL_RX);
    printf("[DeviceInfo] Automatic IQ Balance is %s\n", automaticEnabled ? "enabled." : "NOT enabled.");

    automaticEnabled = SoapySDRDevice_getDCOffsetMode(sdr, SOAPY_SDR_RX, CHANNEL_RX);
    printf("[DeviceInfo] Automatic DC Offset is %s\n", automaticEnabled ? "enabled." : "NOT enabled.");

    char **clockSources = SoapySDRDevice_listClockSources(sdr, &length);
    printf("[DeviceInfo] Clock Sources: ");
    for (size_t i = 0; i < length; i++)
        printf("%s, ", clockSources[i]);
    printf("\n");
    free(clockSources);

    printf("[DeviceInfo] Clock Source: %s\n", SoapySDRDevice_getClockSource(sdr));
}

void SetParameters(SoapySDRDevice *sdr)
{
    // Set clock rate
    if (SoapySDRDevice_hasHardwareTime(sdr, NULL))
    {
        SoapySDRDevice_setMasterClockRate(sdr, CLOCK_RATE);
        printf("[SetParameters] Master Clock rate was set to: %e Hz\n", CLOCK_RATE);
        printf("[SetParameters] Master Clock rate was read back as: %e Hz\n", SoapySDRDevice_getMasterClockRate(sdr));
    }
    else
    {
        printf("[SetParameters] This device does not support timed streaming.\n");
    }

    // Setup Tx parameters
    if (SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, CHANNEL_TX, SAMPLE_RATE_TX) != 0)
    {
        printf("[SetParameters] Tx setSampleRate fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setAntenna(sdr, SOAPY_SDR_TX, CHANNEL_TX, "TX/RX") != 0)
    {
        printf("[SetParameters] Tx setAntenna fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, CHANNEL_TX, GAIN) != 0)
    {
        printf("[SetParameters] Tx setGain fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, CHANNEL_TX, FREQUENCY, NULL) != 0)
    {
        printf("[SetParameters] Tx setFrequency fail: %s\n", SoapySDRDevice_lastError());
    }
    // if (SoapySDRDevice_setBandwidth(sdr, SOAPY_SDR_TX, CHANNEL_TX, BANDWIDTH_TX) != 0)
    // {
    //     printf("[SetParameters] Tx setBandwidth fail: %s\n", SoapySDRDevice_lastError());
    // }

    // Setup Rx parameters
    if (SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, CHANNEL_RX, SAMPLE_RATE_RX) != 0)
    {
        printf("[SetParameters] Rx setSampleRate fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setAntenna(sdr, SOAPY_SDR_RX, CHANNEL_RX, "RX2") != 0)
    {
        printf("[SetParameters] Rx setAntenna fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, CHANNEL_RX, RX_GAIN) != 0)
    {
        printf("[SetParameters] Rx setGain fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, CHANNEL_RX, FREQUENCY, NULL) != 0)
    {
        printf("[SetParameters] Rx setFrequency fail: %s\n", SoapySDRDevice_lastError());
    }
    // if (SoapySDRDevice_setBandwidth(sdr, SOAPY_SDR_RX, CHANNEL_RX, BANDWIDTH_RX) != 0)
    // {
    //     printf("[SetParameters] Rx setBandwidth fail: %s\n", SoapySDRDevice_lastError());
    // }

    if (SoapySDRDevice_setIQBalanceMode(sdr, SOAPY_SDR_RX, CHANNEL_RX, true) != 0)
    {
        printf("[SetParameters] Rx setIQBalanceMode fail: %s\n", SoapySDRDevice_lastError());
    }
}

SoapySDRStream *MakeStream(SoapySDRDevice *sdr, const int direction)
{
    // Print stream arguements to see possible configuration options
    size_t length;
    SoapySDRArgInfo *streamArgs = SoapySDRDevice_getStreamArgsInfo(sdr, direction, (size_t)0, &length);
    printf("[MakeStream] Stream Settings: \n");
    for (size_t i = 0; i < length; i++)
    {
        printf("\tKey: %s, Value: %s, Desc: %s Options:", streamArgs[i].key, streamArgs[i].value, streamArgs[i].description);
        for (size_t j = 0; j < streamArgs[i].numOptions; j++)
        {
            printf("\'%s\' ", streamArgs[i].options[j]);
        }
        printf("\n");
    }
    free(streamArgs);

    SoapySDRKwargs args = {};
    SoapySDRKwargs_set(&args, "WIRE", "sc16"); // define the bus format to signed complex 16

    // If on transmit set the send_frame_size, on recieve set the recv_frame_size
    if (direction == 0)
    {
        if (SoapySDRKwargs_set(&args, "num_send_frames", "32") != 0)
        {
            printf("[MakeStream] num_send_frames allocation failed!\n");
        }
    }
    else
    {
        if (SoapySDRKwargs_set(&args, "num_recv_frames", "32") != 0)
        {
            printf("[MakeStream] num_recv_frames allocation failed!\n");
        }
    }

    // Create a stream
    SoapySDRStream *stream = SoapySDRDevice_setupStream(sdr, direction, SOAPY_SDR_CF32, NULL, 0, &args);
    if (stream == NULL)
    {
        printf("[MakeStream] Setup stream fail: %s\n", SoapySDRDevice_lastError());
    }
    SoapySDRKwargs_clear(&args); // remove the created args

    return stream;
}

// Finds the MTU and uses that value to determine and allocate buffer size
void MakeBuffer(SoapySDRDevice *sdr, SoapySDRStream *stream, complex float *buffer, size_t *contBufferLength, size_t *bufferLength, int targetLength)
{

    *bufferLength = SoapySDRDevice_getStreamMTU(sdr, stream); // Get maximum transfer unit
    printf("[MakeBuffer] Length of MTU buffer: %zu\n", *bufferLength);

    *contBufferLength = 0;
    // add buffer length to ensure there is an extra buffer to trim (elimnates trailing zeros)
    while (*contBufferLength < (targetLength + *bufferLength))
    {
        *contBufferLength += *bufferLength;
    }

    printf("[MakeBuffer] Length of contiguous buffer: %zu\n", *contBufferLength);
}

// Save recieved data to a file
void SaveData(FILE *fp, int *sampleNumber, const complex float *buffer, const int length, const bool saveToFile)
{
    if (saveToFile)
    {
        printf("[SaveData] Saving data to file...\n");

        if (fp == NULL)
        {
            printf("[SaveData] The file cannot be opened.\n");
        }
        else
        {
            float *real = malloc(length * sizeof(float));
            float *imag = malloc(length * sizeof(float));
            if (real == NULL || imag == NULL)
                printf("[SaveData] Failed to allocate memory!\n");
            for (int i = 0; i < length; i++)
            {
                real[i] = creal(buffer[i]);
                imag[i] = cimag(buffer[i]);
            }
            size_t writtenReal = fwrite(real, sizeof(float), length, fp);
            size_t writtenImag = fwrite(imag, sizeof(float), length, fp);

            if (writtenReal != length || writtenImag != length)
            {
                printf("[SaveData] Failed to write data to file!\n");
            }
            free(real);
            free(imag);
        }
    }
    else
    {
        printf("[SaveData] Printing data to screen...\n");
        for (int i = 0; i < length; i++)
        {
            printf("%d: [%f, %fi]\n", *sampleNumber, crealf(buffer[i]), cimagf(buffer[i]));
            (*sampleNumber)++;
        }
    }
}

void FillBuffer(complex float *buff, size_t length, size_t contBufferTxLength, size_t bufferTxLength)
{
    printf("[Fill Buffer] Filling buffer\n");

    // Chirp Constants
    float chirpsPerSec = ((float)SAMPLE_RATE_TX) / contBufferTxLength; // Enough to have one chirp in the contiguous buffer.
    int chirpStartFreq = 0;
    int chirpEndFreq = CHIRP_BANDWIDTH;
    double chirpSlope = (chirpEndFreq - chirpStartFreq) * chirpsPerSec;

    printf("Chirp data:\n chirps per second: %f, chip slope %lf\n", chirpsPerSec, chirpSlope);

    // Generate the up-chirp
    for (int i = 0; i < length; i++)
    {
        double time = i / SAMPLE_RATE_TX;
        const double angle = (2 * PI * time) * (chirpStartFreq + time * chirpSlope / 2);
        buff[i] = cos(angle) + sin(angle) * I;
    }
}

void TransmitReceive(SoapySDRDevice *sdr, SoapySDRStream *txStream, SoapySDRStream *rxStream, complex float *bufferTx, complex float *bufferRx, complex float *contBufferTx, complex float *contBufferRx, long long transmitTime, long long receiveTime, int *firstSampleIndex, const size_t contBufferTxLength, const size_t contBufferRxLength, const size_t bufferTxLength, const size_t bufferRxLength)
{
    const void *buffsTx[] = {bufferTx};
    void *buffsRx[] = {bufferRx};

    long long readTimestampRx;
    int readFlags;

    printf("[TransmitReceive] Scheduled to transmit/receive a chirp at TX:%lf s, RX:%lf s\n", transmitTime / 1.0e9, receiveTime / 1.0e9);

    long long firstRxTimestamp = -1;

    // Activate streams
    int flags = FLAGS;
    if (SoapySDRDevice_activateStream(sdr, txStream, flags, transmitTime, contBufferTxLength) != 0)
    {
        printf("[TransmitReceive] Activate Tx stream fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_activateStream(sdr, rxStream, FLAGS_END, receiveTime, contBufferRxLength) != 0)
    {
        printf("[TransmitReceive] Activate Rx stream fail: %s\n", SoapySDRDevice_lastError());
    }

    // Begin transmitting
    int txStreamStatus;
    const int numTxBuffers = contBufferTxLength / bufferTxLength;
    const int transmitIncrement = bufferTxLength / SAMPLE_RATE_TX * 1e9;
    for (int i = 0; i < numTxBuffers; i++)
    {
        // If it is the last transmission change flags to empty buffer
        if (i == numTxBuffers)
        {
            flags = FLAGS_END;
        }

        buffsTx[0] = bufferTx;
        txStreamStatus = SoapySDRDevice_writeStream(sdr, txStream, buffsTx, bufferTxLength, &flags, transmitTime, STREAM_TIMEOUT_TX);
        transmitTime += transmitIncrement; // Begin next transmission immediately after this one finishes.
        bufferTx += bufferTxLength;        // Move buffer pointer to the next section of the contiguous buffer to be transmitted.

        if (txStreamStatus < 0)
        {
            printf("[TransmitReceive] Write stream failed: %s\n", SoapySDRDevice_lastError());
        }
        else if (txStreamStatus != bufferTxLength)
        {
            printf("[TransmitReceive] Write stream failed: %d\n", txStreamStatus);
        }
    }

    // Begin receiving
    int rxStreamStatus;
    const int numRxBuffers = contBufferRxLength / bufferRxLength;
    for (int i = 0; i < numRxBuffers; i++)
    {
        buffsRx[0] = bufferRx;
        rxStreamStatus = SoapySDRDevice_readStream(sdr, rxStream, buffsRx, bufferRxLength, &readFlags, &readTimestampRx, STREAM_TIMEOUT_RX); // flags and timestamp are read values

        if (i == 0)
        {
            printf("[TransmitReceive] First receive happened at %lf s\n", readTimestampRx / 1.0e9);
        }
        bufferRx += bufferRxLength; // Move buffer pointer to the next section of the contiguous buffer to be received.

        if (rxStreamStatus < 0) // Detect a failed chirp
        {
            printf("[TransmitReceive] Detected failed chirp on buffer chunk %d! Error message: %s. RX Flag: %#05x Filling buffer with zeros.\n", i, SoapySDR_errToStr(rxStreamStatus), readFlags);
            for (int j = 0; j < contBufferRxLength; j++)
            {
                contBufferRx[j] = 0 * (1 + I);
            }
            break; // Skip this chirp, move on to the next one.
        }
        else if (rxStreamStatus != bufferRxLength)
        {
            printf("[TransmitReceive] Read stream failed on buffer chunk %d. Samples captured: %d\n", i, rxStreamStatus);
        }

        if (i == 0)
        {
            firstRxTimestamp = readTimestampRx;
            *firstSampleIndex = bufferRxLength - rxStreamStatus + ZERO_DELAY_SAMPLES;
        }
    }
    // printf("[TransmitReceive] Rx timestamp: %f\n", firstRxTimestamp / 1e9);
}

void TrimBuffer(complex float *contBufferRx, int firstSampleIndex, size_t contBufferRxLength)
{
    printf("[TrimBuffer] Trimming first index to %d\n", firstSampleIndex);
    for (int i = 0; i < contBufferRxLength; i++)
    {
        if (i < contBufferRxLength - firstSampleIndex)
        {
            contBufferRx[i] = contBufferRx[i + firstSampleIndex];
        }
        else
        {
            contBufferRx[i] = 0;
        }
    }
}

void MixSignals(complex float *contBufferTx, complex float *contBufferRx, complex float *mixedSignal, const size_t contBufferRxLength)
{
    for (int i = 0; i < contBufferRxLength; i++)
    {
        mixedSignal[i] = (creal(contBufferTx[i]) + cimag(contBufferTx[i]) * I) * (creal(contBufferRx[i]) - cimag(contBufferRx[i]) * I);
    }
}
