#include <SPI.h>
#include <MySensor.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <BH1750.h>
#include <Wire.h> 

/* MySensors multisensor
 
 Batterylevel
 Temperature
 Motion
 Light 

*/

//********** CONFIG **********************************

    #define NODE_ID     111  // ID of node
    #define TEMP_ID     1    // ID of TEMP
    #define PIR_ID      2    // ID of PIR
    #define LIGHT_ID    3    // ID of LDR
    #define LIGHTDIM_ID 4    // ID of Dimmer
    
    #define PIR_PIN       3     // Pin of PIR
    #define ONE_WIRE_BUS  4     // Pin where dallase sensor is connected 
    #define LED_PIN       5     // OUTPUT TO LED
    
    unsigned long SLEEP_TIME = 30000; // Sleep time between reads
    
    int BATTERY_SENSE_PIN = A0; 

//****************************************************

MySensor node;
MyMessage TEMPmsg(TEMP_ID, V_TEMP);
MyMessage PIRmsg(PIR_ID, V_TRIPPED);
MyMessage LIGHTmsg(LIGHT_ID, V_LEVEL);
MyMessage DIMMERmsg(LIGHTDIM_ID, V_DIMMER);
MyMessage LIGHT2msg(LIGHTDIM_ID, V_LIGHT);

int batteryPcnt;
int oldBatteryPcnt;
float lastTemp;
float lastTemperature[1];
int lastPIR = 2;
int lastPIR2 = 2;
int lastLightLevel;
int dimLevel = 100;
boolean lightON = true;   //  Switch the light on when the PIR is ON
boolean doSleep = true;   //  If the light is on sleep mode is deactivated
boolean debug = true;     //  Give feedback to serial what is going on

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 
BH1750 lightSensor;

void setup() {
  node.begin(incomingMessage, NODE_ID);
  
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // Register all sensors to gateway
  node.present(TEMP_ID, S_TEMP);
  node.present(PIR_ID, S_DOOR); 
  node.present(LIGHT_ID, S_LIGHT_LEVEL);
  node.present(LIGHTDIM_ID, S_DIMMER);
    
  sensors.begin();                      // Startup up the OneWire library
  sensors.setWaitForConversion(false);  // requestTemperatures() will not block current thread
  lightSensor.begin();                  // Startup the BH1750
}

void loop() {
node.process();
int PIR = readPIR();
lightControl(PIR);
if (doSleep){
  readDALLAS();
  readLIGHT();
  readBATTERY();
  node.sleep(PIR_PIN-2, CHANGE, SLEEP_TIME);
  }
}

// *** LIGHT CONTROL ****************************************************
void lightControl(int PIR){
if (lightON){
  Serial.println("light Control is TRUE");
  Serial.println(PIR);
  Serial.println(lastPIR2);
  if (PIR == 1) {
    Serial.println("PIR is HIGH");
    if (PIR != lastPIR2){
    node.request(LIGHTDIM_ID, V_DIMMER);
    node.wait(1000);
    Serial.println("The DIMLEVEL IS:");
    Serial.println(dimLevel);
    lastPIR2 = PIR;
    if (dimLevel != 0) {
      Serial.println("The dimlevel is not 0, so writing the dimlevel to the PIN");
      Serial.println((int)(dimLevel / 100. * 255) );
      analogWrite( LED_PIN, (int)(dimLevel / 100. * 255) );
      doSleep = false;
      }
    }
  }
  else {
    Serial.println("The light Control is on, but the PIR is low");
    analogWrite (LED_PIN,0);
    doSleep = true;
    lastPIR2 = PIR;
    }
  }
}

// *** PIR SENSOR ****************************************************

int readPIR() {
  int PIR = digitalRead(PIR_PIN);
  if (PIR != lastPIR) {
    lastPIR = PIR;
    node.send(PIRmsg.set(PIR == HIGH ? 1 : 0));
  }
  if (debug) {
    Serial.print("Motion: ");
    if (PIR == HIGH) {
      Serial.println("true");
      } 
     else {
      Serial.println("false");
      }
    }
  return PIR;
  }

// *** DALLAS SENSOR ************************************************

void readDALLAS(){
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0); // Fetch and round temperature to one decimal
    if (temperature != lastTemp && temperature != -127.00 && temperature != 85.00) {
       node.send(TEMPmsg.set(temperature,1)); // Send in the new temperature
       lastTemp=temperature; // Save new temperatures for next compare
    }
    if (debug){
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.println("C");
      }
  }

// *** LIGHT SENSOR  *************************************************

void readLIGHT() {
  uint16_t lightLevel = lightSensor.readLightLevel(); // Get Lux value
   if (lightLevel != lastLightLevel) {                // If value of LDR has changed
      node.send(LIGHTmsg.set(lightLevel));            // Send value of LDR to gateway
      lastLightLevel = lightLevel;
      }
   if (debug){
      Serial.print("Light: ");
      Serial.print(lightLevel);
      Serial.println("%");
      }
  }

// *** BATTERY SENSOR **********************************************

void readBATTERY(){
    int sensorValue = analogRead(BATTERY_SENSE_PIN);
    float batteryV  = sensorValue * 0.003363075; 
    int batteryPcnt = sensorValue / 10; 
    if (oldBatteryPcnt != batteryPcnt) {   // If battery percentage has changed
      node.sendBatteryLevel(batteryPcnt);  // Send battery percentage to gateway
      oldBatteryPcnt = batteryPcnt;
      }
    if (debug){
        Serial.print("Battery: ");
        Serial.print(batteryPcnt);
        Serial.println("%");
        }
  }

  void incomingMessage(const MyMessage &message) {
  //Serial.print (message.type);
  if (message.type == V_DIMMER) {
    Serial.print("DIMMER = ");Serial.println(message.getInt());
    dimLevel = message.getInt();
  }
}
