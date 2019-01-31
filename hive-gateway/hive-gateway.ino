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

/**** Configure the nrf24l01 CE and CS pins ****/
RF24 radio(9, 10);
RF24Network network(radio);       // include the radio in the RF24Network
RF24Mesh mesh(radio, network);    // Create mesh network

const uint8_t nodeID = 0;

/**
 * enabled verbose output 
**/
#define DEBUG

uint32_t displayTimer = 0;        // initalized 

// struct for keeping and sending data to RF24Mesh network
struct payload_t
{
    unsigned long weight;
};

void setup()
{
    Serial.begin(115200);

    // node ID 0 for master
    mesh.setNodeID(nodeID);

    #ifdef DEBUG
        Serial.print("DEBUG: This nodes id is: ");
        Serial.println(mesh.getNodeID());
    #endif
    
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
        #ifdef DEBUG
            Serial.println("DEBUG: network.available branch.");
        #endif
        RF24NetworkHeader header;
        network.peek(header);
        #ifdef DEBUG
            Serial.println("DEBUG: network.peek(header) complete.");
        #endif

        payload_t payload;
        switch (header.type)
        {
        // Display the incoming millis() values from the sensor nodes
        case 'D':
            #ifdef DEBUG
                Serial.println("DEBUG: Received data packge (Type D)");
            #endif
            network.read(header, &payload, sizeof(payload));
            Serial.println(payload.weight);
            break;
        default:
            network.read(header, 0, 0);
            Serial.print("Unknown message type: ");
            Serial.println(header.type);
            break;
        }
    }

    // print connected nodes
    if (millis() - displayTimer > 5000)
    {
        #ifdef DEBUG
            Serial.println("DEBUG: timed adress listing triggered.");
        #endif
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
