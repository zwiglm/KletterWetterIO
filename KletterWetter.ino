// This #include statement was automatically added by the Particle IDE.
#include <SparkFun_Photon_Weather_Shield_Library.h>
#include <math.h>

PRODUCT_ID(5036);
PRODUCT_VERSION(1);

/*
  *****************************************************************************************
  **** Put some general info here...
  **** 
  **** 
  **** 
  **** 
  *****************************************************************************************/

// Each time we loop through the main loop, we check to see if it's time to capture the sensor readings
unsigned int sensorCapturePeriod = 300; //100;
unsigned int timeNextSensorReading;

// Each time we loop through the main loop, we check to see if it's time to publish the data we've collected
unsigned int publishPeriod = 900000; //60000;
unsigned int timeNextPublish; 

String field1 = "";
String field2 = "";  // i.e. field2 is null
String field3 = "";
String field4 = "";    
String field5 = "";
String field6 = "";
String field7 = "";
String field8 = "";
String lat = "";
String lon = "";
String el = "";
String status = "";

void setup() {
    initializeTempHumidityAndPressure();
    initializeRainGauge();
    initializeAnemometer();
    initializeWindVane();
    
    // Schedule the next sensor reading and publish events
    timeNextSensorReading = millis() + sensorCapturePeriod;
    timeNextPublish = millis() + publishPeriod; 
}

void loop() {

    // Capture any sensors that need to be polled (temp, humidity, pressure, wind vane)
    // The rain and wind speed sensors use interrupts, and so data is collected "in the background"
    if(timeNextSensorReading <= millis()) {
        captureTempHumidityPressure();
        captureWindVane();

        // Schedule the next sensor reading
        timeNextSensorReading = millis() + sensorCapturePeriod;
    }
    
    // Publish the data collected to Particle and to ThingSpeak
    if(timeNextPublish <= millis()) {
        
        // Get the data to be published
        float tempC = getAndResetTempC();
        float humidityRH = getAndResetHumidityRH();
        float pressureKPa = getAndResetPressurePascals() / 1000.0;
        float rainPerMM = getAndResetRainMM();
        float gustKPH;
        float windKPH = getAndResetAnemometerKPH(&gustKPH);
        float windDegrees = getAndResetWindVaneDegrees();
        
        FuelGauge fuel;
        float voltage = fuel.getVCell();
                          
        // Publish the data                    
        publishRettew(tempC, humidityRH, pressureKPa, rainPerMM, windKPH, gustKPH, windDegrees, voltage);

        // Schedule the next publish event
        timeNextPublish = millis() + publishPeriod;
    }
    
    delay(10);
}


void publishRettew(float tempC, float humidityRH, float pressureKPa, float rainPerMM, float windKPH, float gustKPH, float windDegrees, float voltage) {
    // To write multiple fields, you set the various fields you want to send
    
    field1 = String(tempC, 1);
    field2 = String(humidityRH, 0);
    field3 = String(pressureKPa, 1);
    field4 = String(rainPerMM, 1);
    field5 = String(windKPH, 1);
    field6 = String(gustKPH, 1);
    field7 = String(windDegrees, 0);
    field8 = String(voltage, 1);

    String KWjson;
    createKWjson(KWjson);
    Particle.publish("KlettWettWriteAllWD", KWjson, PRIVATE);

}

//===========================================================
// Temp, Humidity and Pressure
//===========================================================
// The temperature, humidity, and pressure sensors are on board
// the weather station board, and use I2C to communicate.  The sensors are read
// frequently by the main loop, and the results are averaged over the publish cycle

//Create Instance of HTU21D or SI7021 temp and humidity sensor and MPL3115A2 barometric sensor
Weather sensor;

void initializeTempHumidityAndPressure() {
    //Initialize the I2C sensors and ping them
    sensor.begin();
    //Set to Barometer Mode
    sensor.setModeBarometer();
    // Set Oversample rate
    sensor.setOversampleRate(7); 
    //Necessary register calls to enble temp, baro and alt
    sensor.enableEventFlags(); 
    
    return;
}

