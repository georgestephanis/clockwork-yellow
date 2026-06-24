#ifndef NETWORK_TIME_H
#define NETWORK_TIME_H

#include <Arduino.h>

/**
 * Initializes WiFi interface and initiates an asynchronous connection request.
 * Does not block.
 */
void initNetwork();

/**
 * Periodically updates the non-blocking network and NTP synchronization state machine.
 * Must be polled regularly in the main loop.
 */
void updateNetworkState();

/**
 * Returns whether the device is currently connected to WiFi.
 * 
 * @return true if connected, false otherwise.
 */
bool isWiFiConnected();

/**
 * Returns whether the system clock has been synchronized via NTP.
 * 
 * @return true if synchronized, false otherwise.
 */
bool isTimeSynced();

/**
 * Asynchronously triggers a manual WiFi reconnection and NTP synchronization.
 * Does not block.
 */
void triggerManualSync();

/**
 * Periodically triggers background NTP time syncs (e.g., every hour) non-blockingly.
 * 
 * @param intervalSec Interval in seconds between background syncs.
 */
void checkNtpPeriodic(uint32_t intervalSec);

#endif // NETWORK_TIME_H
