#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

/*
    WiFiAmmeter frimware for ESP8266.
*/

/********************** Config **********************/
// Debug mode on/off
#define DEBUG_MODE 1

// WiFi access point
static constexpr const char *const WIFI_AP_SSID = "WiFi_Ammeter";
static constexpr const char *const WIFI_AP_PASS = "12345678";

// Web-server
static constexpr const unsigned int WEB_SERVER_LISTENING_PORT = 80;

// Pins
static constexpr const unsigned int PROTECTIVE_RELAY_PIN = 16;
static constexpr const unsigned int SWITCH_RELAY_PIN = 14;
static constexpr const unsigned int ANALOG_PIN = A0;

// Ampers
static constexpr const float MAX_AMPERE_VALUE = 30;
static constexpr const float SWITCH_AMPERE_VALUE = 5;
static constexpr const float EXCESS_AMPERE_VALUE = -1.0;
/******************** End config ********************/

// Debug out
#if DEBUG_MODE
#define DEBUG(msg) Serial.print(msg)
#define DEBUG_LN(msg) Serial.println(msg)
#else
#define DEBUG(msg)
#define DEBUG_LN(msg)
#endif

enum class SwitchRelayState
{
    LowState = 0,
    HighState
};

static constexpr const char *defaultContentType = "text/html";
static constexpr const char *crossOrigin = "Access-Control-Allow-Origin";

static ESP8266WebServer g_webServer(WEB_SERVER_LISTENING_PORT);
static bool g_runMeasure = false;
static float g_ampereValue = EXCESS_AMPERE_VALUE;
static SwitchRelayState g_SwitchRelayState = SwitchRelayState::LowState;

unsigned long test_cnt = 0; // test

void updateAmpereValue()
{
    /*static constexpr const unsigned int maxAnalogValue = 1023;
    const int currentValue =analogRead(ANALOG_PIN);
    if (g_SwitchRelayState == SwitchRelayState::LowState)
    {
        g_ampereValue = (float)(currentValue);
    }
    else // HIGH
    {
        g_ampereValue = (float)(currentValue);
    }*/
    // tets
    if (test_cnt++ == 5000)
    {
        g_ampereValue = MAX_AMPERE_VALUE+1;
        test_cnt = 0;
        return;
    }
    g_ampereValue = random(MAX_AMPERE_VALUE);
}

/* Relay controll */
void enableProtectiveRelay()
{
    digitalWrite(PROTECTIVE_RELAY_PIN, LOW);
}

void disableProtectiveRelay()
{
    digitalWrite(PROTECTIVE_RELAY_PIN, HIGH);
}

void setHighSwitchRelay()
{
    if (g_SwitchRelayState != SwitchRelayState::HighState)
    {
        digitalWrite(SWITCH_RELAY_PIN, HIGH);
        g_SwitchRelayState = SwitchRelayState::HighState;
        DEBUG_LN("Switch to high");
    }
}

void setLowSwitchRelay()
{
    if (g_SwitchRelayState != SwitchRelayState::LowState)
    {
        digitalWrite(SWITCH_RELAY_PIN, LOW);
        g_SwitchRelayState = SwitchRelayState::LowState;
        DEBUG_LN("Switch to low");
    }
}

/* WEB-API handlers */
void notFoundPage()
{
    g_webServer.send(404, defaultContentType, "");
}

void setCurrentNormalRqHandle()
{
    disableProtectiveRelay();
    g_runMeasure = true;
    g_webServer.sendHeader(crossOrigin, "*");
    g_webServer.send(200, defaultContentType, "");
}

void getValueRqHandle()
{
    char sendBuf[16];
    memset(sendBuf, 0, sizeof(sendBuf));
    sprintf(sendBuf, "%f", (float)g_ampereValue);

    g_webServer.sendHeader(crossOrigin, "*");
    g_webServer.send(200, defaultContentType, sendBuf);
}

void setup()
{
#if DEBUG_MODE
    Serial.begin(115200);
#endif
    DEBUG_LN("Device started");

    // Start WiFi access point
    DEBUG_LN("Try start WiFi access point...");
    if (WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS))
    {
        DEBUG_LN("success");
    }
    else
    {
        DEBUG("failed to start");
        return;
    }

    DEBUG("WifiAP SSID:");
    DEBUG_LN(WIFI_AP_SSID);

    DEBUG("WifiAP PASSWORD:");
    DEBUG_LN(WIFI_AP_PASS);

    // Config web server
    g_webServer.onNotFound(notFoundPage);

    // Registration handlers for API
    DEBUG_LN("Registration handlers for web API ...");

    g_webServer.on("/s", setCurrentNormalRqHandle);
    g_webServer.on("/v", getValueRqHandle);

    // Run web sever
    g_webServer.begin();
    DEBUG("Web-server started on port:");
    DEBUG_LN(WEB_SERVER_LISTENING_PORT);

    // Pin mode set
    pinMode(PROTECTIVE_RELAY_PIN, INPUT);
    pinMode(SWITCH_RELAY_PIN, INPUT);

    // Enable protective relay
    enableProtectiveRelay();

    // Sets switch relay on high value
    setHighSwitchRelay();
}

void loop()
{
    g_webServer.handleClient();

    if (!g_runMeasure)
        return;

    updateAmpereValue();

    // Over current -> enable protection
    if (g_ampereValue > MAX_AMPERE_VALUE)
    {
        DEBUG_LN("Over current");
        DEBUG_LN(g_ampereValue);

        enableProtectiveRelay();
        setHighSwitchRelay();
        g_runMeasure = false;
        g_ampereValue = EXCESS_AMPERE_VALUE;
        return;
    }

    // Set switch relay to low
    if (g_ampereValue < SWITCH_AMPERE_VALUE)
        setLowSwitchRelay();
    else
        setHighSwitchRelay();
}