float humidityRHTotal = 0.0;
unsigned int humidityRHReadingCount = 0;
float tempCTotal = 0.0;
unsigned int tempCReadingCount = 0;
float pressurePascalsTotal = 0.0;
unsigned int pressurePascalsReadingCount = 0;

void captureTempHumidityPressure() {
  // Read the humidity and pressure sensors, and update the running average
  // The running (mean) average is maintained by keeping a running sum of the observations,
  // and a count of the number of observations
  
  // Measure Relative Humidity from the HTU21D or Si7021
  float humidityRH = sensor.getRH();
  
  //If the result is reasonable, add it to the running mean
  if(humidityRH > 0 && humidityRH < 105) // It's theoretically possible to get supersaturation humidity levels over 100%
  {
      // Add the observation to the running sum, and increment the number of observations
      humidityRHTotal += humidityRH;
      humidityRHReadingCount++;
  }

  // Measure Temperature from the HTU21D or Si7021
  // Temperature is measured every time RH is requested.
  // It is faster, therefore, to read it from previous RH
  // measurement with getTemp() instead with readTemp()
  float tempC = sensor.getTemp();
  
  //If the result is reasonable, add it to the running mean
  if (tempC > -50 && tempC < 70)
  {
	  tempCTotal += tempC;
	  tempCReadingCount++;
  }

  //Measure Pressure from the MPL3115A2
  float pressurePascals = sensor.readPressure();
  
  //If the result is reasonable, add it to the running mean
  // What's reasonable? http://findanswers.noaa.gov/noaa.answers/consumer/kbdetail.asp?kbid=544
  if(pressurePascals > 80000 && pressurePascals < 110000)
  {
      // Add the observation to the running sum, and increment the number of observations
      pressurePascalsTotal += pressurePascals;
      pressurePascalsReadingCount++;
  }
  
  return;
}

float getAndResetTempC() 
{
	if (tempCReadingCount == 0) {
		return 0;
	}
	float result = tempCTotal/float(tempCReadingCount);
	tempCTotal = 0.0;
	tempCReadingCount = 0;
	return result;
}

float getAndResetHumidityRH()
{
    if(humidityRHReadingCount == 0) {
        return 0;
    }
    float result = humidityRHTotal/float(humidityRHReadingCount);
    humidityRHTotal = 0.0;
    humidityRHReadingCount = 0;
    return result;
}


float getAndResetPressurePascals()
{
    if(pressurePascalsReadingCount == 0) {
        return 0;
    }
    float result = pressurePascalsTotal/float(pressurePascalsReadingCount);
    pressurePascalsTotal = 0.0;
    pressurePascalsReadingCount = 0;
    return result;
}

//===========================================================================
// Rain Guage
//===========================================================================
int RainPin = D2;
volatile unsigned int rainEventCount;
unsigned int lastRainEvent;
float RainScaleMM = 0.2794; // Each pulse is 0.011 inches   OR 0.2794 mm    of rain 

void initializeRainGauge() {
  pinMode(RainPin, INPUT_PULLUP);
  rainEventCount = 0;
  lastRainEvent = 0;
  attachInterrupt(RainPin, handleRainEvent, FALLING);
  return;
  }
  
void handleRainEvent() {
    // Count rain gauge bucket tips as they occur
    // Activated by the magnet and reed switch in the rain gauge, attached to input D2
    unsigned int timeRainEvent = millis(); // grab current time
    
    // ignore switch-bounce glitches less than 10mS after initial edge
    if(timeRainEvent - lastRainEvent < 10) {
      return;
    }
    
    rainEventCount++; //Increase this minute's amount of rain
    lastRainEvent = timeRainEvent; // set up for next event
}

float getAndResetRainMM()
{
    float result = RainScaleMM * float(rainEventCount);
    rainEventCount = 0;
    return result;
}

//===========================================================================
// Wind Speed (Anemometer)
//===========================================================================

// The cup-type anemometer measures wind speed by closing a contactas a magnet moves past a switch. 
// A wind speed of 1.492 MPH (2.4km/h) causes the switch to close once per second.
// We measure the average period (elaspsed time between pulses), and calculate the average windspeed since the last recording.

