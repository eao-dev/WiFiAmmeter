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

// Ampers
static constexpr const unsigned int MAX_AMPERE_VALUE = 30;
static constexpr const unsigned int SWITCH_AMPERE_VALUE = 5;
static constexpr const unsigned int EXCESS_AMPERE_VALUE = -1;
/******************** End config ********************/

// Debug out
#if DEBUG_MODE
#define DEBUG(msg) Serial.print(msg)
#define DEBUG_LN(msg) Serial.println(msg)
#else
#define DEBUG(msg)
#define DEBUG_LN(msg)
#endif

static constexpr const char *defaultContentType = "text/html";

static ESP8266WebServer webServer(WEB_SERVER_LISTENING_PORT);
static bool runMeasure = false;

void pushAmpereToQueue(unsigned int value)
{
    // TODO: push to queue
}

unsigned int getAmpereValue()
{
    // TODO: implement this function
    return 1;
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
    digitalWrite(SWITCH_RELAY_PIN, HIGH);
}

void setLowSwitchRelay()
{
    digitalWrite(SWITCH_RELAY_PIN, LOW);
}

/* WEB-API handlers */
void rootRqHandle()
{
    webServer.send(200, "text/html", "<h1>Main</h1>");
}

void notFoundPage()
{
    webServer.send(404, defaultContentType, "");
}

void setCurrentNormalRqHandle()
{
    disableProtectiveRelay();
    runMeasure = true;
    webServer.send(200, defaultContentType, "");
}

void getValueRqHandle()
{
    // TODO: pop from queue and send
    char sendBuf[16];
    memset(sendBuf, 0, sizeof(sendBuf));
    ltoa(millis(), sendBuf, 10);
    webServer.send(200, defaultContentType, sendBuf);
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

    DEBUG_LN("WifiAP SSID:");
    DEBUG(WIFI_AP_SSID);

    // Config web server;
    webServer.on("/", rootRqHandle);
    webServer.onNotFound(notFoundPage);

    // Registration handlers for API
    DEBUG_LN("Registration handlers for web API ...");

    webServer.on("/s", setCurrentNormalRqHandle);
    webServer.on("/v", getValueRqHandle);

    // Run web sever
    webServer.begin();
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
    webServer.handleClient();

    if (!runMeasure)
        return;

    unsigned int current = getAmpereValue();
    pushAmpereToQueue(current);

    // Over current -> enable protection
    if (current > MAX_AMPERE_VALUE)
    {
        enableProtectiveRelay();
        setHighSwitchRelay();
        runMeasure = false;
        return;
    }

    // Set switch relay to low
    if (current < SWITCH_AMPERE_VALUE)
    {
        setLowSwitchRelay();

        do
        {
            current = getAmpereValue();
            pushAmpereToQueue(current);
            webServer.handleClient();
        } while (current < SWITCH_AMPERE_VALUE);
    }
}