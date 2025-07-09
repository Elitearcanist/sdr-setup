#Written by Levi Powell, August 2023
#This code plots the data saved by the SDR


import time
import array
import matplotlib.pyplot as plt
import numpy as np
from numpy.fft import fft
from numpy.fft import fftshift

f_config_name = 'config.txt'
f_data_name = 'data.bin'
f_nulldata_name = 'nulldata.bin'
window = True # apply a hanning window to the data
fftLength = 100
intData = False
lockAxis = True # Sets axis to fixed values



print("Reading configuration file...")

f_config = open(f_config_name, 'r')
line = f_config.read()
row = line.split(',')
sampleRate = int(row[0])
bandwidth = int(row[1])
numSamples = int(row[2])
numChirps = int(row[3])
chirpDelay = int(row[4])

print(f"\tSample rate was set to {sampleRate / 1e6} MS/s")
print(f"\tBandwidth was set to {bandwidth / 1e6} MHz")
print(f"\tNumber of samples was set to {numSamples}")
print(f"\tNumber of chirps was set to {numChirps}")

dataMatrix = np.empty([numChirps,numSamples], np.csingle)
realMatrix = np.empty([numChirps,numSamples], np.float32)
FFTMagMatrix = np.empty([numChirps,fftLength], np.float32)

dataMatrix2 = np.empty([numChirps,numSamples], np.csingle)
realMatrix2 = np.empty([numChirps,numSamples], np.float32)
FFTMagMatrix2 = np.empty([numChirps,fftLength], np.float32)

realMatrixDiff = np.empty([numChirps,numSamples], np.float32)
FFTMagMatrixDiff = np.empty([numChirps,fftLength], np.float32)



print("Opening data files...")
f_nulldata = open(f_nulldata_name, 'rb')
f_data = open(f_data_name, 'rb')

windowArray = np.float32(np.hanning(numSamples))



print(f"Reading {f_nulldata_name}...")
startTime = time.time()

for row in range(numChirps):
    if intData:
        reals = array.array('i')
        imags = array.array('i')
    else:
        reals = array.array('f')
        imags = array.array('f')
    reals.fromfile(f_nulldata, numSamples)
    imags.fromfile(f_nulldata, numSamples)
    reals = np.float32(reals)
    imags = np.float32(imags)

    dataMatrix[row] = reals + 1j*imags
    realMatrix[row] = reals

    del reals, imags
if window:
    dataMatrix = dataMatrix * windowArray
    realMatrix = realMatrix * windowArray

endTime = time.time()
print(f"\tTotal time was {endTime - startTime} s")



print(f"Calculating FFT for {f_nulldata_name}...")
startTime = time.time()

