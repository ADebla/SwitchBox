/***************************************************
  This is a library for Switch Box System 

  Designed specifically to work with the Slow Control Board from UCLouvain designed by Antoine Deblaere

  This library will allow you to easily connect to WIFI, communicate with Sensors, and more
  
  Written by Antoine Deblaere for IRMP/CP3
  BSD license, all text above must be included in any redistribution
 ****************************************************/
 
 #include "SwitchBox_V2_USB.h"
 
 MAX4820 my_relay_driver;
 QL355TP my_power_supply;
 ST7036 lcd = ST7036(2,16,0x78); //Default configurations
 Button btn_remote(BTN_REMOTE);
 Button btn_local(BTN_LOCAL);
 Button btn_manual(BTN_MANUAL);
 PCF8575 pcfBtn(0x21);
 //PinButton btn_lastmanual(BTN_MANUAL);
//JSON Definitions
 DynamicJsonBuffer jsonBuffer;
 String format_JSON = "{\"lv\":0,\"hv\":0,\"mode\":0,\"channels\":0}";
 JsonObject& root = jsonBuffer.parseObject(format_JSON);
 String output_JSON="";
 
  
  SwitchBox::SwitchBox(int adress_PF575,int adress_PF575_2, int address_LCD) {
	  _i2caddr_PF575 = adress_PF575;
	  _i2caddr_PF575_2 = adress_PF575_2;
	  _i2caddr_LCD = address_LCD;
	  data_led=0xFFFF;
	  data_relay=0x000000000000;
	  mqttServerSet=false;
	  myStrMQTTSERVER="";
	  myConnectToCMD=false;
	  localMode_Setup=false;
	  remoteMode_Setup=false;
	  counter_Channel_local=0;
	  lcd_position=0;
	  bundleLedSet = false;
	  
	  //Initiate Status for MQTT Topics
	  lv_status=false;
	  hv_status=false;
	  mode_status=false;
	  
	  //Variables setup for Increase Button
	  previous_state_btn = true;
	  status_btn_increase = false;
	  
	  //LCD Definition
	  lcd = ST7036(2,16,_i2caddr_LCD);
	  pcfBtn = PCF8575(adress_PF575_2);
	  
	  //Setup JSON Values
	  root["lv"] = 0;
	  root["hv"] = 0;
	  root["mode"] = 0;
	  root["channels"] = 0;
	  
	  //Setup Bundle Status Values
	  bundle1_status=false;
	  bundle2_status=false;
	  bundle3_status=false;
	  bundle4_status=false;
}

void SwitchBox::begin(String mqtt_server,char* ssid,char* password,char* id)
{
	//Start Serial
	Serial.begin(115200);
	
	//Start I2C Communication
	Wire.begin();
	//Wire.setClock(400000L);
	
	//Initiate MAX4820 Drivers
	my_relay_driver.begin(MAX_RESET,MAX_SET,MAX_CS,MAX_DIN,MAX_SCLK);
	
	//Initiate PCF8575 Led Drivers -- SET ON OUTPUT -- For First PCF
	set_IO_PCF575_OUTPUT(word(B11111111,B11111111),_i2caddr_PF575);
	
	//Initiate PCF8575 Number 2 -- SET ON INPUT
	//set_IO_PCF575_OUTPUT(word(B11111111,B11111111),_i2caddr_PF575_2);
	pcfBtn.begin();
	
	//Setup Driving PINS
	pinMode(DRV_HV, OUTPUT);
	pinMode(DRV_LV, OUTPUT);
	digitalWrite(DRV_HV,LOW);
	digitalWrite(DRV_LV,LOW);
	
	//Start Button
	btn_remote.begin();
	btn_local.begin();
	btn_manual.begin();
	
	//Pass WiFi Connection parameters to MQTT
	mqttClient.setClient(espClient);
	
	//TURN ALL RELAYS OFF
	turn_All_Relays_OFF();
	
	//Connect TCP/IP
	//connectToTcpClient("192.168.0.106",9221,3);
	
	my_power_supply.begin(tcpClient);
	
	//my_power_supply.connect("192.168.0.113",9221,5);
	
	//my_power_supply.set_voltage(1,1);
	
	delay(500);
	
	//LCD Begin
	lcd.init();
	lcd.clear();
	lcd.home();
	
	delay(500);
	
	//Print Welcome
	lcd.setCursor(0,5);
	lcd.print("WELCOME");
	lcd.setCursor(2,3);
	lcd.print("CP3   USER");
	delay(1000);
	
	//Assign values to global Variables
	myStrMQTTSERVER=mqtt_server;
	mySSID=ssid;
	myPASSWORD=password;
	myID=id;
}

