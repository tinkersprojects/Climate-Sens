/**********************************************************************************************
 * Arduino Climate Sens Library - Version 1.0
 * by William Bailes <williambailes@gmail.com> http://tinkersprojects.com/
 *
 * This Library is licensed under a GPLv3 License
 **********************************************************************************************/



#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include <wiring.h> 
#endif

#include "ClimateSens.h"
#include "site.h"


bool SendData = false; // send data on start up
void IRAM_ATTR  triggerTimer();

/******************* SETUP *******************/

ClimateSens::ClimateSens( String _DeviceId, String _DeviceKey)
{
  DeviceKey = _DeviceKey;
  DeviceId = _DeviceId;
  
  setStatus(INIT);
}


void ClimateSens::setSettings(unsigned int _length, settingsFormat *_settings)
{
  settingsLength = _length;
  settings = _settings;
}

void ClimateSens::setData(unsigned int _length, dataFormat *_data)
{
  dataLength = _length;
  data = _data;
}

void ClimateSens::setLedCallback(CallbackStatusFunction _CallbackStatus)
{
  CallbackStatus = _CallbackStatus;
}

void ClimateSens::setStartCallback(CallbackFunction _startCallback)
{
  startCallback = _startCallback;
}




void ClimateSens::begin(String _ssid,String _password)
{
  WifiPassword = _password;
  begin(_ssid);
}
  

void ClimateSens::begin(String _ssid)
{

  setStatus(INIT);
  Serial.println("CS - begin");
  DeviceSsid = _ssid;
  

  EEPROM.begin(EEPROM_SIZE);
  load();

  bool status = false;

  delay(100);
  init();
  Serial.println("CS - init complete");
 
  #if defined(ESP32)
    webserver.reset(new WebServer(HTTP_PORT));
  #else
    webserver.reset(new ESP8266WebServer(HTTP_PORT));
  #endif

  webserver->on("/", std::bind(&ClimateSens::handleRoot, this));
  webserver->on("/get", std::bind(&ClimateSens::handleGetDetails, this));
  webserver->on("/get/details", std::bind(&ClimateSens::handleGetDetails, this));
  webserver->on("/get/data", std::bind(&ClimateSens::handleGetData, this));
  webserver->on("/get/settings", std::bind(&ClimateSens::handleGetSettings, this));
  webserver->on("/post/settings",HTTP_POST,std::bind(&ClimateSens::handlePostSettings, this));
  webserver->onNotFound(std::bind(&ClimateSens::handleNotFound, this));
  
  webserver->begin();
  Serial.println("CS - webserver complete");
  initTimer();
  Serial.println("CS - timers complete");
}


//******** EEPROM SAVE *********//

void ClimateSens::rest()
{
  Serial.println("CS - rest");
  save();
  ESP.restart();
}


void ClimateSens::save()
{
  Serial.println("CS - save");
  
  EEPROM.write(0, 100);
  EEPROM.write(1, settingsLength+CSsettingsLength);

  unsigned int count = 2;

    
  for (int i = 0; i < CSsettingsLength; i++)
  {
    if(CSsettings[i].name.length() > 100 || (*CSsettings[i].value).length() > 100)
      continue;
    
    
    Serial.print(CSsettings[i].name);
    Serial.print("- ");
    Serial.print(CSsettings[i].name.length());
    Serial.print(": ");
    Serial.print(*CSsettings[i].value);
    Serial.print("- ");
    Serial.println((*CSsettings[i].value).length());



    EEPROM.write(count, CSsettings[i].name.length());
    count++;
    for (int j = 0; j < CSsettings[i].name.length(); j++)
    {
      EEPROM.write(count, CSsettings[i].name.charAt(j));
      count++;
    }

    
    EEPROM.write(count, (*CSsettings[i].value).length());
    count++;
    for (int j = 0; j < (*CSsettings[i].value).length(); j++)
    {
      EEPROM.write(count, (*CSsettings[i].value).charAt(j));
      count++;
    }
  }



    
  for (int i = 0; i < settingsLength; i++)
  {
    if(settings[i].name.length() > 100 || (*settings[i].value).length() > 100)
      continue;

    
    Serial.print(settings[i].name);
    Serial.print("- ");
    Serial.print(settings[i].name.length());
    Serial.print(": ");
    Serial.print(*settings[i].value);
    Serial.print("- ");
    Serial.println((*settings[i].value).length());


    EEPROM.write(count, settings[i].name.length());
    count++;
    for (int j = 0; j < settings[i].name.length(); j++)
    {
      EEPROM.write(count, settings[i].name.charAt(j));
      count++;
    }

    
    EEPROM.write(count, (*settings[i].value).length());
    count++;
    for (int j = 0; j < (*settings[i].value).length(); j++)
    {
      EEPROM.write(count, (*settings[i].value).charAt(j));
      count++;
    }
  }
  
  EEPROM.commit();
}