int AnemometerPin = D3;
float AnemometerScaleKPH = 2.4; // Windspeed if we got a pulse every second (i.e. 1Hz)
volatile unsigned int AnemoneterPeriodTotal = 0;
volatile unsigned int AnemoneterPeriodReadingCount = 0;
volatile unsigned int GustPeriod = UINT_MAX;
unsigned int lastAnemoneterEvent = 0;

void initializeAnemometer() {
  pinMode(AnemometerPin, INPUT_PULLUP);
  AnemoneterPeriodTotal = 0;
  AnemoneterPeriodReadingCount = 0;
  GustPeriod = UINT_MAX;  //  The shortest period (and therefore fastest gust) observed
  lastAnemoneterEvent = 0;
  attachInterrupt(AnemometerPin, handleAnemometerEvent, FALLING);
  return;
  }
  
void handleAnemometerEvent() {
    // Activated by the magnet in the anemometer (2 ticks per rotation), attached to input D3
     unsigned int timeAnemometerEvent = millis(); // grab current time
     
    //If there's never been an event before (first time through), then just capture it
    if(lastAnemoneterEvent != 0) {
        // Calculate time since last event
        unsigned int period = timeAnemometerEvent - lastAnemoneterEvent;
        // ignore switch-bounce glitches less than 10mS after initial edge (which implies a max windspeed of 149mph)
        if(period < 10) {
          return;
        }
        if(period < GustPeriod) {
            // If the period is the shortest (and therefore fastest windspeed) seen, capture it
            GustPeriod = period;
        }
        AnemoneterPeriodTotal += period;
        AnemoneterPeriodReadingCount++;
    }
    
    lastAnemoneterEvent = timeAnemometerEvent; // set up for next event
}

float getAndResetAnemometerKPH(float * gustKPH)
{
    if(AnemoneterPeriodReadingCount == 0)
    {
        *gustKPH = 0.0;
        return 0;
    }
    // Nonintuitive math:  We've collected the sum of the observed periods between pulses, and the number of observations.
    // Now, we calculate the average period (sum / number of readings), take the inverse and muliple by 1000 to give frequency, 
	// and then mulitply by our scale to get KPH.
    // The math below is transformed to maximize accuracy by doing all muliplications BEFORE dividing.
    float result = AnemometerScaleKPH * 1000.0 * float(AnemoneterPeriodReadingCount) / float(AnemoneterPeriodTotal);
    AnemoneterPeriodTotal = 0;
    AnemoneterPeriodReadingCount = 0;
    *gustKPH = AnemometerScaleKPH  * 1000.0 / float(GustPeriod);
    GustPeriod = UINT_MAX;
    return result;
}


//===========================================================
// Wind Vane
//===========================================================
void initializeWindVane() {
    return;
}

// For the wind vane, we need to average the unit vector components (the sine and cosine of the angle)
int WindVanePin = A0;
float windVaneCosTotal = 0.0;
float windVaneSinTotal = 0.0;
unsigned int windVaneReadingCount = 0;

void captureWindVane() {
    // Read the wind vane, and update the running average of the two components of the vector
    unsigned int windVaneRaw = analogRead(WindVanePin);
    
    float windVaneRadians = lookupRadiansFromRaw(windVaneRaw);
    if(windVaneRadians > 0 && windVaneRadians < 6.14159)
    {
        windVaneCosTotal += cos(windVaneRadians);
        windVaneSinTotal += sin(windVaneRadians);
        windVaneReadingCount++;
    }
    return;
}

float getAndResetWindVaneDegrees()
{
    if(windVaneReadingCount == 0) {
        return 0;
    }
    float avgCos = windVaneCosTotal/float(windVaneReadingCount);
    float avgSin = windVaneSinTotal/float(windVaneReadingCount);
    float result = atan(avgSin/avgCos) * 180.0 / 3.14159;
    windVaneCosTotal = 0.0;
    windVaneSinTotal = 0.0;
    windVaneReadingCount = 0;
    // atan can only tell where the angle is within 180 degrees.  Need to look at cos to tell which half of circle we're in
    if(avgCos < 0) result += 180.0;
    // atan will return negative angles in the NW quadrant -- push those into positive space.
    if(result < 0) result += 360.0;
    
   return result;
}

