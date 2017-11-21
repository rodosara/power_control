//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sketch for the Arduino MKRFox1200 for the power_control project.                                                                 //
// This sketch gater data from the inverter of the photovoltaic system.                                                             //
// The inverter use a Modbus RTU communication over serial bus with a 9600/N/1 configuration. It use ModbusMaster library.          //
// The data is sent to the server through the SigFox network. It allow you to send 12 byte of payload on ISM band. (1% duty Cycle). //
// This project is for academic purpose, it may not be perfectly correct.                                                           //
//                                                                                                                                  //
// Author: Saraceni Rodolfo                                                                                                         //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Include all the library used in the sketch
#include "ArduinoLowPower.h"
#include <ModbusMaster.h>
#include "util/crc16.h"
#include <SigFox.h>

// Define the pin for the RS485 converter
#define MAX485_DE      2
#define MAX485_RE_NEG  2

// Define the costant for millis use
unsigned long previousMillis = 0;
const long interval = 900000;    // break of 15 minutes

// Instantiate ModbusMaster object
ModbusMaster inverter;

// Declare the variables that contain the register read from the inverter
uint16_t tot_energy_h, tot_energy_l, vdc, idc, pac, vac;

// Function to set the RS485 converter in trasmission mode (the converter is only half-duplex)
void preTransmission()
{
    digitalWrite(MAX485_RE_NEG, 1);
    digitalWrite(MAX485_DE, 1);
}

// Function to set the RS485 converter in receive mode (the converter is only half-duplex)
void postTransmission()
{
    digitalWrite(MAX485_RE_NEG, 0);
    digitalWrite(MAX485_DE, 0);
}

// Function thath reboot the board if there is any problem
void reboot() {
    NVIC_SystemReset();
    while (1);
}

void setup()
{
    // Put tthe RS485 converter pins in output mode
    pinMode(MAX485_RE_NEG, OUTPUT);
    pinMode(MAX485_DE, OUTPUT);

    // Init the RS485 converter in receive mode
    digitalWrite(MAX485_RE_NEG, 0);
    digitalWrite(MAX485_DE, 0);

    // Modbus communication runs at 9600 baud
    Serial1.begin(9600);

    // Modbus slave ID 1
    inverter.begin(1, Serial1);

    // Callbacks allow us to configure the RS485 transceiver correctly
    inverter.preTransmission(preTransmission);
    inverter.postTransmission(postTransmission);

    // Initilize a serial communication through USB for debug
    Serial.begin(115200);

    // Check there is some trouble with SigFox module
    if (!SigFox.begin()) {
        //something is really wrong, try rebooting
        reboot();
    }
}

void loop()
{
    // Read any register in order to verify the connection with the inverter
    int ver = inverter.readInputRegisters(3, 1);
    if (ver == inverter.ku8MBSuccess) {
    
        // Use millis() to control the break between one query and the other
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;

            get_data();    // Call the function for gater data from inverter
          
            // Print all the value gatered thorugh serial debug
            Serial.println(tot_energy_h);
            Serial.println(tot_energy_l);
            Serial.println(vdc);
            Serial.println(idc);
            Serial.println(pac);
            Serial.println(vac);
           
            // Initiliaze the Sigfox module
            SigFox.begin();
            delay(100);     // Wait for the module to be prepared
          
            SigFox.beginPacket();    // Start the package
          
            // Send byte by byte the value read from the inverter, use the write function for better data rate control
            SigFox.write(tot_energy_l);
            SigFox.write(tot_energy_h);
            SigFox.write(vdc);
            SigFox.write(idc);
            SigFox.write(pac);
            SigFox.write(vac);
          
            int ret = SigFox.endPacket();    // Save in ret variable the return from the sending process

            // Control if the mailing has happened correctly
            if (ret == 0)
                Serial.println("OK if ret=0");
            else {
                Serial.print("KO if ret=1");   
                SigFox.end();    // Close the communication and stop the SigFox module
          
                // Print it through serial bus for debugging
                Serial.print(" -|- end.Packet: ");
                Serial.print(ret);
            }

        }
    }
}

// Function to query the inverter and read 6 input register in Modbus protocol.
// These are specific address for my inverter, you must adapat at yours.
// Use specific byte variables for better control the traffic data on SigFox network
uint16_t get_data () {
    uint8_t result = 0;

    // Read register 3001-3002 (TOTAL_ENERGY)
    result = inverter.readInputRegisters(0, 2);    // Query the inverter, and save the values red
    if (result == inverter.ku8MBSuccess) {     // Verify the correct communication with the slave
        tot_energy_h = inverter.getResponseBuffer(0x00);    // Save the values in the corresponding variables
        tot_energy_l = inverter.getResponseBuffer(0x01);
    }

    delay(100);

    // Read register 3013 (VDC)
    result = inverter.readInputRegisters(12, 1);
    if (result == inverter.ku8MBSuccess) {
        vdc = inverter.getResponseBuffer(0x00);
    }

    delay(100);

    // Read register 3014 (IDC)
    result = inverter.readInputRegisters(13, 1);
    if (result == inverter.ku8MBSuccess) {
        idc = inverter.getResponseBuffer(0x00);
    }

    delay(100);

    // Read register 3017 (PAC)
    result = inverter.readInputRegisters(16, 1);
    if (result == inverter.ku8MBSuccess) {
        pac = inverter.getResponseBuffer(0x00);
    }

    delay(100);

    // Read register 3020 (VAC)
    result = inverter.readInputRegisters(19, 1);
    if (result == inverter.ku8MBSuccess) {
        vac = inverter.getResponseBuffer(0x00);
    }
    return (tot_energy_h, tot_energy_l, vdc, idc, pac, vac);
}
