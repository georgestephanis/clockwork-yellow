#include "network_time.h"
#include <WiFi.h>
#include <time.h>
#include "config.h"

// Internal connection states
enum NetState { 
    STATE_DISCONNECTED, 
    STATE_CONNECTING, 
    STATE_CONNECTED, 
    STATE_SYNCING, 
    STATE_READY, 
    STATE_FAILED 
};

static NetState current_state = STATE_DISCONNECTED;
static bool wifi_connected = false;
static bool time_synced = false;
static uint32_t connection_start_time = 0;
static uint32_t last_ntp_sync_ms = 0;

void initNetwork() {
    if (WiFi.status() == WL_CONNECTED) {
        wifi_connected = true;
        current_state = STATE_CONNECTED;
        return;
    }

    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    connection_start_time = millis();
    current_state = STATE_CONNECTING;
    wifi_connected = false;
    time_synced = false;
}

void updateNetworkState() {
    uint32_t now = millis();

    switch (current_state) {
        case STATE_DISCONNECTED:
            initNetwork();
            break;

        case STATE_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                wifi_connected = true;
                current_state = STATE_CONNECTED;
                Serial.println("\nWiFi Connected!");
                Serial.print("IP Address: ");
                Serial.println(WiFi.localIP());
            } else if (now - connection_start_time >= WIFI_TIMEOUT_MS) {
                wifi_connected = false;
                current_state = STATE_FAILED;
                connection_start_time = now; // Start backoff timer
                Serial.println("\nWiFi Connection Failed (Timeout).");
            }
            break;

        case STATE_CONNECTED:
            Serial.print("Configuring NTP with server: ");
            Serial.println(NTP_SERVER);
            configTime(0, 0, NTP_SERVER);
            connection_start_time = now; // Start NTP sync timer
            current_state = STATE_SYNCING;
            break;

        case STATE_SYNCING:
            {
                time_t now_time;
                time(&now_time);
                struct tm timeinfo;
                gmtime_r(&now_time, &timeinfo);

                // tm_year is years since 1900. If synced, it should be > 120 (Year > 2020)
                if (timeinfo.tm_year > 120) {
                    time_synced = true;
                    current_state = STATE_READY;
                    last_ntp_sync_ms = now;
                    Serial.println("Time synchronized successfully via NTP!");
                } else if (now - connection_start_time >= 15000) { // 15-second NTP timeout
                    time_synced = false;
                    current_state = STATE_FAILED;
                    connection_start_time = now; // Start backoff timer
                    Serial.println("Time synchronization failed (Timeout).");
                }
            }
            break;

        case STATE_READY:
            // Continuous connectivity check
            if (WiFi.status() != WL_CONNECTED) {
                wifi_connected = false;
                time_synced = false;
                current_state = STATE_DISCONNECTED;
                Serial.println("WiFi connection lost.");
            }
            break;

        case STATE_FAILED:
            // Non-blocking 15-second backoff before retrying connection
            if (now - connection_start_time >= 15000) {
                Serial.println("Retrying WiFi connection...");
                initNetwork();
            }
            break;
    }
}

bool isWiFiConnected() {
    return wifi_connected;
}

bool isTimeSynced() {
    return time_synced;
}

void triggerManualSync() {
    Serial.println("Manual sync triggered asynchronously.");
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect();
        initNetwork();
    } else {
        time_synced = false;
        current_state = STATE_CONNECTED; // Force re-sync
    }
}

void checkNtpPeriodic(uint32_t intervalSec) {
    if (current_state == STATE_READY) {
        uint32_t now = millis();
        if (now - last_ntp_sync_ms >= intervalSec * 1000) {
            Serial.println("Periodic background NTP sync triggered.");
            time_synced = false;
            current_state = STATE_CONNECTED; // Triggers re-sync
        }
    }
}
