///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sketch for the ESP8266-01 for the power_control project.                                                                      //
// This sketch calculate the value of current through two amperometric transformers. The function is inspired from EmonLib.      //
// Because the ESP8266-01 board not have analog pin this use a ADS1115, as external ADC, it communicae with board through I2C.   //
// This sketch calculate the power in watt and send thorugh WiFi an HTTP POST request a json string with the value to a server.  //
// Also update the value of ICAL, it is very useful for correct your measurement. Visit the Emonlib website for info.            //
// This project is for academic purpose, it may not be perfectly correct. Adapt the code with SSID and pass of your WiFi.        //
//                                                                                                                               //
// Author: Saraceni Rodolfo                                                                                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Include all the used library
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include "EmonLib.h"
#include <ESP8266HTTPClient.h>

// Define the costant for calculation of current
# define Number_of_Samples 1480    // Number of measure
# define adc_bits 10    // Number of bit of resolution
# define adc_counts 1024
# define SupplyVoltage 6141    // Voltage reference of ADC

Adafruit_ADS1115 ads;    // New instance of ADS library

// Update these with values suitable for your network.
const char* ssid = "INSERT SSID OF YOUR NETWORK";
const char* password = "INSERT PASSWORD OF YOUR NETWORK";
WiFiClient espClient;    // New instance of WiFi library

double offsetI = 0, filteredI = 0, sqI = 0, sumI = 0, full_count = 0, sampleI = 0, I_RATIO = 0, irms = 0,  ICAL = 0;  // Global variables for cal_sumI function
int analog_pin = 0, watt = 0, consumata = 0, prelevata = 0, ceduta = 0, auto_cons = 0;

// Array where save the string for json message
char consumata_json[75]; char prodotta_json[75]; char prelevata_json[75]; char ceduta_json[75];  char auto_cons_json[75];
char json[75];    // Array for the "Stampa" function

// Main function for calculate the value of current, because the resolution of ADS1115 isn't supported by the library the count must be 
double calc_watt(int analog_pin, double ICAL) {
    for (unsigned int n = 0; n < Number_of_Samples; n++)
    {
        full_count = ads.readADC_SingleEnded(analog_pin);    // Read value from the ADS115 pin
        double volts = full_count * (6.144 / 32767.0);
        sampleI = volts * (1023 / 5);
        offsetI = (offsetI + (sampleI - offsetI) / 1024);     // Eliminate the offset
        filteredI = sampleI - offsetI;
        sqI = filteredI * filteredI;
        sumI += sqI;
    }

    I_RATIO = ICAL * ((SupplyVoltage / 1000.0) / (adc_counts));
    irms = I_RATIO * sqrt(sumI / Number_of_Samples);

    // Calculate and cast to int the double value of power in Watt (don't need a so great precision)
    double watt_double = irms * 230;
    watt = (int) watt_double;

    return watt;    // Return the int value of power
}

// Function for print through serial port for debug
void stampa(int watt, char* json)
{
    Serial.print("["); Serial.print(analog_pin); Serial.print("]");
    Serial.print(" JSON--> ");  Serial.println(json);
    Serial.print("FULL-COUNTS: "); Serial.print(full_count);
    Serial.print(" -|- COUNTS: "); Serial.print(sampleI);
    Serial.print(" -|- IRMS: "); Serial.print(irms);
    Serial.print(" -|- WATT: "); Serial.println(watt);
    Serial.print("OFFSETI: ");  Serial.print(offsetI);
    Serial.print(" -|- ADC-COUNTS: ");  Serial.print(adc_counts);
    Serial.print(" -|- ADC-BITS: ");  Serial.print(adc_bits);
    Serial.print(" -|- ICAL: ");  Serial.print(ICAL);
    Serial.print(" -|- VOLT-SUPPLY: ");  Serial.println(SupplyVoltage);
    Serial.println("----------------------------------------------------------------------------------------");
}

void setup() {
    pinMode(BUILTIN_LED, OUTPUT);    // Initialize the BUILTIN_LED pin as an output
    Serial.begin(115200);    // Initialize serial communication for debug
    Wire.pins(0, 2);    // Initialize I2C communication for the ADC
    ads.begin();    // Initialize the ADC
    setup_wifi();    // Initiliaze the WiFi

    offsetI = ADC_COUNTS >> 1;    // Calculate the offset in Voltage for a correct measurement

    // Message for debbugging print only one time at the setup
    Serial.println("#################################### SETUP ################################");
    Serial.print("offsetI: ");  Serial.print(offsetI);
    Serial.print(" -|- adc-counts: ");  Serial.print(adc_counts);
    Serial.print(" -|- adc-bits: ");  Serial.print(adc_bits);
    Serial.print(" -|- volt-supply: ");  Serial.println(SupplyVoltage);
    Serial.println(); Serial.println();
}

