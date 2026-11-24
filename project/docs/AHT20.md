# AHT20 Humidity and Temperature Sensor

Following are relevant details extracted directly from the [AHT20 Datasheet (V1.0)](https://cdn-learn.adafruit.com/assets/assets/000/123/394/original/Data_Sheet_AHT20.pdf) for easy reference.

## Temperature Conversion

The temperature $T$ can be calculated by substituting the temperature output signal $S_{T}$ into the following formula:

$$
T[\degree C] = (\frac{S_{T}}{2^{20}}) \times 200 - 50
$$

## Relative Humidity Conversion

The relative humidity $RH$ can be calculated according to the relative humidity signal $S_{RH}$ output by serial data ($SDA$ over $I^2C$) through the following formula (the result is expressed in $\%RH$):

$$
RH[\%] = (\frac{S_{RH}}{2^{20}}) \times 100\%
$$