void SwitchBox::run()
{
	//Check Serial Input
	receiveWithEndMarker();
	showNewData();
	
	//Verify is Switch Box has not changed
	if(get_SwitchBox_Mode()) //REMOTE MODE
	{
		if(remoteMode_Setup==false)
		{
			remote_Mode_Setup();
		}
		else
		{
			//LOOP for not losing connection
			mqttClient.loop();
			
			//WAIT MQTT Commands
		}
	}
	else //LOCAL MODE
	{
		if(localMode_Setup==false)
		{
			local_Mode_Setup();
		}
		else
		{
			if(btn_manual.pressed())
			{
				if(counter_Channel_local == 0)
				{
					counter_Channel_local++; // Pass to 1
					
					//HV OFF
					set_MAIN_HV(0);
					delay(1000);
					//LV OFF
					set_MAIN_LV(0);
					delay(250);
					
					Serial.println(counter_Channel_local);
					
					//Set ON First CHANNEL
					shift_Out(counter_Channel_local,true);
					//set_Bundle_Led(counter_Channel_local);
					set_LED(myLedPins[counter_Channel_local],false,counter_Channel_local);
					print_Channel_LCD(counter_Channel_local,true);
					
					//LV ON
					set_MAIN_LV(1);
					delay(1000);
					//HV ON
					set_MAIN_HV(1);
					delay(250);
				}
				//Decrement counter between 48 & 2 ( so max is 48 and min is 1
				else if(counter_Channel_local<=48 && counter_Channel_local>=2) 
				{
					Serial.print("Channel Avant : ");
					Serial.print(counter_Channel_local);
					counter_Channel_local--;
					Serial.print("Channel Apres : ");
					Serial.print(counter_Channel_local);
					
					//HV OFF
					set_MAIN_HV(0);
					delay(1000);
					//LV OFF
					set_MAIN_LV(0);
					delay(250);

					//SET OFF next channel but positive 
					shift_Out(counter_Channel_local+1,false);
					//set_Bundle_Led(counter_Channel_local);
					set_LED(myLedPins[counter_Channel_local+1],true,counter_Channel_local);
					print_Channel_LCD(counter_Channel_local+1,false);
					delay(250);

					//SET ON previous channel
					shift_Out(counter_Channel_local,true);
					set_LED(myLedPins[counter_Channel_local],false,counter_Channel_local);
					print_Channel_LCD(counter_Channel_local,true);

					//LV ON
					set_MAIN_LV(1);
					delay(1000);
					//HV ON
					set_MAIN_HV(1);
					delay(250);
				}
			}
			
			//Read Button
			readIncreaseBtn();
			
			//Reset Status Btn
			if(status_btn_increase == true)
			{
				//Increment counter
				counter_Channel_local++;		
				
				if(counter_Channel_local>48)
				{
					//Reset Variables
					counter_Channel_local=1;
					data_led=0xFFFF;
					data_relay=0x000000000000;
				}
				
				if(counter_Channel_local==1)
				{
					//HV OFF
					set_MAIN_HV(0);
					delay(1000);
					//LV OFF
					set_MAIN_LV(0);
					delay(250);
					
					//Set ON First CHANNEL
					shift_Out(counter_Channel_local,true);
					//set_Bundle_Led(counter_Channel_local);
					set_LED(myLedPins[counter_Channel_local],false,counter_Channel_local);
					print_Channel_LCD(counter_Channel_local,true);
					
					//LV ON
					set_MAIN_LV(1);
					delay(1000);
					//HV ON
					set_MAIN_HV(1);
					delay(250);
				}
				else
				{
					//HV OFF
					set_MAIN_HV(0);
					delay(1000);
					//LV OFF
					set_MAIN_LV(0);
					delay(250);
					
					//SET OFF previous channel
					shift_Out(counter_Channel_local-1,false);
					//set_Bundle_Led(counter_Channel_local);
					set_LED(myLedPins[counter_Channel_local-1],true,counter_Channel_local);
					print_Channel_LCD(counter_Channel_local-1,false);
					delay(250);
					
					//SET ON next channel
					shift_Out(counter_Channel_local,true);
					set_LED(myLedPins[counter_Channel_local],false,counter_Channel_local);
					print_Channel_LCD(counter_Channel_local,true);
					
					//LV ON
					set_MAIN_LV(1);
					delay(1000);
					//HV ON
					set_MAIN_HV(1);
					delay(250);
				}
				
				Serial.println("Button pressed");
				
				//Reset Button Status
				status_btn_increase=false;
			}
		}
	}
	/*if(checkMQTTStatus())
	{
		
	}
	else if (checkMQTTStatus()==false && switchBoxMode==true)
	{
		Serial.println("Disconnected from MQTT Server");
		print_LCD("Disconnected",0,3,true);
		print_LCD("MQTT Server",2,3,false);
		delay(2000);
		
		remote_Mode_Setup();
	}*/
}

