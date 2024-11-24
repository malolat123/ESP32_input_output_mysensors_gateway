/********************************
 *
 * REVISION HISTORY
 * Version 2.02 - AT
 * v 2.01
 * Rewrite for esp32 + wifi support (8 relay)
 * v 2.02
 * Rewrite outputs to pcf8575 //removed
 * v 2.03 
 * Rewrite input to pcf8575
 *
 ********************************/


#define MY_GATEWAY_ESP32

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enables and select radio type (if attached)
//#define MY_RADIO_RF24
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#define MY_GATEWAY_ESP32

#define MY_WIFI_SSID "My WIFI"
#define MY_WIFI_PASSWORD "1234567890"
// Enable UDP communication
//#define MY_USE_UDP  // If using UDP you need to set MY_CONTROLLER_IP_ADDRESS or MY_CONTROLLER_URL_ADDRESS below

// Set the hostname for the WiFi Client. This is the hostname
// passed to the DHCP server if not static.
#define MY_HOSTNAME "ESP32_GW"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
//#define MY_IP_ADDRESS 192,168,1,100

// The port to keep open on node server mode
#define MY_PORT 5003
// How many clients should be able to connect to this gateway (default 1)
#define MY_GATEWAY_MAX_CLIENTS 2


#include <MySensors.h>

#include "PCF8575.h"
PCF8575 PCF(0x20);

#define NUMBER_OF_RELAYS 16
#define RELAY_ON 1
#define RELAY_OFF 0

int relayPin[NUMBER_OF_RELAYS] = { 15, 4, 16, 17, 5, 18, 19, 23, 13, 12, 14, 27, 26, 25, 33, 32 };

//for debouncing
#define noEvent 0
#define shortPress 1
#define longPress 2
#define minimalTimeDuration 30
#define shortTime 1000
#define timeout 2000
#define statusOff PCF8575_INITIAL_VALUE

#define BUTTON_FIRST_ID 0
#define BINARY_FIRST_ID 100

MyMessage msg[NUMBER_OF_RELAYS];
MyMessage msgBinary[NUMBER_OF_RELAYS];

void before() {
}

void setup() {

#ifdef MY_DEBUG
  Serial.begin(115200);
  delay(300);
  Serial.print("Hello, Start at:  ");
  Serial.println(millis());
#endif

  Wire.begin();
  PCF.begin();
  PCF.selectNone();

#ifdef MY_DEBUG
  Serial.print("PCF... ");
  if (!PCF.isConnected()) {
    Serial.println("=> not connected");
  } else {
    Serial.println("=> connected!!");
  }
#endif


#ifdef MY_DEBUG
  Serial.print("PCF_Status: ");
  Serial.println(~PCF.readButton16(), BIN);
#endif


  for (int i = 0; i < NUMBER_OF_RELAYS; i++) {
    msg[i].sensor = i + BUTTON_FIRST_ID;
    msg[i].type = V_LIGHT;
    msgBinary[i].sensor = i + BINARY_FIRST_ID;  // initialize messages
    msgBinary[i].type = V_STATUS;
  }
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Relay", "2.03");

  for (int i = 0; i < NUMBER_OF_RELAYS; i++) {
    present(BUTTON_FIRST_ID + i, S_BINARY);
    present(BINARY_FIRST_ID + i, S_BINARY);
  }
}


void receive(const MyMessage& message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type == V_LIGHT) {
    // Change relay state
    switchRelay(message.sensor, message.getBool() ? RELAY_ON : RELAY_OFF);
    saveState(message.sensor, message.getBool());

#ifdef MY_DEBUG
    // Write some debug info
    Serial.print("Incoming change for sensor:");
    Serial.print(message.sensor);
    Serial.print(", New status: ");
    Serial.println(message.getBool());
#endif
  }
}

void loop() {
  // Send locally attached sensor data here
  checkInputs();
}

void checkInputs(void) {
  unsigned long buttonPressStartTimeStamp;
  unsigned long buttonPressDuration = 0;

  uint16_t status_act = PCF.readButton16(); //check status
  if (status_act != statusOff) {  //somethings changed
    buttonPressStartTimeStamp = millis();

#ifdef MY_DEBUG
    Serial.print("key(status_act):BIN: ");
    Serial.print(~status_act, BIN);
#endif

#ifdef MY_DEBUG
    Serial.print("buttonPressStartTimeStamp: ");
    Serial.println(buttonPressStartTimeStamp);
#endif
                                                                                     
    while ((PCF.readButton16() == status_act) && (buttonPressDuration < timeout)) {  //check if status back to normal or timeout
      buttonPressDuration = (millis() - buttonPressStartTimeStamp);
    }


#ifdef MY_DEBUG
    Serial.print("buttonPressDuration: ");
    Serial.println(buttonPressDuration);
#endif

    if (buttonPressDuration > minimalTimeDuration) {  
      byte event = noEvent;
      (buttonPressDuration <= shortTime) ? event = shortPress : event = longPress;
      
      //finding button no.
      byte button = 0;
      while (bitRead(status_act, button) != 0) {
        button++;
      }
      
      switch (event) {
        case shortPress:
          saveState(button, !loadState(button));
          switchRelay(button, loadState(button) ? RELAY_ON : RELAY_OFF);
          send(msg[button].set(loadState(button)));
          break;
        case longPress:
          saveState(button + NUMBER_OF_RELAYS, RELAY_ON);
          send(msgBinary[button].set(loadState(button + NUMBER_OF_RELAYS)));
          delay(500);
          saveState(button + NUMBER_OF_RELAYS, RELAY_OFF);
          send(msgBinary[button].set(loadState(button + NUMBER_OF_RELAYS)));
          break;
        case noEvent:
        default:
          break;
      }
    }
  }
}

void switchRelay(byte relay, byte state) {
  digitalWrite(relayPin[relay], state);


#ifdef MY_DEBUG
  Serial.print("SwitchRelay: ");
  Serial.print(relay);
  Serial.print(", to State: ");
  Serial.println(state);
#endif
}

