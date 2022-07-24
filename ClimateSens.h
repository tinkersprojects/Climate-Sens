#ifndef CLIMATESENS_h
#define CLIMATESENS_h

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#include <wiring.h>
#endif

#ifdef ESP32
    // ESP32 WIFI stack
    #include <WiFi.h>
    #include <WebServer.h>
    #include <DNSServer.h>
    #include <ESPmDNS.h>
    #include <WiFiAP.h>
    #include <WiFiClient.h>
    #include <HTTPClient.h>
#else
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
    #include <WiFiClientSecureBearSSL.h>
     #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
    #include <ESP8266mDNS.h> 
    #include <DNSServer.h>

#endif


#include <EEPROM.h>
#include <ArduinoJson.h>

#define DNS_PORT 53
#define HTTP_PORT 80
#define CSsettingsLength 3
#define EEPROM_SIZE 512
#define URL "https://connect.climatesens.com"
#define LibraryVersion "V1"



enum Status_Type {
    INIT,           // setting up system
    IDEL,           // system reading and not sending data
    ACTIVITY,       // using AP
    CONNECTION,     // sending data to server
    ERROR           // error!!!
};

enum Field_Type {
    TEXT,
    NUMBER,
    SELECT,
    CHECKBOX
};

struct dataSubFormat {
    String name;
    double *value;
};

struct dataFormat {
    String name;
    String unit;
    double *value;
    int subDatacount;
    dataSubFormat *subData;
};

struct settingsFormat {
    String name;
    Field_Type type;
    String *value;
};

typedef void (*CallbackFunction) ();
typedef void (*CallbackStatusFunction) (Status_Type status);



class ClimateSens
{
    public:
        // SETUP 
        ClimateSens(String _DeviceId,String _DeviceKey);

        void setSettings(unsigned int _length, settingsFormat *_settings);
        void setData(unsigned int _length, dataFormat *_data);
        void setLedCallback(CallbackStatusFunction _CallbackStatus);
        void setStartCallback(CallbackFunction _startCallback);

        void begin(String _ssid);
        void begin(String _ssid,String _password);

        void init();      //allows phone to connect to device
        
        void rest();
        void load();
        void save();

        void disconnect();
        
        void handleCS();
        
        void initTimer();
        void setTimer(unsigned long seconds);

        int send();

        // fix
        String error;
        String DeviceSsid = "Access-Point";
        String DevicePassword = "";
        String WifiSsid = "";
        String WifiPassword = "";
        Status_Type status;
    private:
        String DeviceKey;
        String DeviceId;

        //DNSServer domainNameSever;
        //CallbackFunction configWebServerCallBack = NULL;
        CallbackStatusFunction CallbackStatus = NULL;
        CallbackFunction startCallback = NULL;

        hw_timer_t * timer = NULL;
        unsigned long uploadFreq = 1800000000; //60*30*1000000; //3600; // in uS

        #if defined(ESP32)
            std::unique_ptr<WebServer> webserver;
        #else
            std::unique_ptr<ESP8266WebServer> webserver;
        #endif
        
        bool accessPointAcctive = true;

        unsigned int dataLength;
        unsigned int settingsLength;
        dataFormat *data;
        settingsFormat *settings;
        settingsFormat CSsettings[CSsettingsLength] = {
                    {"Name",TEXT,&DeviceSsid},
                    {"WiFi SSDI",TEXT,&WifiSsid},
                    {"WiFi Password",TEXT,&WifiPassword}
        };

        void setStatus(Status_Type _status);

        void handleNotFound();
        void handleRoot();
        void handleGetData();
        void handleGetDetails();
        void handleGetSettings();
        void handlePostSettings();
};


#endif
