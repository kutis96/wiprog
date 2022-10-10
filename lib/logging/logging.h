#ifndef LOGGING_H
#define LOGGING_H

#define DEBUG(X)           \
    Serial.print("dbug:"); \
    Serial.println(X)
#define INFO(X)            \
    Serial.print("info:"); \
    Serial.println(X)
#define WARN(X)            \
    Serial.print("warn:"); \
    Serial.println(X)

#define DEBUG_HEXVALUE(X, Y) \
    Serial.print("dbug:");   \
    Serial.print(X);         \
    Serial.print(Y, HEX)

#endif