void SwitchBox::connectToWiFi(char* _ssid, char* _password)
{
	WiFi.begin(_ssid,_password);
	
	while (WiFi.status() != WL_CONNECTED)
	{
		Serial.println("Connecting to WiFi");
		print_LCD("Connecting to WiFi..",0,0,true);
		shift_LCD(1000,1);
	}
	Serial.println("Connected to WiFi network");
	delay(100);
	print_LCD("Connected to WiFi..",0,0,true);
	shift_LCD(600,1);
}

void SwitchBox::connectToTcpClient(char* _host, uint16_t _port, int number_of_try)
{
	my_power_supply.connect(_host,_port,number_of_try);
}

void SwitchBox::setMQTTServer(String mqtt_server,uint16_t mqtt_port)
{
	if(mqtt_server!="")
	{
	  //Get reference of server
	  mqtt_server.toCharArray(myMqttServer,15);
	  
      //Setup MQTT Server    
      mqttClient.setServer(myMqttServer,mqtt_port);
    
      //Setup Callback
      mqttClient.setCallback(([this] (char* topic, byte* payload, unsigned int length) { this->callbackSB(topic, payload, length); }));
	  
	  //Status of set
	  mqttServerSet=true;
	}
	else
	{
		Serial.println("MQTT Serverd not hardcoded");
	}
}

void SwitchBox::connectToMQTT(int nbr,const char* clientID,bool connectToCMD)
{
	myConnectToCMD=connectToCMD;
	
	while (!mqttClient.connected())
	{
		Serial.println("Attempting MQTT Connection... targeting -> "+String(myMqttServer));
		
		print_LCD("Attempting",0,3,true);
		print_LCD("MQTT  Connection",2,0,false);
		
		if(mqttClient.connect(clientID))
		{
			Serial.println("Connected to MQTT Server.");
			Serial.println();
			delay(100);
			print_LCD("SWITCH BOX",0,3,true);
			print_LCD("READY",2,5,false);
			delay(2000);
			
			if(connectToCMD==true)
			{
				//Sub to Topics
				subscribeToTopic(true,false,false,false,false);

				Serial.println("Subscribed to Switch BOX TOPICS");
				Serial.println();
			}
			else
			{
				Serial.println("You didn't subcrisbed to Switch BOX TOPICS");
				Serial.println();
			}
			break;
		}
		else
		{
			//Check Button again
			if(get_SwitchBox_Mode()==false)
			{
				break;
			}
			delay(1500);
		}
	}
	
	
	
	
   /* //Try to connect to MQTT Server 5 times | 5 S timeout
    while (!mqttClient.connected())
    {
		
        for (int i=0;i<nbr;i++)
        {
			Serial.println("Attempting MQTT Connection..."+String(i+1)+"/\0"+String(nbr)+ " targeting -> "+String(myMqttServer));
			print_LCD("Attempting",0,3,true);
			print_LCD("MQTT Connection",2,1,false);

            if(mqttClient.connect(clientID))
            {
                Serial.println("Connected to MQTT Server.");
                Serial.println();
				delay(100);
				print_LCD("SWITCH BOX",0,3,true);
				print_LCD("READY",2,5,false);
                
                if(connectToCMD==true)
                {
					//Sub to Topics
                    subscribeToTopic(true,false,false,false,false);
					
                    Serial.println("Subscribed to Switch BOX TOPICS");
                    Serial.println();
                }
                else
                {
                    Serial.println("You didn't subcrisbed to Switch BOX TOPICS");
                    Serial.println();
                }
                break;
            }
            else
            {
				delay(5000);
            }
        }
        Serial.println();
        break;
    }*/
}