void ClimateSens::load()
{
  if(EEPROM.read(0) != 100)
  {
    rest();
    return;
  }
  Serial.println("CS - load");

  unsigned int settingsCount = EEPROM.read(1)*2 + 1;
  Serial.println(settingsCount);

  int count = 2;
  for (int i = 0; i < settingsCount; i++)
  {
    byte lengthName = EEPROM.read(count);
    count++;
    
    String NameTemp = "";
    String ValueTemp = "";

    for (i = 0; i < lengthName; i++)
    {
      NameTemp += String((char)EEPROM.read(count));
      count++;
    }


    
    byte lengthValue = EEPROM.read(count);
    count++;

    for (i = 0; i < lengthValue; i++)
    {
      ValueTemp += String((char)EEPROM.read(count));
      count++;
    }

    
    Serial.print(NameTemp);
    Serial.print(" -");
    Serial.print(lengthValue);
    Serial.print(": ");
    Serial.print(ValueTemp);
    Serial.print(" -");
    Serial.println(lengthName);


    for (int i = 0; i < settingsLength; i++)
    {
      if(settings[i].name == NameTemp)
      {
        *settings[i].value = ValueTemp;
        break;
      }
    }
    

    for (int i = 0; i < CSsettingsLength; i++)
    {
      if(CSsettings[i].name == NameTemp)
      {
        *CSsettings[i].value = ValueTemp;
        break;
      }
    }



  }

}



//******** TIMERS *********//


void ClimateSens::initTimer()
{
  Serial.println("CS - begin timer");
 
 
  timer = timerBegin(0, (uint16_t)80, true);    //timer 0, div 80,000
  timerAttachInterrupt(timer, &triggerTimer, true);  //attach callback

  
  timerAlarmWrite(timer, uploadFreq, true); //set time in ms
  timerAlarmEnable(timer);                          //enable interrupt



  //timerWrite(timer, 0); //reset timer (feed watchdog)
}


void IRAM_ATTR triggerTimer()
{
  Serial.println("CS - Timer trigger");
  SendData = true;
}
        






/*********** HANDLE SERVER ******************/

void ClimateSens::handleRoot() 
{
  setStatus(ACTIVITY);
  Serial.println("CS - handleRoot");
  webserver->send ( 200, "text/html", index_html);
}

void ClimateSens::handleGetData() 
{
  setStatus(ACTIVITY);
  Serial.println("CS - handleGetData");
  
  String output = "[";

  //output += "{\"name\":\"Total Drunk\",\"value\":"+String(totalDrunk)+",\"subValues\":{\"lastDrink\":"+String(lastDrink)+",\"weightAvg\":"+String(weightAvg)+",\"weight\":"+String(weight)+"},\"unit\":\"L\"}";
  //output += "{\"name\":\"weight\",\"value\":"+String(weight)+",\"subValues\":{\"sensorValueA\":"+String(sensorValueA)+",\"sensorValueB\":"+String(sensorValueB)+",\"sensorValueC\":"+String(sensorValueC)+",\"sensorValueD\":"+String(sensorValueD)+"},\"unit\":\"L\"}";
  
  
  for (int i = 0; i < dataLength; i++)
  {
    
    if(i > 0)
    output += ",";

    output += "{\"name\":\""+data[i].name+"\",\"unit\":\""+data[i].unit+"\",\"value\":\""+String(*data[i].value)+"\",\"subValues\":{";
    
    for (int k = 0; k < data[i].subDatacount; k++)
    {
      if(k > 0)
        output += ",";
      
      output += "\""+data[i].subData[k].name+"\":"+String(*data[i].subData[k].value);
    }
    output += "}}";
  }
    
  output += "]";

  webserver->send(200, "application/json",output);
}

