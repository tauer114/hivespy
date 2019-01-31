/*
    Hivespy - Network
    === Hive Node ===
    Libraries:
    nRF24/RF24, https://github.com/nRF24/RF24
    nRF24/RF24Network, https://github.com/nRF24/RF24Network
    nrf24/RF24Mesh, https://github.com/nRF24/RF24Mesh/releases
*/

#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>

/**** Configure the nrf24l01 CE and CS pins ****/
RF24 radio(9, 10);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

/**
   User Configuration: nodeID - A unique identifier for each radio. Allows addressing
   to change dynamically with physical changes to the mesh.
   In this example, configuration takes place below, prior to uploading the sketch to the device
   A unique value from 1-255 must be configured for each node.
   This will be stored in EEPROM on AVR devices, so remains persistent between further uploads, loss of power, etc.
 **/
const uint8_t nodeID = 1;

/**
 * enabled verbose output 
**/
#define DEBUG

/**
    wakeup interval of node:
    milliseconds
**/
const int globalSleepTime = 10000; 

// struct for keeping and sending data to RF24Mesh network
struct payload_t
{
    unsigned long weight;
};

void setup()
{
    Serial.begin(115200);

    // Set the nodeID manually
    mesh.setNodeID(nodeID);
    #ifdef DEBUG
        Serial.print("DEBUG: Node id set to: ");
        Serial.println(nodeID);
    #endif

    // Connect to the mesh
    Serial.println(F("Connecting to the mesh..."));

    mesh.begin();

    Serial.println("Setup complete");
}

void loop()
{
    Serial.println("Waking up all nessecary modules.");
    componentWakeup();

    // Call mesh.update to keep the network updated
    mesh.update();

    // == Calculate data: ==
    payload_t payload;
    // === Weight: ===
    payload.weight = random(2000,4000)/100.0;

    // Send an 'D' type message containing the data
    if (!mesh.write(&payload, 'D', sizeof(payload)))
    {
        // If a write fails, check connectivity to the mesh network
        if (!mesh.checkConnection())
        {
            //refresh the network address
            Serial.println("Renewing Address");
            mesh.renewAddress();
            #ifdef DEBUG
                Serial.print("DEBUG: mesh address: ");
                Serial.println(mesh.mesh_address);
            #endif
        }
        else
        {
            Serial.println("Send fail, Test OK. Should never happen.");
        }
    }
    else
    {
        Serial.println("Send OK: ");
        Serial.print("Weight: ");
        Serial.println(payload.weight);
    }

    // sleep most of the time
    Serial.println(F("Going to sleep."));
    componentSleep();
    delay(globalSleepTime);
}

bool componentWakeup()
{
    radio.powerUp();
    return true;
}

bool componentSleep()
{
    radio.powerDown();
    return true;
}