bool SwitchBox::checkMQTTStatus()
{
	if(mqttClient.connected())
	{
		return true;
	}
	else
	{
		return false;
	}
}

void SwitchBox::publishToMQTT(const char* topic,const char* payload) 
{
    mqttClient.publish(topic,payload);
}

void SwitchBox::subscribeToTopic(bool cmd,bool ch,bool lv, bool hv,bool mode) 
{
	if(cmd)
	{
		mqttClient.subscribe(cmd_topic);	
	}
	
	if(ch)
	{
		mqttClient.subscribe(ch_topic);
	}
	
	if(lv)
	{
		mqttClient.subscribe(lv_topic);	
	}
	
	if(hv)
	{
		mqttClient.subscribe(hv_topic);
	}
	
	if(mode)
	{
		mqttClient.subscribe(mode_topic);
	}
	
}

void SwitchBox::callbackSB(char* topic, byte* payload, unsigned int length)
{
    String data;
	String extracted_data;
	int channel;
	bool status;
	char bool_char[2];
	char ch_char[49];
	
	//Get DATA
	data = byteArrayToString(payload,length);
	data.trim();
	
	Serial.println(data);
	
    if(String(topic) == cmd_topic)
    {
		extracted_data=data;
		
		if(data.indexOf("CH")>=0 && data.indexOf("|")>0)
		{
			//Get Channel
			int index = data.indexOf('|');
			extracted_data = extracted_data.substring(0,index);
			Serial.println(extracted_data);
			extracted_data.remove(0,2);
			channel = extracted_data.toInt();
			
			//Get Status
			data= data.substring(data.lastIndexOf('|')+1);
			data.trim();
			
			if( data =="ON")
			{
				status=true;
			}
			else
			{
				status= false;
			}
			//Shift Out Relay
			shift_Out(channel,status);
			
			//Set Led 
			set_LED (myLedPins[channel],!status,channel);
			
		}
		else if (data.indexOf("LV")>=0 && data.indexOf("|")>0)
		{
			//Status
			if(data.indexOf("ON")>=0)
			{
				status=true;
			}
			else
			{
				status=false;
			}
			set_MAIN_LV(status);
		}
		else if (data.indexOf("HV")>=0 && data.indexOf("|")>0)
		{
			//Status
			if(data.indexOf("ON")>=0)
			{
				status=true;
			}
			else
			{
				status=false;
			}
			set_MAIN_HV(status);
		}
		else if (data =="CH?")
		{
			//Assign Buffer
			for ( int i=0;i<48;i++)
			{
				sprintf(ch_char+i,"%d",ch_status[i]);
			}

			//Publish to Topic
			publishToMQTT(ch_topic,ch_char);
		}
		else if (data =="LV?")
		{
			//Assign Buffer 
			sprintf(bool_char,"%d",lv_status);

			//Publish to Topic
			publishToMQTT(lv_topic,bool_char);
		}
		else if(data == "HV?")
		{
			//Assign Buffer 
			sprintf(bool_char,"%d",hv_status);

			//Publish to Topic
			publishToMQTT(hv_topic,bool_char);
		}
		else if(data == "MODE?")
		{
			//Assign Buffer 
			sprintf(bool_char,"%d",mode_status);

			//Publish to Topic
			publishToMQTT(mode_topic,bool_char);
		}
		else if(data == "STATUS?")
		{
			uint64_t value = 0;
			
			//Translate & Count
			for (uint8_t i=0; i<(sizeof(ch_status)/sizeof(int));i++)
			{
				if (ch_status[i] == 1)
				{
					uint64_t temp_val = 1;
					
					int index = i;
					
					for(uint8_t i=0;i<index;i++)
					{
						temp_val = temp_val*2;
					}
					value +=temp_val;
				}
			}
			
			Serial.println(PriUint64<DEC>(value));
			
			root["channels"] = value;
			
			//Store JSON Data into Char array
			root.printTo(output_JSON_char);	
			
			//Publish MQTT
			publishToMQTT(status_topic,output_JSON_char);
		}
    }
}