void ClimateSens::handleGetDetails() 
{
  setStatus(ACTIVITY);
  Serial.println("CS - handleGetDetails");
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  //chip_info.revision

  String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();
  
  String firmwareVersion = "V1";
  String DeviceName = "";
  String chipRevision = ""; //chip_info.revision;

  String output = "{\"name\":\""+DeviceName+"\",\"LibraryVersion\":\""+LibraryVersion+"\",\"firmwareVersion\":\""+firmwareVersion+"\",\"chipId\":\""+chipId+"\",\"chipRevision\":\""+chipRevision+"\"}";
  
  webserver->send(200, "application/json",output);
}

void ClimateSens::handleGetSettings() 
{
  setStatus(ACTIVITY);
  Serial.println("CS - handleGetSettings");
  String output = "[";

  
  for (int i = 0; i < CSsettingsLength; i++)
  {
    if(i > 0)
      output += ",";

    output += "{\"name\":\""+CSsettings[i].name+"\",\"type\":\"";
    
    switch (CSsettings[i].type)
    {
    case NUMBER:
      output += "number";
        break;
      
    case CHECKBOX:
      output += "checkbox";
      break;

    case SELECT:
      output += "select";
      break;

    case TEXT:
      output += "text";
      break;
    
    default:
      output += "";
      break;
    }
    
    output += "\", \"value\":\""+(*CSsettings[i].value)+"\"}";

  }

  for (int i = 0; i < settingsLength; i++)
  {
    
    if(CSsettingsLength > 0 || i > 0)
      output += ",";
 
    if(settings[i].name == "WiFi Password")
    {
      output += "{\"name\":\""+settings[i].name+"\",\"type\":\""+settings[i].type+"\", \"value\":\"\"}";
      continue;
    }

    output += "{\"name\":\""+settings[i].name+"\",\"type\":\""+settings[i].type+"\", \"value\":\""+(*settings[i].value)+"\"}";

  }


  
  output += "]";
  webserver->send(200, "application/json",output);
}


void ClimateSens::handlePostSettings() 
{
  setStatus(ACTIVITY);
  Serial.println("CS - handlePostSettings");

      Serial.print("body: ");
      Serial.println(webserver->arg("plain"));

      //StaticJsonDocument<512> doc;
      DynamicJsonDocument doc(512);
      DeserializationError error =  deserializeJson(doc, webserver->arg("plain"));

      if (error) 
      {
        webserver->send(200, "application/json","{success:false}");  
        return;
      }

      JsonObject obj = doc.as<JsonObject>();
      

      bool found = false;

      for (int i = 0; i < CSsettingsLength; i++)
      {
        if(obj.containsKey(CSsettings[i].name))
        {
          found = true;
          JsonVariant setting = obj.getMember(CSsettings[i].name);
          *CSsettings[i].value = setting.as<String>(); 
        }
      }
      
      for (int i = 0; i < settingsLength; i++)
      {
        if(obj.containsKey(settings[i].name))
        {
          found = true;
          JsonVariant setting = obj.getMember(settings[i].name);
          *settings[i].value = setting.as<String>(); 
        }
      }

      if(found)
      {
        save();
        webserver->send(200, "application/json","{success:true}");
        delay(1000);
        ESP.restart();
        delay(10000);
      }
      else
      {
        webserver->send(200, "application/json","{success:false}"); 
      }
}

void ClimateSens::handleNotFound() 
{
  Serial.println("CS - handleNotFound");
  handleRoot();
}





/******************* CONNECT *******************/


