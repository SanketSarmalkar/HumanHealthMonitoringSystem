#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "MAX30105.h"
#include "MAX30100_PulseOximeter.h"

#include "heartRate.h"

#define DS18B20 2

PulseOximeter pox;

OneWire oneWire(DS18B20);
DallasTemperature sensors(&oneWire);


MAX30105 particleSensor;

const byte RATE_SIZE = 4;  //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];     //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0;  //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

float bodytemp = 0;

/*Put WiFi SSID & Password*/
const char* ssid = "Galaxy M518B8E";    // Enter SSID here
const char* password = "jzni3896";  // Enter Password here

ESP8266WebServer server(80);

bool LEDstatus = LOW;

void setup() {
  Serial.begin(115200);
  delay(100);
  pinMode(16, OUTPUT);



  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))  //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1)
      ;
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup();                     //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A);  //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0);   //Turn off Green LED



  Serial.println("Connecting to ");
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.begin(ssid, password);

  //check NodeMCU is connected to Wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");

    Serial.begin(9600);
    Serial.println("Dallas Temperature IC Control Library Demo");
    // Start up the library
    sensors.begin();
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.on("/ledon", handle_ledon);
  server.on("/ledoff", handle_ledoff);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP Server Started");
}
void loop() {
  server.handleClient();
  /* 
  if(LEDstatus)
  {
    digitalWrite(16, HIGH);}
  else
  {
    digitalWrite(16, LOW);}
*/
  /*

  Serial.print(" Requesting temperatures..."); 
  sensors.requestTemperatures(); // Send the command to get temperature readings 
  Serial.println("DONE"); 
/********************************************************************/
  /*  Serial.print("Temperature is: "); 
  bodytemp = sensors.getTempCByIndex(0);
  Serial.print(sensors.getTempCByIndex(0));

*/
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true) {

    Serial.print("Requesting temperatures...");
    sensors.requestTemperatures();  // Send the command to get temperature readings
    Serial.println("DONE");
    /********************************************************************/
    Serial.print("Body Temperature: ");
    bodytemp = sensors.getTempCByIndex(0);
    Serial.print(sensors.getTempCByIndex(0));
    //We sensed a beat!
    long delta = millis() - lastBeat;
    //lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    // beatsPerMinute = pox.getHeartRate();
    /*Serial.print("delta");
    Serial.print(delta);
    Serial.print("bpm rahul");
    Serial.print(beatsPerMinute);*/
    
    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute;  //Store this reading in the array
      rateSpot %= RATE_SIZE;                     //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }


    Serial.print(" IR=");
    Serial.print(irValue);
    Serial.print(", BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM=");
    Serial.println(beatAvg);

    delay(1000);
  }
/*
  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);
*/
  if (irValue < 50000){
    Serial.println("No finger?");
    delay(2000);
    beatsPerMinute = 0;
  }
  lastBeat = millis();
}

void handle_OnConnect() {
  //LEDstatus = LOW;
  //Serial.println("LED: OFF");
  server.send(200, "text/html", updateWebpage(LEDstatus));
}

void handle_ledon() {
  //LEDstatus = HIGH;
  //Serial.println("LED: ON");
  server.send(200, "text/html", updateWebpage(LEDstatus));
}

void handle_ledoff() {
  LEDstatus = LOW;
  Serial.println("LED: OFF");
  server.send(200, "text/html", updateWebpage(LEDstatus));
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

String updateWebpage(uint8_t LEDstatus) {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"> <script src=\"https://cdn.tailwindcss.com\"><\/script>\n";
  ptr += "<title>LED Control</title>\n";
  ptr += "<style>html {font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #3498db;}\n";
  ptr += ".button-on:active {background-color: #3498db;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1 class=\"text-amber-700 text-2xl\">ESP8266 Web Server</h1>\n";
  ptr += "<h3 class=\"mb-0\">Using Station(STA) Mode</h3>\n";
  /*ptr +=""
  
   if(LEDstatus){
    ptr +="<p>BLUE LED: ON</p><a class=\"button button-off\" href=\"/ledoff\">OFF</a>\n";
   }else{
    ptr +="<p>BLUE LED: OFF</p><a class=\"button button-on\" href=\"/ledon\">ON</a>\n";
   }
*/

  //Ajax Code Start
  ptr += "<script>\n";
  ptr += "setInterval(loadDoc,1000);\n";
  ptr += "function loadDoc() {\n";
  ptr += "var xhttp = new XMLHttpRequest();\n";
  ptr += "xhttp.onreadystatechange = function() {\n";
  ptr += "if (this.readyState == 4 && this.status == 200) {\n";
  ptr += "document.body.innerHTML =this.responseText}\n";
  ptr += "};\n";
  ptr += "xhttp.open(\"GET\", \"/\", true);\n";
  ptr += "xhttp.send();\n";
  ptr += "}\n";
  ptr += "</script>\n";
  //Ajax Code END

  ptr += "</body>\n";
  ptr += "</html>\n";
  //For Body Temperature
  ptr += "<p >";
  //ptr += "<i class='fas fa-thermometer-full' style='color:#d9534f'></i>";
  ptr += "<span class='sensor-labels pt-4'> Body Temperature </span><br/>";
  ptr += "<div class=\"rounded-2 bg-lime-100 p-2\">";
  ptr += "<span class=\"text-2xl text-green-500 \">";
  ptr += (float)bodytemp;
  ptr += "<\span>";
  ptr += "<sup class='units'>°C</sup>";
  ptr += "<\div>";
  ptr += "</p>";
  /*
  ptr +="</body>\n";
  ptr +="</html>\n";*/
  //For heartbeat
  ptr += "<p class='sensor'>";
  ptr += "<i class='fas fa-thermometer-full' style='color:#d9534f'></i>";
  ptr += "<span class='sensor-labels pt-4'> Beats per Mins </span><br/>";
  ptr += "<div class=\"rounded-2 bg-lime-100 p-2\">";
  ptr += "<span class=\"text-2xl text-green-500 \">";
  ptr += (float)beatsPerMinute;
  ptr += "<\span>";
  ptr += "<sup class='units'>°C</sup>";
  ptr += "<\div>";
  ptr += "</p>";

  ptr += "</div>";
  ptr += "</div>";
  ptr += "</div>";
  ptr += "</div>";
  ptr += "</div>";
  ptr += "</body>";
  ptr += "</html>";
  return ptr;
}
