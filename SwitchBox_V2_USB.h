/***************************************************
  This is a library for Switch Box System 

  Designed specifically to work with the Slow Control Board from UCLouvain designed by Antoine Deblaere

  This library will allow you to easily connect to WIFI, communicate with Sensors, and more
  
  Written by Antoine Deblaere for IRMP/CP3
  BSD license, all text above must be included in any redistribution
 ****************************************************/
 
 #ifndef SwitchBox_V2_USB_H
 #define SwitchBox_V2_USB_H
 
 #include "WiFi.h"
 #include "MAX4820.h"
 #include "QL355TP.h"
 #include "Wire.h"
 #include "LCD.h"
 #include "LCD_C0220BiZ.h"
 #include "ST7036.h"
 #include <PubSubClient.h>
 #include "Button.h"
 #include "PinButton.h"
 #include "PCF8575.h"
 #define ARDUINOJSON_USE_LONG_LONG 1
 #include <ArduinoJson.h>
 #include <PriUint64.h>
 
 //PINS of Relays Drivers
 #define DRV_HV 33
 #define DRV_LV 32
 #define MAX_SCLK 23
 #define MAX_CS 17
 #define MAX_RESET 19
 #define MAX_DIN 16
 #define MAX_SET 18
 
 //PINS of button
 #define BTN_MANUAL 25
 #define BTN_REMOTE 26
 #define BTN_LOCAL 27
 
 //Define STATUS
 #define OFF 0
 #define ON 1
 
 //Define LED MUX PINS
 #define CH1 7
 #define CH2 6
 #define CH3 5
 #define CH4 4
 #define CH5 3
 #define CH6 2
 #define CH7 1
 #define CH8 0
 #define CH9 11
 #define CH10 10
 #define CH11 9
 #define CH12 8

 //Define Default MQTT Credentials
 #define SWITCHBOX_DEFAULT_MQTT_SERVER ""
 #define SWITCHBOX_DEFAULT_MQTT_PORT 1883
 #define SWITCHBOX_DEFAULT_MQTT_CLIENT_ID "SwitchBoxBoard"
 #define SWITCHBOX_DEFAULT_NBR_OF_TRY 5
 #define cmd_topic  	"SB/cmd"       //Used to send any types of commands
 #define ch_topic   	"SB/status/CH" //Use to retrieve status each relays channels
 #define lv_topic   	"SB/status/LV" //Use to retrieve LV Status
 #define hv_topic   	"SB/status/HV" //Use to retrieve HV Status
 #define mode_topic 	"SB/status/MODE"      //Use to retrieve the mode which the SB is in
 #define status_topic   "SB/status" //Use to retrieve status each relays channels


 class SwitchBox {
	 public :
        /**
         *  Constructor.
         */
        SwitchBox(int address_PF575,int adress_PF575_2,int address_LCD);
		
		 /**
         * Initialises the Switch Box Library
         */
        void begin(String mqtt_server,char* ssid,char* password,char* id);
		
		/**
         * Make the the Switch Box Run
         */
        void run();
		
		/**
         * Connect to WiFi 
         */
        void connectToWiFi(char* ssid, char* password);
		
		/**
         * Connect as TCP Client
         */
        void connectToTcpClient(char* host, uint16_t port, int number_of_try);
		
		/**
         * Set the MQTT Server harcoded
         *
         * Take parameter of the function and provides it to the MQTT Client
        *
         *If not create you will be able to enter it in the Access Point while configuring the Wifi
         */
        void setMQTTServer(String mqtt_server=SWITCHBOX_DEFAULT_MQTT_SERVER, uint16_t mqtt_port=SWITCHBOX_DEFAULT_MQTT_PORT);
		
		/**
         * Initialises the MQTT Communication
         *
         *Take default MQTT Server, Client ID & TTL Subscription feedback
         *
         *If it doesnt suit, pass yours as paramaters
         */
        void connectToMQTT(int nbr=SWITCHBOX_DEFAULT_NBR_OF_TRY, const char* clientID = SWITCHBOX_DEFAULT_MQTT_CLIENT_ID,bool connectToCMD=false);
		
		/**
         * Check the status of MQTT Connectivity
         *
         *Return true  if connected | False if not
         *
         *If it doesnt suit, pass yours as paramaters
         */
        bool checkMQTTStatus();
		
		/**
         * Publish to MQTT Topic
         *
         *Param 1 : Topic where you want to publish || Param 2 : Data you want to publish
         *
         */
        void publishToMQTT(const char* topic, const char* payload);
		
		/**
         * Subscribe to TTL Status
         *
         *Take default TTL Topic as paramaters
        *
        *If it doesnt suit, pass yours as paramaters
        */
        void subscribeToTopic(bool cmd,bool ch,bool lv, bool hv,bool mode); 
		
		/**
         * Setup callback function 
         *
         * Get feeback from topic that changed if you subscribed to them
         */
        void callbackSB(char* topic, byte* payload, unsigned int length); 
		
		/**
         * Set HV Status
         */
        void set_MAIN_HV(bool status);
		
		/**
         * Set LV Status
         */
        void set_MAIN_LV(bool status);
		
		/**
         * Set LED Status
         */
        void set_LED(int whichLED, bool status,int channel);
		
		/**
         * Shift Out One channel
         */
		void shift_Out(int channel, int status);
		
		/**
         * Print out message to LCD
         */
		void print_LCD(String msg,int line, int start_position,bool haveToClear);
		
		/**
         * Print out message to LCD concering Channels -- Allow to respect the displaying structure
         */
		void print_Channel_LCD(int channel,bool status);
		
		/**
         * Print out message to LCD
         */
		void shift_LCD(int duration,int split);
		
		/**
         * Get Switch Box Mode
         */
		bool get_SwitchBox_Mode();
		
		/**
         * Setup LOCAL MODE
         */
		void local_Mode_Setup();
		
		/**
         * Setup REMOTE MODE
         */
		void remote_Mode_Setup();
		
		/**
         * Reset Board to Default Values
         */
		void reset_Board();	
		
	private:
		/**
         * Constructor
         */
		 WiFiClient espClient;
		 WiFiClient tcpClient;
		 PubSubClient mqttClient;
		 
		 /**
         * Variables
         */
		 bool mqttServerSet;
		 char myMqttServer[40];
		 String myStrMQTTSERVER;
		 char* mySSID;
		 char* myPASSWORD;
		 char* myID;
		 bool myConnectToCMD;
		 int myLedPins[49]= {0,7,6,5,4,3,2,1,0,11,10,9,8,7,6,5,4,3,2,1,0,11,10,9,8,7,6,5,4,3,2,1,0,11,10,9,8,7,6,5,4,3,2,1,0,11,10,9,8};
		 int lcd_position_topleft[12] = {1,5,9,13,17,21,25,29,33,37,41,45};
		 int lcd_position_topright[12] = {2,6,10,14,18,22,26,30,34,38,42,46};
		 int lcd_position_bottomleft[12] = {3,7,11,15,19,23,27,31,35,39,43,47};
		 int lcd_position_bottomright[12] = {4,8,12,16,20,24,28,32,36,40,44,48};
		 int lcd_position; //1 = top left || 2 = top right || 3 = bottom left || 4 = bottom right
		 bool switchBoxMode; // 0 IS LOCAL -- 1 IS REMOTE
		 bool localMode_Setup;
		 bool remoteMode_Setup;
		 int counter_Channel_local;
		 bool bundleLedSet;
		 char output_JSON_char[100];
		 
		 /**
         * Topics Variables
         */
		 bool lv_status;
		 bool hv_status;
		 bool mode_status;
		 int ch_status[48] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		 bool bundle1_status;
		 bool bundle2_status;
		 bool bundle3_status;
		 bool bundle4_status;
		 
		 /**
         * Increase Button Variables
         */
		 int previous_state_btn;
		 bool status_btn_increase;
		 
		 /**
         *  Private Functions
         */
        String byteArrayToString( byte*payload, unsigned int length);
		
		/**
         *  Set LED Bundle
         */
		void set_Bundle_Led(int whichChannel);
		
		/**
         *  Read Increase Button
         */
		void readIncreaseBtn();
		
		/**
         * Print HEXA
         */
		void printHex(uint16_t val);
		
		/**
         * Placeholder to track the I2C address of the first PF575.
         */
        uint8_t _i2caddr_PF575;
		
		/**
         * Placeholder to track the I2C address of the second PF575.
         */
        uint8_t _i2caddr_PF575_2;
		
		/**
         * Placeholder to track the I2C address of PF575.
         */
        uint8_t _i2caddr_LCD;
		
		/**
         * Placeholder to track the status of all the LEDS.
         */
		 uint16_t data_led;
		 
		 /**
         * Placeholder to track the status of all the LEDS.
         */
		 uint64_t data_relay;
		
		/**
         * Set IO of the PF575 as output.
         */
        void set_IO_PCF575_OUTPUT(uint16_t data, int address);
		
		/**
         * Turn all relays OFF
         */
        void turn_All_Relays_OFF();
		
		/**
         * Receive Data trough serial without interrupting all the code
         */
		void receiveWithEndMarker();
		
		/**
         * Get the data from Serial when end character has been meet
         */
		void showNewData();
		
		
		 
		 /**
         * 
         */
		 
		const static byte numChars = 32;
		
		char receivedChars[numChars];   // an array to store the received data

		boolean newData = false;
		
		const char endMarkerOfString[1] = {'\r'};
		
		char charToBeCompared[numChars];
		

		
 };
#endif 