void SwitchBox::set_MAIN_LV(bool status)
{
	if(status==true)
	{
		digitalWrite(DRV_LV,HIGH);
		root["lv"] = 1;
	}
	else
	{
		digitalWrite(DRV_LV,LOW);
		root["lv"] = 0;
	}
	
	//Assign Status to LV
	lv_status=status;
}

void SwitchBox::set_MAIN_HV(bool status)
{
	if(status==true)
	{
		digitalWrite(DRV_HV,HIGH);
		root["hv"] = 1;
	}
	else
	{
		digitalWrite(DRV_HV,LOW);
		root["hv"] = 0;
	}
	
	//Assign Status to HV
	hv_status=status;
}

void SwitchBox::set_LED(int whichLED, bool status,int channel)
{
	//Power up the needed channels LEDS
	if(whichLED<=12)
	{
		if(status==1)
		{
			data_led |= 1<<whichLED;

		}
		else
		{
			data_led &= ~(1<<whichLED);
		}
	}
	
	//12 = BUNDLE 1
	//13 = BUNDLE 2
	//14 = BUNDLE 3
	//15 = BUNDLE 4
	//Power up the needed bundle LEDS
	
	uint8_t result = 0;
	
	for (uint8_t i=0; i<4;i++)
	{
		for(uint8_t j=0;j<12;j++)
		{		
			result += ch_status[(i*12)+j];
			
		}		
		
		//Check Result
		if(i==0)
		{
			if(result>0)
			{
				bundle1_status==true;
				data_led &= ~(1<<12); // BUNDLE 1 | ON
			}
			else
			{
				bundle1_status==false;
				data_led |= 1<<12;// BUNDLE 1 | OFF
			}	
		}
		else if(i==1)
		{
			if(result>0)
			{
				bundle2_status==true;
				data_led &= ~(1<<13); // BUNDLE 2 | ON
			}
			else
			{
				bundle2_status==false;
				data_led |= 1<<13;// BUNDLE 2 | OFF
			}	
		}
		else if(i==2)
		{
			if(result>0)
			{
				bundle3_status==true;
				data_led &= ~(1<<14); // BUNDLE 3 | ON
			}
			else
			{
				bundle3_status==false;
				data_led |= 1<<14;// BUNDLE 3 | OFF
			}	
		}
		if(i==3)
		{
			if(result>0)
			{
				bundle4_status==true;
				data_led &= ~(1<<15); // BUNDLE 4 | ON
			}
			else
			{
				bundle4_status==false;
				data_led |= 1<<15;// BUNDLE 4 | OFF
			}	
		}
		result=0;
	}
	
	
	
	/*if(channel <= 12)
	{
		if(bundleLedSet == false)
		{
			data_led &= ~(1<<12); // BUNDLE 1 | ON
			data_led |= 1<<13;// BUNDLE 2 | OFF
			data_led |= 1<<14;// BUNDLE 3 | OFF
			data_led |= 1<<15;// BUNDLE 4 | OFF
			bundleLedSet = true;
		}
		
		//Check to reset bundleLED
		if ( channel ==12)
		{
			bundleLedSet = false;
		}
	}
	
	if(channel>12 && channel <=24)
	{
		if(bundleLedSet == false)
		{
			data_led &= ~(1<<13); // BUNDLE 2 | ON
			data_led |= 1<<12;// BUNDLE 1 | OFF
			data_led |= 1<<14;// BUNDLE 3 | OFF
			data_led |= 1<<15;// BUNDLE 4 | OFF
			bundleLedSet = true;
		}
		
		//Check to reset bundleLED
		if ( channel ==24)
		{
			bundleLedSet = false;
		}
	}
	
	if(channel>24 && channel<=36)
	{
		if(bundleLedSet == false)
		{
			data_led &= ~(1<<14); // BUNDLE 3 | ON
			data_led |= 1<<12;// BUNDLE 1 | OFF
			data_led |= 1<<13;// BUNDLE 2 | OFF
			data_led |= 1<<15;// BUNDLE 4 | OFF
			bundleLedSet = true;
		}
		
		//Check to reset bundleLED
		if ( channel ==36)
		{
			bundleLedSet = false;
		}
	}
	
	if(channel>36 && channel<=48)
	{
		if(bundleLedSet == false)
		{
			data_led &= ~(1<<15); // BUNDLE 4 | ON
			data_led |= 1<<12;// BUNDLE 1 | OFF
			data_led |= 1<<13;// BUNDLE 2 | OFF
			data_led |= 1<<14;// BUNDLE 3 | OFF
			bundleLedSet = true;
		}
		
		//Check to reset bundleLED
		if ( channel ==48)
		{
			bundleLedSet = false;
		}
	}*/

  //Transmit DATA to I2C
  Wire.beginTransmission(_i2caddr_PF575);
  Wire.write(lowByte(data_led));
  Wire.write(highByte(data_led));
  Wire.endTransmission();
}