void setup_wifi() {

    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to "); Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println(""); Serial.println("WiFi connected");
    Serial.println("IP address: "); Serial.println(WiFi.localIP());
}

void loop() {
        
    // Calculate the current consumed by the home with the relative function
    analog_pin = 0; ICAL = 7.20;    // Set the pin and the relative correction value calculate with the specific equation
    int consumata = calc_watt(analog_pin, ICAL);    // Call the function and pass the parameters
    if (consumata < 2) { consumata = 0; }    // Approximation to avoid false reading caused by very small value of current
    snprintf (consumata_json, 75, "[{\"measurement\": \"data\", \"fields\": {\"consumata\":%d}}]", consumata);    // Compose the json message to sent to server
    stampa(consumata, consumata_json);    // Call the function for debug
    sampleI = 0; sumI = 0;    // Reset variables of function
    
    // Calculate the current produced by the plant with the relative function
    analog_pin = !analog_pin; ICAL = 7.70;    // Set the pin and the relative correction value calculate with the specific equation
    int prodotta = calc_watt(analog_pin, ICAL);    // Call the function and pass the parameters
    if (prodotta < 2) { prodotta = 0; }    // Approximation to avoid false reading caused by very small value of current
    snprintf (prodotta_json, 75, "[{\"measurement\": \"data\", \"fields\": {\"prodotta\":%d}}]", prodotta);    // Compose the json message to sent to server
    stampa(prodotta, prodotta_json);    // Call the function for debug
    sampleI = 0; sumI = 0;    // Reset variables of function

    // Control to calculate the value of auto-consumption and selling or buying energy from and to the grid
    // If the home consume more energy than the plant produce, it must buy from grid. Instead if produce more than the home consumed it can sell the surplus
    if (consumata >= prodotta) { 
      prelevata = consumata - prodotta;    // The home buy energy from grid
      ceduta = 0;    // The system is not selling energy at this moment
      auto_cons = prodotta;    // All the energy produced is used, or the plant is off so it's 0
    } 
    else {
      ceduta = prodotta - consumata;    // The home send energy to the grid, so it's selling energy
      prelevata = 0;    // The system is not buying energy at this moment
      auto_cons = consumata;    // A part of the energy produced is used, and it's equal to the consumption
    }

    // Prepare the string to send through http post request
    
    snprintf (prelevata_json, 75, "[{\"measurement\": \"data\", \"fields\": {\"prelevata\":%d}}]", prelevata);
    delay(100);
    snprintf (ceduta_json, 75, "[{\"measurement\": \"data\", \"fields\": {\"ceduta\":%d}}]", ceduta);
    delay(100);
    snprintf (auto_cons_json, 75, "[{\"measurement\": \"data\", \"fields\": {\"auto_cons\":%d}}]", auto_cons);
    delay(100);

    // Print the value to debug
    Serial.println(prodotta_json); Serial.println(ceduta_json); Serial.println(auto_cons_json);

    // Section where the ESP send the json string to server through http post request
    if (WiFi.status() == WL_CONNECTED) {    // Check WiFi connection status
        HTTPClient http;    // Declare object of class HTTPClient
        http.begin("http://SERVER URL:PORT OF SERVER/ APPLICATION ADDRESS");    // Specify request destination
        http.addHeader("Content-Type", "application/json");    // Specify content-type header

        int httpCode = http.POST(consumata_json);    // Send the request
        Serial.print("Consumata send: "); Serial.println(httpCode);    // Print HTTP return code

        httpCode = http.POST(prodotta_json);    // Send the request
        Serial.print("Prodotta send: "); Serial.println(httpCode);    // Print HTTP return code

        httpCode = http.POST(prelevata_json);
        Serial.print("Prelevata send: "); Serial.println(httpCode);

        httpCode = http.POST(ceduta_json);
        Serial.print("Ceduta send: "); Serial.println(httpCode);

        httpCode = http.POST(auto_cons_json);
        Serial.print("Auto-cosumata send: "); Serial.println(httpCode);


        http.end();    // Close connection

    } else {
        Serial.println("Error in WiFi connection");
    }

            
    // Put the ESP8266-01 in deep sleep, when the board wake up it hard reset so it start from the setup function.
    // For activated in ESP8266-01 the wake up from timed sleep, solder the "post-sleep-reset-pin" of smd chip on reset pin of the board
    Serial.println("###################### DEEP SLEEP ######################");
    Serial.println();
    ESP.deepSleep(1000000*60*5);
}



