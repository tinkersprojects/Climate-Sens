#include <ClimateSens.h>

#define GreenLedPin 5
#define RedLedPin 6
#define TempSensorPin 1
#define ButtonPin 1

#define multiple 1

String DeviceId   = "DeviceId";
String DeviceKey  = "DeviceKey";

ClimateSens csAccess(DeviceId,DeviceKey);

double temp;
double tempMax;
double tempMin;
double tempAvg;
double tempAvgtotal;
unsigned int tempAvgCounter = 0;
bool StartNew = true;

String location;
String ledPower = "true";
String ledActivity = "true";
String ledConnection = "true";
String ledError = "true";

void LED(Status_Type status);

dataSubFormat subData[3] = {{"max",&tempMax},{"min",&tempMin},{"avg",&tempAvg}};

dataFormat data[1] = {{"Temp","C", &temp,3,subData}};

settingsFormat settings[5] = {
                    {"Location",TEXT,&location},
                    {"LED power",CHECKBOX,&ledPower},
                    {"LED activity",CHECKBOX,&ledActivity},
                    {"LED connection",CHECKBOX,&ledConnection},
                    {"LED error",CHECKBOX,&ledError}
                  };

void LED(Status_Type status);
void clear();


void setup()
{
  Serial.begin(115200);

  pinMode(ButtonPin,INPUT);
  pinMode(TempSensorPin,INPUT);
  pinMode(GreenLedPin,OUTPUT);
  pinMode(RedLedPin,OUTPUT);
  LED(csAccess.status);

  csAccess.setData(1,data);
  csAccess.setSettings(5,settings);
  csAccess.setLedCallback(LED);
  csAccess.setStartCallback(clear);
  csAccess.begin("Temp");

}

void loop()
{
  temp = analogRead(1) * multiple;

  if(StartNew)
  {
    StartNew = false;
    tempMax = temp;
    tempMin = temp;
    tempAvg = temp;
    tempAvgtotal = temp;
    tempAvgCounter = 1;
  }
  else
  {
    if(tempMax < temp)
      tempMax = temp;

    if(tempMin < temp)
      tempMin = temp;

    tempAvgCounter++;
    tempAvgtotal += temp;
    tempAvg = temp / tempAvgCounter;
  }


  delay(10);

  LED(csAccess.status);
  csAccess.handleCS();
}

void LED(Status_Type status)
{
  digitalWrite(GreenLedPin,LOW);
  digitalWrite(RedLedPin,LOW);

  switch (csAccess.status)
  {
    case INTI:
      digitalWrite(GreenLedPin,HIGH);
      digitalWrite(RedLedPin,HIGH);
      break;

    case IDEL:
      if(ledPower == "true")
      {
        digitalWrite(GreenLedPin,HIGH);
      }
      break;

    case ACTIVITY:
    
      if(ledActivity == "true")
      {
        digitalWrite(RedLedPin,HIGH);
      }

      break;

    case CONNECTION:
    
      if(ledConnection == "true")
      {
        digitalWrite(GreenLedPin,HIGH);
        digitalWrite(RedLedPin,HIGH);
        delay(300);
        digitalWrite(RedLedPin,LOW);
      }
      break;

    case ERROR:
    
      if(ledError == "true" && millis()%600 >300)
      {
        digitalWrite(RedLedPin,HIGH);
      }
      break;
  }
}

void clear()
{
  StartNew = true;
}
