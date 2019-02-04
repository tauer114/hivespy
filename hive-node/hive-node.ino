/*
    Hivespy - Network
    === Hive Node ===
    Libraries:
    nRF24/RF24, https://github.com/nRF24/RF24
    nRF24/RF24Network, https://github.com/nRF24/RF24Network
    nrf24/RF24Mesh, https://github.com/nRF24/RF24Mesh/releases
    Low-Power, https://github.com/rocketscream/Low-Power
*/

#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include "SPI.h"
#include "LowPower.h"

/**** Configure the nrf24l01 CE and CS pins ****/
RF24 radio(9, 10);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

/*
  * userconfig:
  * name of the node
  * ID of the node, has to be unique and from 1 - 255
  * location of the hive
  * time in seconds, the node should sleep between measuring
  */

const uint8_t nodeID = 1;
const String nodeName = "Meine Beute 1";
const String nodeLocation = "Standort 1";
const uint16_t nodeSleepTime = 3600;

/*
 *  struct for keeping and sending data to RF24Mesh network
 */
struct payload_t
{
    long weight;
    long battery;
    String nodeName;
    String nodeLocation;
    int sendErrorCount;
};

/*
 * defintion of some internal vars
 */
int sendErrorCount = 0;

void setup()
{
    Serial.begin(115200);

    // Set the nodeID manually
    mesh.setNodeID(nodeID);

    // Connect to the mesh
    Serial.println(F("Connecting to the mesh..."));

    mesh.begin();

    Serial.println("Setup complete");
}

void loop()
{
    componentWakeup();

    // Call mesh.update to keep the network updated
    mesh.update();

    // == Calculate/assemble data object: ==
    payload_t payload;
    payload.weight = getWeight();
    payload.battery = getBattery();
    payload.sendErrorCount = sendErrorCount;
    payload.nodeName = nodeName;
    payload.nodeLocation = nodeLocation;

    sendPayload(payload);
    componentSleep();
}

/* 
 * ===================
 * End of main loop
 * ===================
 */

/* 
 * ===================
 * Functions and helper
 * ===================
 */
void componentWakeup()
{
    Serial.println("Waking up all nessecary modules.");
    radio.powerUp();
}

void componentSleep()
{
    Serial.println(F("Going to sleep."));
    radio.powerDown();
    for (int j = 0; j < (nodeSleepTime / 8); j++)
    {
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    }
}

void sendPayload(payload_t payload)
{
    // Send an 'D' type message containing the data
    while (!mesh.write(&payload, 'D', sizeof(payload)))
    {
        Serial.println("Sending message failed. Retrying to reconnect to mesh.");
        payload.sendErrorCount = ++sendErrorCount;

        // If a write fails, check connectivity to the mesh network
        if (!mesh.checkConnection())
        {
            //refresh the network address
            Serial.println("Renewing address");
            mesh.renewAddress();
            #ifdef DEBUG
                Serial.print("DEBUG: mesh address after renewing: ");
                Serial.println(mesh.mesh_address);
            #endif
        }
        else
        {
            Serial.println("Send fail but connection is OK. Should never happen.");
        }
    }

    Serial.println("Send OK.");
}

float getWeight()
{
    return random(2000, 4000) / 100.0;
}

float getBattery() {
    return random(0,10000) / 100.0;
}