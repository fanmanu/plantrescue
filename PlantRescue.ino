/*   Bonsai Rescue - Smart Sensor
 * 
 *   This code works for:
 *   1) Reading two sensors: DHT11 and Soil Moisture
 *   2) Uses ESP32 for processing and wifi connection
 *   3) Publish sensor readings to ThingsIO
 *   
 *   Libraries required: 
 *   DHTesp - https://github.com/beegee-tokyo/DHTesp
 *   Adafruit - https://github.com/adafruit/Adafruit_Sensor
 *   WiFiMulti - https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiMulti.h
 *   
 *   Made by: Manu Nagarajan - GDTSE @ University of Auckland
 *   
 *   Version 1: 
 *    - Amalgamation of 
 *        1) DHT_ESP32.ino (DHT Sensor Library)
 *        2) ThingsIO.ino (
 *    - 
 *   
 */

/***********************************************
 * Include Libraries
************************************************/
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <DHTesp.h>

/***********************************************
 * Define Pins
************************************************/
#define DHTPIN 22                                       //Digital pin for DHT Sensor
#define MOISTUREPIN 32                                  //Analog pin for Moisture Sensor

/***********************************************
 * Define Constants
************************************************/
#define TOKEN "eaxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"    // Assign your IoT platform TOKEN
#define WIFINAME "xxxxxxxxxxxxxx"                       // Assign your SSID
#define WIFIPASS "xxxxxxxx"                             // Assign your Wifi Password
#define DEVICE "ESP32"                                  // ThingsIO Device Label
#define VAR_PUB_1 "temperature"                         // ThingsIO Variables' label for publishing data
#define VAR_PUB_2 "humidity"
#define VAR_PUB_3 "soilmoisture"
#define VAR_PUB_4 "soilpercentage"
#define DHTTYPE DHT11                                   // DHT 11 sensor

//Declarations for CLOUD Connectivity
const char* host = "api.thingsai.io";                   // OR host = devapi2.thethingscloud.com
const char* post_url = "/devices/deviceData";           // OR /api/v2/thingscloud2/_table/data_ac
const char* time_server = "baas.thethingscloud.com";    // This is to convert timestamp
const int httpPort = 80;
const int httpsPort = 443;
const char*  server = "api.thingsai.io";                // Server URL
char timestamp[10];

DHTesp dht;
TempAndHumidity newValues;
WiFiMulti wifiMulti;                                    
WiFiClient client;                                      // Use WiFiClient class to create TCP connections

int moistureValue;                                      // Store incoming value
int moistureLevel;                                      // Store moisture level

//Declarations for 3.3V Capacitive Soil Sensor
int moistureMin = 1812;                                 // Soil drowned in water
int moistureMax = 3400;                                 // Soil is dry

//int count = 0, i, m, j, k;                              // Count

/***********************************************
 * Define Instances
************************************************/
//


/***********************************************
 * Main Functions
************************************************/
void setup() 
{
  Serial.begin(115200);
  delay(10);

  wifiMulti.addAP(WIFINAME,WIFIPASS);                  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  dht.setup(DHTPIN, DHTesp::DHT11);                     // Initialise temperature sensor
  Serial.println("DHT initiated");
  pinMode(MOISTUREPIN, INPUT);                          // Initialise moisture sensor
  delay(500);
  
}

void loop() 
{
   /*float temperature = */getTemperature();
   /*float soilMoisture = measureSoilMoisture();*/
  sendTemp();

}

/***********************************************
 * Additional Functions
************************************************/
bool getTemperature(){
/*     
 * getTemperature - Reads temperature from DHT11 sensor   
 * @return bool      
 *    true if temperature could be aquired      
 *    false if aquisition failed
*/

   newValues = dht.getTempAndHumidity();

   if (dht.getStatus() != 0) {                          // Check for failed reads or early exits (to try again) 
     Serial.println("DHT11 error status: " + String(dht.getStatusString()));
     return false;
   }

   Serial.println("");
   Serial.println("TEMP:" + String(newValues.temperature) + " HUMID:" + String(newValues.humidity));
   return true; 
}