void SwitchBox::shift_Out(int channel, int status)
{
	
	int my_result = 47-(channel-1);
	
	if (status == 1)
	{
		data_relay |= 1ULL << my_result;
	}
	else
	{
		data_relay &= ~(1ULL << my_result);
	}
	
	//Serial.println(data_relay,HEX);
	
	//Allow relays status change
	my_relay_driver.allow_Changes();
	
	//Assin status to global var
	ch_status[channel-1]=status;
	
	//Send Data to Relay Drivers
	my_relay_driver.shift_Out_Relay_driver(LSBFIRST,data_relay);
}

void SwitchBox::set_IO_PCF575_OUTPUT(uint16_t data,int address)
{
  Wire.beginTransmission(address);
  Wire.write(lowByte(data));
  Wire.write(highByte(data));
  Wire.endTransmission();
}

void SwitchBox::turn_All_Relays_OFF()
{
	my_relay_driver.turn_all_off();
}

void SwitchBox::print_LCD(String msg,int line, int start_position,bool haveToClear)
{
	//Clear LCD
	if(haveToClear)
	{
		lcd.clear();
		lcd.home();
	}

	//Set Cursor
	lcd.setCursor(line,start_position);
	lcd.print(msg);
}

void SwitchBox::print_Channel_LCD(int channel,bool status)
{
	int size = sizeof(lcd_position_topleft)/sizeof(int);

	//Check Which Array the value is involved in
	for(int x=0;x<sizeof(lcd_position_topleft)/sizeof(int);x++)
	{
		if(channel == lcd_position_topleft[x])
		{
			lcd_position=1;
			break;
		}
		else if (channel == lcd_position_topright[x])
		{
			lcd_position=2;
			break;
		}
		else if (channel == lcd_position_bottomleft[x])
		{
			lcd_position=3;
			break;
		}
		else if (channel == lcd_position_bottomright[x])
		{
			lcd_position=4;
			break;
		}
	}
	
	//Display character
	switch (lcd_position)
	{
		case 1 :
			if(status==1)
			{
				print_LCD(String(channel)+"-ON ",0,0,true);
			}
			else
			{
				print_LCD(String(channel)+"-OFF ",0,0,true);
			}
			break;
		case 2 :
			if(status==1)
			{
				print_LCD(String(channel)+"-ON ",0,8,false);
			}
			else
			{
				print_LCD(String(channel)+"-OFF ",0,8,false);
			}
			break;
		case 3 :
			if(status==1)
			{
				print_LCD(String(channel)+"-ON ",2,0,false);
			}
			else
			{
				print_LCD(String(channel)+"-OFF ",2,0,false);
			}
			break;
		case 4 :
			if(status==1)
			{
				print_LCD(String(channel)+"-ON ",2,8,false);
			}
			else
			{
				print_LCD(String(channel)+"-OFF ",2,8,false);
			}
			break;
		default :
			break;
	}
}

