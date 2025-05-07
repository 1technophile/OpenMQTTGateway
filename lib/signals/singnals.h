#ifndef SIGNALS_H
#define SIGNALS_H

#include <stdint.h>

#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation) || defined(ZgatewayRTL_433)

/**
   * Store signal values from RF, IR, SRFB or Weather stations so as to avoid duplicates
   */
void storeSignalValue(uint64_t MQTTvalue);

/**
   * get oldest time index from the values array from RF, IR, SRFB or Weather stations so as to avoid duplicates
   */
int getMin();

/**
   * Check if signal values from RF, IR, SRFB or Weather stations are duplicates
   */
bool isAduplicateSignal(uint64_t value);

#endif // defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation) || defined(ZgatewayRTL_433)

// Macros and structure to enable the duplicates removing on the following gateways
#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation) || defined(ZgatewayRTL_433)
// array to store previous received RFs, IRs codes and their timestamps
struct ReceivedSignal {
  uint64_t value;
  uint32_t time;
};

ReceivedSignal receivedSignal[] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};

#  define struct_size (sizeof(receivedSignal) / sizeof(ReceivedSignal))
#endif

#endif // SIGNALS_H