/***********************************************
 * Auxiliar Functions
************************************************/
int GiveMeTimestamp(){
/*     
 * Timestamp calculation function   
 * 
 * 
*/
  unsigned long timeout = millis();                        

  while (client.available() == 0)
  {
    if (millis() - timeout > 50000)
    {
      client.stop();
      return 0;
    }
  }

  while (client.available())
  {
    String line = client.readStringUntil('\r');         //indexOf() is a funtion to search for smthng, it returns -1 if not found
    int pos = line.indexOf("\"timestamp\"");            //search for "\"timestamp\"" from beginning of response got and copy all data after that , it'll be your timestamp
    if (pos >= 0)
    {
      int j = 0;
      for (j = 0; j < 10; j++)
      {
        timestamp[j] = line[pos + 12 + j];
      }
    }
  }
}


void sendTemp(){ 
/*     
 * Send the query and receive the response   
 * 
 * 
*/
  float temperature = dht.getTemperature();
  Serial.print("Celcius: ");
  Serial.println(temperature);

  float humidity = dht.getHumidity();
  Serial.print("Humidity: ");
  Serial.println(humidity);

  int soilMoisture = analogRead(MOISTUREPIN); 
  Serial.print("Soil Moisture: ");
  Serial.println(soilMoisture);
  
  int soilPercent = map(soilMoisture,moistureMin,moistureMax,100,0);  // soilMoisture reading will convert to percentage range
  Serial.print("Soil Percentage: ");
  Serial.println(soilPercent);


  Serial.print("Connecting to ");
  Serial.println(host);                                 //defined upside :- host = devapi2.thethingscloud.com or 139.59.26.117



  ///////////////////////////////////// TIMESTAMP CODE SNIPPET /////////////////////////
  Serial.println("Inside get timestamp\n");
  if (!client.connect(time_server, httpPort))
  {
    return;                                             //*-*-*-*-*-*-*-*-*-*
  }

  client.println("GET /api/timestamp HTTP/1.1");
  client.println("Host: baas.thethingscloud.com");
  client.println("Cache-Control: no-cache");
  client.println(TOKEN);
  client.println();

  GiveMeTimestamp();                                    //it'll call the function which will get the timestamp response from the server
  Serial.println("timestamp receieved");
  Serial.println(timestamp);

  Serial.println("inside ThingsCloudPost");

  String PostValue = "{\"device_id\": 56565656, \"slave_id\": 2";
  PostValue = PostValue + ",\"dts\":" + timestamp;
  PostValue = PostValue + ",\"data\":{\"celcius\":" + temperature + ",\"humidity\":" + humidity + ",\"soil\":" + soilMoisture + ",\"moisturepercent\":" + soilPercent + "}" + "}";

  Serial.println(PostValue);

  /* create an instance of WiFiClientSecure */
  WiFiClientSecure client;

  Serial.println("Connect to server via port 443");
  if (!client.connect(server, httpsPort)) {
    Serial.println("Connection failed!");
  } else {
    Serial.println("Connected to server!");
    /* create HTTP request */

    client.println("POST /devices/deviceData HTTP/1.1");
    client.println("Host: api.thingsai.io");
    //client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("cache-control: no-cache");
    client.println("Authorization: Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.IjVhMzBkZDFkN2QwYjNhNGQzODkwYzQ4OSI.kaY6OMj5cYlWNqC2PNTkXs9PKy6_m9tdW5AG7ajfVlY");
    client.print("Content-Length: ");
    client.println(PostValue.length());
    client.println();
    client.println(PostValue);

    //POSTING the data on to the cloud is done and now get the response form cloud server//////////////////
    Serial.print("Waiting for response ");
    while (!client.available()) {
      delay(50); //
      Serial.print(".");
    }
    /* if data is available then receive and print to Terminal */
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }

    /* if the server disconnected, stop the client */
    if (!client.connected()) {
      Serial.println();
      Serial.println("Server disconnected");
      client.stop();
    }
  }
  Serial.println("//////////////////////    THE END     /////////////////////");
  delay(3000);
}  