void ClimateSens::init()
{
  Serial.println("CS - initAP");
  accessPointAcctive = true;

  //disconnect();
  delay(200);

  WiFi.mode(WIFI_AP_STA);

  if(DevicePassword == "")
  {
    Serial.println("CS - no password");
    Serial.println(DeviceSsid);
    WiFi.softAP(DeviceSsid.c_str());
    Serial.println(DeviceSsid.c_str());
  }
  else
  {
    Serial.println("CS - password");
    Serial.println(DeviceSsid);
    WiFi.softAP(DeviceSsid.c_str(),DevicePassword.c_str());
  }

  
  if(WifiSsid == "")
  {
    Serial.println("CS - WIFI not set");
    return ;
  }

  if(WifiPassword == "")
  {
    Serial.println("CS - no password");
    Serial.println(WifiSsid.c_str());
    WiFi.begin(WifiSsid.c_str());
  }
  else{
    Serial.println("CS - password");
    Serial.println(WifiSsid.c_str());
    WiFi.begin(WifiSsid.c_str(),WifiPassword.c_str());
  }
    
    delay(500);


  for (int i = 0; i < 40; i++)
  {
    if(WiFi.status() == WL_CONNECTED) 
    {
      Serial.println("CS - Connected");
      setStatus(CONNECTION);
      return ;
    }
    delay(500);
  }

  
  setStatus(ERROR);
  error = "Could not connet to WiFi.";

  
  //domainNameSever = DNSServer();
  //domainNameSever.setErrorReplyCode(DNSReplyCode::NoError);
  //domainNameSever.start(DNS_PORT, F("*"), WiFi.softAPIP());
  
}


void ClimateSens::disconnect()
{  
  Serial.println("CS - disconnect");
  WiFi.softAPdisconnect();
  WiFi.disconnect();
}




/******************* HANDLE and SEND *******************/



void ClimateSens::handleCS()
{
  if(timerAlarmEnabled(timer) && timerReadSeconds(timer) >= uploadFreq)
  {
    SendData = true;
  }


  if(SendData)
  {
    SendData = false;
    send();
  }
  
  if(status != ERROR)
    setStatus(IDEL);
  
  //if(timer timeout -> send data) send();

  webserver->handleClient();
/*
  if(CallbackStatus != NULL)
  {
    CallbackStatus(status);
  }*/
}



int ClimateSens::send()
{
  setStatus(CONNECTION);
  Serial.println("CS - send");
  //make data string
  String PostData = "{";
  //\"Temperature\":\""+Temperature+"\",\"Pressure\":\""+Pressure+"\",\"Altitude\":\""+Altitude+"\",\"Humidity\":\""+Humidity+"\"}}";
  
  for (int i = 0; i < dataLength; i++)
  {
    if(i > 0)
      PostData += ",";

    PostData += "\""+data[i].name+"\":"+String(*data[i].value);
    
    for (int k = 0; k < data[i].subDatacount; k++)
    {
      PostData += ",\""+data[i].name+" - "+data[i].subData[k].name+"\":"+String(*data[i].subData[k].value);
    }
  }
    
  PostData += "}";



  Serial.println(PostData);

  startCallback();

  if(WifiSsid == "")
    return 0;
    
  if(WiFi.status() != WL_CONNECTED)
  {
    Serial.println("CS - WIFI not Connected");
    return 0;
  }

    
    HTTPClient http;
    if(http.begin(URL))
    {
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Accept", "application/json");
      http.addHeader("X-API-Key", DeviceId);
      http.addHeader("Authorization", DeviceKey);
      http.addHeader("HTTP_ACCEPT", "application/json");
  
      int httpCode = http.POST(PostData);

       if (httpCode > 0)
       {
          Serial.printf("[HTTP] POST... code: %d\n", httpCode);
          Serial.println(http.getString());
          if(httpCode == HTTP_CODE_OK) 
          {
              /*DeserializationError error = deserializeJson(doc, http.getString());
              if (error) 
              {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.c_str());
                return false;
              }*/ 
              http.end();
              return true;
              
              /*if(doc.containsKey("success") && doc["success"].as<bool>() == true)
              {
                Serial.println("success");
                http.end();
                return true;
              }            
              http.end();*/
          }
          else
          {
            error = "HTTP code: "+String(httpCode);
            setStatus(ERROR);
          }
      } 
      else 
      {
          Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
          Serial.println(http.getString());
      } 
      http.end();
    } 
    else 
    {
      Serial.printf("[HTTPS] Unable to connect\n");
      error = "Unable to connect to server";
      setStatus(ERROR);
    }

    
    return 0;
}














/************** OTHER ******************/



void ClimateSens::setStatus(Status_Type _status)
{

  if(status != _status)
  {
    Serial.print("CS - setStatus: ");
    Serial.println(_status);
  }
  
  status = _status;
  if(CallbackStatus != NULL)
  {
    CallbackStatus(status);
  }
}
   
