/*
    Hivespy - Network
    === Gateway Node ===
    Libraries:
    nRF24/RF24, https://github.com/nRF24/RF24
    nRF24/RF24Network, https://github.com/nRF24/RF24Network
    nrf24/RF24Mesh, https://github.com/nRF24/RF24Mesh/releases
*/

#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>

 /*
  * userconfig:
  * name of the node, max 32 chars
  * ID of the node, has to be unique and from 1 - 255
  * location of the hive, max 32 chars
  * TODO: location could be defined at gateway too?
  */
const uint8_t  nodeID = 0;
const String   nodeName = "Gateway";
const String   nodeLocation = "Standort 1";

 /* 
  * Configure the nrf24l01 CE and CS pins
  */
RF24 radio(9, 10);
RF24Network network(radio);       // include the radio in the RF24Network
RF24Mesh mesh(radio, network);    // Create mesh network

 /*
  * Caching behavior
  * maxitems: how many values should the gateway save, before the data must be sent to the mqtt broker
  * maxtime:  how many milliseconds should the gateway wait, before the data must be sent to the mqtt broker
  * whatever happens first, triggers a push to the broker
  */
const unsigned int cache_maxitems = 50;
const unsigned long cache_maxtime = 3600000;


uint32_t displayTimer = 0;        // initalized
uint8_t  cachecounter = 0; 

// struct for keeping and sending data to RF24Mesh network
struct payload_t
{
    long weight;
    String nodeName;
    String nodeLocation;
    int sendErrorCount;
};

void setup()
{
    Serial.begin(115200);

    // node ID 0 for master
    mesh.setNodeID(nodeID);

    Serial.println("Starting mesh network");
    mesh.begin();

    Serial.println("Setup complete.");
}

void loop()
{
    // Call mesh.update to keep the network updated
    mesh.update();

    // In addition, keep the 'DHCP service' running on the master node so addresses will
    // be assigned to the sensor nodes
    mesh.DHCP();

    if (network.available())
    {
        RF24NetworkHeader header;
        network.peek(header);

        payload_t payload;
        switch (header.type)
        {
        // Display the incoming millis() values from the sensor nodes
        case 'D':
            network.read(header, &payload, sizeof(payload));
            break;
        default:
            network.read(header, 0, 0);
            Serial.print("Unknown message type: ");
            Serial.println(header.type);
            break;
        }
    }

    // check, if data should be sent to the broker

    // print connected nodes
    if (millis() - displayTimer > 60000)
    {
        displayTimer = millis();
        Serial.println(" ");
        Serial.println(F("********Assigned Addresses********"));
        for (int i = 0; i < mesh.addrListTop; i++)
        {
            Serial.print("NodeID: ");
            Serial.print(mesh.addrList[i].nodeID);
            Serial.print(" RF24Network Address: 0");
            Serial.println(mesh.addrList[i].address, OCT);
        }
        Serial.println(F("**********************************"));
    }
}