void SwitchBox::shift_LCD(int duration,int split)
{
	delay(300);
	int result= ((duration-300)/split);
	for(int i=0;i<=split;i++)
	{
		lcd.command(0x18);
		delay(result);
	}
}

bool SwitchBox::get_SwitchBox_Mode()
{
	if (btn_remote.pressed())
	{
		switchBoxMode=true;
		root["mode"] = 1;
		Serial.println("remote pressed");
	}
	else if (btn_local.pressed())
	{
		switchBoxMode=false;
		root["mode"] = 0;
		Serial.println("local pressed");
	}
	return switchBoxMode;
}

void SwitchBox::local_Mode_Setup()
{
	//Assign value to mode
	mode_status=false;
	
	//Reset Board
	reset_Board();
	
	//Disconnect WiFi & MQTT
	if(mqttClient.connected())
	{
		mqttClient.disconnect();
		Serial.println("Disconnected from MQTT Server.");
	}
	if(WiFi.status()== WL_CONNECTED)
	{
		WiFi.disconnect();
		Serial.println("Disconnected from WiFi Network.");
	}
	
	//Print LCD
	print_LCD("MANUAL",0,5,true);
	print_LCD("MODE",2,6,false);
	
	//SET variables to default values
	counter_Channel_local=0;
	remoteMode_Setup=false;
	localMode_Setup=true;
}

void SwitchBox::remote_Mode_Setup()
{
	//Reset Board
	reset_Board();
	
	//SET MQTT SERVER Reference
	setMQTTServer(myStrMQTTSERVER);
	
	//Connect to WiFi if needed
	if((WiFi.status() != WL_CONNECTED))
	{
		connectToWiFi(mySSID,myPASSWORD);
	}
	
	//Connect to MQTT
	connectToMQTT(SWITCHBOX_DEFAULT_NBR_OF_TRY,myID,true);
	
	//Print LCD
	print_LCD("REMOTE",0,5,true);
	print_LCD("MODE",2,6,false);
	
	//SET variables to default values
	localMode_Setup=false;
	remoteMode_Setup=true;
	
	//Assign value to mode
	mode_status=true;
}

void SwitchBox::reset_Board()
{
	//HV OFF
	digitalWrite(DRV_HV,LOW);
	
	//LV OFF
	digitalWrite(DRV_LV,LOW);
	
	//Turn all relays OFF
	turn_All_Relays_OFF();
	
	//Turn off ALL LEDS
	set_IO_PCF575_OUTPUT(word(B11111111,B11111111),_i2caddr_PF575);
	
	//Set Variables to default Values
	data_led=0xFFFF;
	data_relay=0x000000000000;
}

//Private Fuctions
String SwitchBox::byteArrayToString( byte*payload, unsigned int length)
{
  String myMsg;
  
  for (int i = 0; i < length; i++) 
    {
      myMsg+= (char)payload[i];
    }
  return myMsg;
}

void SwitchBox::set_Bundle_Led(int whichChannel)
{
	if(whichChannel<=12)
	{
		set_IO_PCF575_OUTPUT(word(B11101111,B11111111),_i2caddr_PF575);
	}
	else if( whichChannel> 12 && whichChannel<=24)
	{
		set_IO_PCF575_OUTPUT(word(B11011111,B11111111),_i2caddr_PF575);
	}
	else if( whichChannel> 24 && whichChannel<=36)
	{
		set_IO_PCF575_OUTPUT(word(B10111111,B11111111),_i2caddr_PF575);
	}
	else if( whichChannel> 36 && whichChannel<=48)
	{
		set_IO_PCF575_OUTPUT(word(B01111111,B11111111),_i2caddr_PF575);
	}
	Serial.println("On est dans le channel" + String(whichChannel));
}