#calculate FFT
for i in range(len(dataMatrix)):
    sr = sampleRate
    
    X = fftshift(fft(dataMatrix[i]))
    X = X[numSamples//2 - fftLength//2:numSamples//2 + fftLength//2]

    FFTMagMatrix[i] = np.abs(X)
    FFTMagMatrix[i] = 10*np.log10(FFTMagMatrix[i]) # TODO sometimes numbers are 0 here

del dataMatrix

endTime = time.time()
print(f"\tTotal time was {endTime - startTime} s")



print(f"Reading {f_data_name}...")
startTime = time.time()

for row in range(numChirps):
    if intData:
        reals = array.array('i')
        imags = array.array('i')
    else:
        reals = array.array('f')
        imags = array.array('f')
    reals.fromfile(f_data, numSamples)
    imags.fromfile(f_data, numSamples)
    reals = np.float32(reals)
    imags = np.float32(imags)

    dataMatrix2[row] = reals + 1j*imags
    realMatrix2[row] = reals

    del reals, imags
if window:
    dataMatrix2 = dataMatrix2 * windowArray
    realMatrix2 = realMatrix2 * windowArray

endTime = time.time()
print(f"\tTotal time was {endTime - startTime} s")



print(f"Calculating FFT for {f_data_name}...")
startTime = time.time()

#calculate FFT
for i in range(len(dataMatrix2)):
    sr = sampleRate
    
    X = fftshift(fft(dataMatrix2[i]))
    X = X[numSamples//2 - fftLength//2:numSamples//2 + fftLength//2]

    FFTMagMatrix2[i] = np.abs(X)
    FFTMagMatrix2[i] = 10*np.log10(FFTMagMatrix2[i])

del dataMatrix2

endTime = time.time()
print(f"\tTotal time was {endTime - startTime} s")

del windowArray



print(f"Calculating average FFT of {f_nulldata_name}...")
startTime = time.time()

# Average the targetless chirps in FFT
for i in range(fftLength):
    FFTMagMatrix[:,i] = np.average(FFTMagMatrix[:,i])

endTime = time.time()
print(f"\tTotal time was {endTime - startTime} s")



print(f"Calculating difference matrix...")
startTime = time.time()

realMatrixDiff = realMatrix2 - realMatrix
FFTMagMatrixDiff = FFTMagMatrix2 - FFTMagMatrix

endTime = time.time()
print(f"\tTotal time was {endTime - startTime} s")



print("Plotting...")

if intData:
    vmaxT = '2e6'
    vminT = '-2e6'
    vmaxF = '20'
    vminF = '-20'
else:
    vmaxT = '0.01'
    vminT = '-0.01'
    vmaxF = '20'
    vminF = '-20'

subplot1 = 220
subplot2 = 210

plt.figure(1)

# Plot the two received signals (before subtraction)
plt.subplot(subplot1+1)
# plt.subplot(221)
plt.title('Null Data')
if lockAxis:
    plot = plt.matshow(realMatrix, cmap='hot', vmax=vmaxT, vmin=vminT, aspect="auto", fignum=False)
else:
    plot = plt.matshow(realMatrix, cmap='hot', aspect="auto", fignum=False)
plt.colorbar(plot)
plt.xlabel('Sample Number')
plt.ylabel('Chirp Number')

plt.subplot(subplot1+2)
plt.title('Sample Data')
if lockAxis:
    plot = plt.matshow(realMatrix2, cmap='hot', vmax=vmaxT, vmin=vminT, aspect="auto", fignum=False)
else:
    plot = plt.matshow(realMatrix2, cmap='hot', aspect="auto", fignum=False)
plt.colorbar(plot)
plt.xlabel('Sample Number')
plt.ylabel('Chirp Number')

plt.subplot(subplot1+3)
if lockAxis:
    plot = plt.matshow(FFTMagMatrix, cmap='hot', vmax=vmaxF, vmin=vminF, aspect="auto", fignum=False)
else:
    plot = plt.matshow(FFTMagMatrix, cmap='hot', aspect="auto", fignum=False)
plt.colorbar(plot)
plt.xlabel('Frequency Bin')
plt.ylabel('Chirp Number')

plt.subplot(subplot1+4)
if lockAxis:
    plot = plt.matshow(FFTMagMatrix2, cmap='hot', vmax=vmaxF, vmin=vminF, aspect="auto", fignum=False)
else:
    plot = plt.matshow(FFTMagMatrix2, cmap='hot', aspect="auto", fignum=False)
plt.colorbar(plot)
plt.xlabel('Frequency Bin')
plt.ylabel('Chirp Number')

# Free up memory
del FFTMagMatrix, FFTMagMatrix2
del realMatrix, realMatrix2



# Plot the signals after subtraction
plt.figure(2)
plt.subplot(subplot2+1)
plt.title('Difference')
if lockAxis:
    plot = plt.matshow(realMatrixDiff, cmap='hot', vmax=vmaxT, vmin=vminT, aspect="auto", fignum=False)
else:
    plot = plt.matshow(realMatrixDiff, cmap='hot', aspect="auto", fignum=False)
plt.colorbar(plot)
plt.xlabel('Sample Number')
plt.ylabel('Chirp Number')

plt.subplot(subplot2+2)
if lockAxis:
    plot = plt.matshow(FFTMagMatrixDiff, cmap='hot', vmax=vmaxF, vmin=vminF, aspect="auto", fignum=False)
else:
    plot = plt.matshow(FFTMagMatrixDiff, cmap='hot', aspect="auto", fignum=False)
plt.colorbar(plot)
plt.xlabel('Frequency Bin')
plt.ylabel('Chirp Number')

# Free up memory
del FFTMagMatrixDiff
del realMatrixDiff

plt.show()



f_config.close()
f_nulldata.close()
f_data.close()
print("Done")