float lookupRadiansFromRaw(unsigned int analogRaw)
{
    // The mechanism for reading the weathervane isn't arbitrary, but effectively, we just need to look up which of the 16 positions we're in.
    if(analogRaw >= 2200 && analogRaw < 2400) return (3.14);//South
    if(analogRaw >= 2100 && analogRaw < 2200) return (3.53);//SSW
    if(analogRaw >= 3200 && analogRaw < 3299) return (3.93);//SW
    if(analogRaw >= 3100 && analogRaw < 3200) return (4.32);//WSW
    if(analogRaw >= 3890 && analogRaw < 3999) return (4.71);//West
    if(analogRaw >= 3700 && analogRaw < 3780) return (5.11);//WNW
    if(analogRaw >= 3780 && analogRaw < 3890) return (5.50);//NW
    if(analogRaw >= 3400 && analogRaw < 3500) return (5.89);//NNW
    if(analogRaw >= 3570 && analogRaw < 3700) return (0.00);//North
    if(analogRaw >= 2600 && analogRaw < 2700) return (0.39);//NNE
    if(analogRaw >= 2750 && analogRaw < 2850) return (0.79);//NE
    if(analogRaw >= 1510 && analogRaw < 1580) return (1.18);//ENE
    if(analogRaw >= 1580 && analogRaw < 1650) return (1.57);//East
    if(analogRaw >= 1470 && analogRaw < 1510) return (1.96);//ESE
    if(analogRaw >= 1900 && analogRaw < 2000) return (2.36);//SE
    if(analogRaw >= 1700 && analogRaw < 1750) return (2.74);//SSE
	
    if(analogRaw > 4000) return(-1); // Open circuit?  Probably means the sensor is not connected
	// MaZ todo: better error handling
    // Particle.publish("KlettWettWriteErr", String::format("Windvane: %d", analogRaw), PRIVATE);
	
    return -1;
}


// Function to build the 'json' to trigger the Webhook.  To save characters the string only includes parameters that are not null.
void createKWjson(String &dest)
{
  // dest = "{ \"1\":\""+ field1 +"\", \"2\":\""+ field2 +"\",\"3\":\""+ field3 +"\",\"4\":\""+ field4 +"\",\"5\":\""+ field5 +"\",\"6\":\""+ field6 +"\",\"7\":\""+ field7 +"\",\"8\":\""+ field8 +"\",\"a\":\""+ lat +"\",\"o\":\""+ lon +"\",\"e\":\""+ el +"\", \"s\":\""+ status +"\"}";
  
    dest = "{";
    
    if(field1.length() >0 ){
        dest = dest + "\"1\":\""+ field1 +"\",";
    }

    if(field2.length() > 0){
        dest = dest + "\"2\":\""+ field2 +"\",";
    }

    if(field3.length() > 0){
        dest = dest + "\"3\":\""+ field3 +"\",";
    }

    if(field4.length() > 0){
        dest = dest + "\"4\":\""+ field4 +"\",";
    }

    if(field5.length() > 0){
        dest = dest + "\"5\":\""+ field5 +"\",";
    }

    if(field6.length() > 0){
        dest = dest + "\"6\":\""+ field6 +"\",";
    }

    if(field7.length() > 0){
        dest = dest + "\"7\":\""+ field7 +"\",";
    }

    if(field8.length() > 0){
        dest = dest + "\"8\":\""+ field8 +"\",";
    }

    if(lat.length() > 0){
        dest = dest + "\"a\":\""+ lat +"\",";
    }

    if(lon.length() > 0){
        dest = dest + "\"o\":\""+ lon +"\",";
    }

    if(el.length() > 0){
        dest = dest + "\"e\":\""+ el +"\",";
    }

    if(status.length() > 0){
        dest = dest + "\"s\":\""+ status +"\",";
    }
    
    dest = dest + "}";
}