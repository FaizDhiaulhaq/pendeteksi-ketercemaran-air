#include <OneWire.h>
#include <DallasTemperature.h>
#include<Timer.h>
#include <TinyGPS.h>


Timer t;

const int pinDs=2,pinTurbidity=A2,pinPh=A11; 

OneWire ourWire(pinDs);
TinyGPS gps;

DallasTemperature sensors(&ourWire);

#define Offset 0.00            //deviation compensate
#define LED 13
#define samplingInterval 20
#define printInterval 800
#define ArrayLenth  40    //times of collection
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex=0;



char *api_key="E8LUTR7GVQ7ABF4K";     // Enter your Write API key from ThingSpeak
static char postUrl[150];

void httpGet(String ip, String path, int port=80);
char LAT[20],LON[20];
float tem,tem1,tem2,ph,kekeruhan;

void setup(void)
{
  pinMode(LED,OUTPUT);
  Serial.begin(9600);  
  Serial1.begin(9600);
  Serial2.begin(9600);
 Serial.println("Connecting Wifi....");
 connect_wifi("AT",1000);
 connect_wifi("AT+CWMODE=1",1000);
 connect_wifi("AT+CWQAP",1000);  
 connect_wifi("AT+RST",5000);

 connect_wifi("AT+CWJAP=\"UMGdanau\",\"danaulimboto\"",10000);
 Serial.println("Wifi Connected"); 
  delay(2000);
}
void loop(void)
{
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float pHValue,voltage;
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;    
  //for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (Serial2.available())
    {
      char c = Serial2.read();
      Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }  if (newData)
  {
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);    dtostrf(flat,7, 6, LAT);
    dtostrf(flon,7, 6, LON);
    Serial.println(LAT);
    Serial.println(LON);
   
    }
  }
  if(millis()-samplingTime > samplingInterval)
  {
      pHArray[pHArrayIndex++]=analogRead(pinPh);
      if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
      voltage = avergearray(pHArray, ArrayLenth)*5.0/1024;
      pHValue = 3.5*voltage+0.5;
      samplingTime=millis();
  }
  if(millis() - printTime > printInterval)   //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
  { Serial.println(LAT);
    Serial.println(LON);
  Serial.print("Voltage:");
  Serial.print(voltage,2);
  Serial.print(" pH value: ");
  Serial.println(pHValue,2);
  ph=pHValue;
  int turbidityValue = analogRead(pinTurbidity);
  float voltage1 = turbidityValue * (5.0 / 1024.0); 
  Serial.print("Voltage:");
  Serial.print(voltage1);
  kekeruhan=40-(voltage1/4)*100;
  Serial.print(" Turbidity= ");
  Serial.println(kekeruhan);

  sensors.requestTemperatures(); 
  tem=sensors.getTempCByIndex(0);
  Serial.print("Suhu= ");
  Serial.print(tem);
  Serial.println("C");
  send2server();
  t.update();

  digitalWrite(LED,digitalRead(LED)^1);
  printTime=millis();
  }
}
double avergearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }else{
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}
void connect_wifi(String cmd, int t)
{
  int temp=0,i=0;
  while(1)
  {

    Serial.println(cmd);
    Serial1.println(cmd); 
    while(Serial1.available())
    {
      if(Serial1.find("OK"))

      i=8;
    }
    delay(t);
    if(i>5)
    break;
    i++;
  }
  if(i==8)
  {
   Serial.println("OK");
  }
  else
  {
   Serial.println("Error");
  }
}
void send2server()
{
  char tempStr[8];
  char phStr[8];
  char keruhStr[8];
    
  dtostrf(tem, 5, 3, tempStr);
  dtostrf(ph, 5, 3, phStr);
  dtostrf(kekeruhan, 5, 3, keruhStr);

//Suhu,Ph,Turbidity,Lux Solar,Volt,Latitude,Langitude
  sprintf(postUrl, "update?api_key=%s&field1=%s&field2=%s&field3=%s&field4=%s&field5=%s",api_key,tempStr,phStr,keruhStr,LAT,LON);
  httpGet("api.thingspeak.com", postUrl, 80);
}
void httpGet(String ip, String path, int port)
{
  int resp;
  String atHttpGetCmd = "GET /"+path+" HTTP/1.0\r\n\r\n";
  //AT+CIPSTART="TCP","192.168.20.200",80
  String atTcpPortConnectCmd = "AT+CIPSTART=\"TCP\",\""+ip+"\","+port+"";
  connect_wifi(atTcpPortConnectCmd,1000);
  int len = atHttpGetCmd.length();
  String atSendCmd = "AT+CIPSEND=";
  atSendCmd+=len;
  connect_wifi(atSendCmd,1000);
  connect_wifi(atHttpGetCmd,1000);
}
