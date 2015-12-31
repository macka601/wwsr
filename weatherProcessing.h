#ifndef WEATHERPROCESSING_H
#define WEATHERPROCESSING_H


void windDirection(char *windDirBuf, int windByte, int returnAsDegrees);
float getTemperature(uint8_t *byte, int insideTemp, int asImperial);
float getWindSpeed(uint8_t *byte);
float getWindGust(int windGustByte);
float getAbsPressure(uint8_t *byte);
float getRelPressure(uint8_t *byte);
unsigned char  getHumidity(unsigned char byte);
float getDewPoint(float temperature, float humidity);
float getWindChill(uint8_t *byte, int asImperial);
float getLastHoursRainFall(uint8_t *byteCurrent, uint8_t *byteHourly, int asImperial);
float getLast24HoursRainFall(uint8_t *byteCurrent, uint8_t *byte24Hour, int asImperial);
float getTotalRainFall(uint8_t *byte, int asImperial);

extern int logType;

#endif /* end of header guard */