void SwitchBox::printHex(uint16_t val)
{
	if (val < 0x1000) Serial.print('0');
	if (val < 0x100)  Serial.print('0');
	if (val < 0x10)   Serial.print('0');
	Serial.println(val, HEX);
}

void SwitchBox::readIncreaseBtn()
{
	//1 = OFF
	//0 = ON
	int reading = pcfBtn.read(0);
	
	if(previous_state_btn == true && reading == false)
	{
		status_btn_increase = true;
	}
	previous_state_btn = reading;
}

void SwitchBox::receiveWithEndMarker()
{
	static byte ndx = 0;
    char endMarker = '\r';
    char rc;
   
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }
    }
}

void SwitchBox::showNewData()
{
	int channel;
	bool status;
	char extracted_data [10];
	char bool_char[5];
	char ch_char[49];
	
	if(newData==true)
	{
		/*data = receivedChars;
		data.trim();
		
		Serial.println(data);*/
		if(strstr(receivedChars,"CH") != NULL && (strstr(receivedChars,"|")))
		{
			sscanf(receivedChars,"CH%d|%s",&channel,bool_char);
			
			if(strstr(bool_char,"ON"))
			{
				status = true;
			}
			else
			{
				status = false;
			}
			
			//Shift Out Relay
			shift_Out(channel,status);
			
			//Set Led 
			set_LED (myLedPins[channel],!status,channel);
		}
		
		//Do your thing
		else if(strstr(receivedChars,"LV") != NULL && (strstr(receivedChars,"|")))
		{
			if(strstr(receivedChars,"ON"))
			{
				status = true;
			}
			else
			{
				status = false;
			}
			set_MAIN_LV(status);
			
			Serial.println("STATUS LV : "+ String(status));
		}
		
		else if(strstr(receivedChars,"HV") != NULL && (strstr(receivedChars,"|")))
		{
			if(strstr(receivedChars,"ON"))
			{
				status = true;
			}
			else
			{
				status = false;
			}
			set_MAIN_HV(status);
			
			Serial.println("STATUS HV : "+ String(status));
		}
		
		else if(strcmp(receivedChars,"CH?") == 0)
		{
			//Assign Buffer
			for ( int i=0;i<48;i++)
			{
				sprintf(ch_char+i,"%d",ch_status[i]);
			}
			
			//Send to USB
			Serial.println("STATUS OF ALL CHANNEL : "+String(ch_char));
		}
		else if(strcmp(receivedChars,"LV?") == 0)
		{
			//Assign Buffer 
			sprintf(bool_char,"%d",lv_status);
			
			//Send to USB
			Serial.println("STATUS LV : "+ String(bool_char));
		}
		else if(strcmp(receivedChars,"HV?") == 0)
		{
			//Assign Buffer 
			sprintf(bool_char,"%d",hv_status);
			
			//Send to USB
			Serial.println("STATUS LV : "+ String(bool_char));
		}
		else if(strcmp(receivedChars,"MODE?") == 0)
		{
			//Assign Buffer 
			sprintf(bool_char,"%d",mode_status);
			
			//Send to USB
			if( mode_status == 1)
			{
				Serial.println("SB MODE : REMOTE");
			}
			else
			{
				Serial.println("SB MODE : LOCAL");
			}
		}
		else if(strcmp(receivedChars,"STATUS?") == 0)
		{
			uint64_t value = 0;
			
			//Translate & Count
			for (uint8_t i=0; i<(sizeof(ch_status)/sizeof(int));i++)
			{
				if (ch_status[i] == 1)
				{
					uint64_t temp_val = 1;
					
					int index = i;
					
					for(uint8_t i=0;i<index;i++)
					{
						temp_val = temp_val*2;
					}
					value +=temp_val;
				}
			}
			
			Serial.println(PriUint64<DEC>(value));
			
			root["channels"] = value;
			
			//Store JSON Data into Char array
			root.printTo(output_JSON_char);	
			
			//Send to USB
			Serial.println(output_JSON_char);
		}
		
		/*if(strcmp(receivedChars,"LV|ON") == 0)
		{
			Serial.println("On a recu LV|ON");
		}*/
	
		//Reset Variables
		newData = false;
	}
}
