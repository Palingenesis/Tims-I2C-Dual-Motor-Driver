
/*
	This is code to control the tracks of my robot TIM-02.

	By Tim Jackson.1960
		More info on Instructables. https://www.instructables.com/member/Palingenesis/instructables/

	This Sketch was written for the ESP8266 (new NodeMCU). V1.0

	Wire request buffer of the Motor Driver was originaly set/configured for 32 bit values but was reduced to 24+ bit numbers.
	The original code is commented out should I change microcontroller with more memory.
	I say 24+ bit number becouse I have used a seperate Byte for negative flags.
	This gives a number range of: -16,777,215 to 16,777,215.
	To make it more universal, all values are Ticks of the Motors Quadratic Encoder.

	Wire_Request converts as follows:

	Flags	(2 bytes Used to hold 16 single bit values like negative switches and flags)

	First Byte:
		Flags_1 true if:
			LSB		1	=	0x01 	CurPosition_A < 0
					2	=	0x02	CurPosition_B < 0
					4	=	0x04	Station_F_A < 0
					8	=	0x08	Station_F_B < 0
					16	=	0x10	Station_R_A < 0
					32	=	0x20	Station_R_B < 0
					64	=	0x40	Has_Station_A
			MSB		128	=	0x80	Has_Station_B

	Second Byte:
		Flags_2 true if:
			LSB		1	=	0x01	Debug Value 1 < 0
					2	=	0x02	Debug Value 2 < 0
					4	=	0x04	Is_400K (default = 100K)
					8	=	0x08
					16	=	0x10
					32	=	0x20
					64	=	0x40
			MSB		128	=	0x80

		Bytes 2 to 7	(3 bytes = 24 Bit):
			CurPossition_A	_Val_32 & 0xFF
			CurPossition_A	(_Val_32 >> 8) & 0xFF
			CurPossition_A	(_Val_32 >> 16) & 0xFF
			CurPossition_B	_Val_32 & 0xFF
			CurPossition_B	(_Val_32 >> 8) & 0xFF
			CurPossition_B	(_Val_32 >> 16) & 0xFF

		Byte 8	(1 byte = 8 Bit):
			I2C_Address

		Bytes 9 to 16	(2 bytes = 16 Bit):
			Station_F_A		_Val_32 & 0xFF
			Station_F_A		(_Val_32 >> 8) & 0xFF
			Station_F_B		_Val_32 & 0xFF
			Station_F_B		(_Val_32 >> 8) & 0xFF
			Station_R_A		_Val_32 & 0xFF
			Station_R_A		(_Val_32 >> 8) & 0xFF
			Station_R_B		_Val_32 & 0xFF
			Station_R_B		(_Val_32 >> 8) & 0xFF

		Bytes 17 to 20	(2 bytes = 16 Bit):
			Motor Load A	_Val_32 & 0xFF
			Motor Load A	(_Val_32 >> 8) & 0xFF
			Motor Load B	_Val_32 & 0xFF
			Motor Load B	(_Val_32 >> 8) & 0xFF


	The buffer holds 32 bytes, the rest are not used.
		Currently hold data version an maker.


	ESP8266 (2M SPIFFS)
		Sketch uses 271356 bytes (25%) of program storage space. Maximum is 1044464 bytes.
		Global variables use 28036 bytes (34%) of dynamic memory, leaving 53884 bytes for local variables. Maximum is 81920 bytes.

*/

/*	Only used by Tim	*/
//#include "Credentials.h"
//#include "Tims_Arduino_ESP8266.h"

/*	Debug	*/
//#define DEBUG

/*	Mode	*/
#define USING_SERVER
//#define USING_SERIAL


/*
  Type of connection?
  "Access Point", "Access Point with password" or "Station"

  "Access Point (AP) mode"
  This is not connected to your local network, you connect to the Network of the ESP8266.
  This means you can connect to it with your Moble Phone anywhare you are.
  You change the WiFi your Phone is connected to, to the ESP8266 WiFi.
  Then open browser to the IP of the ESP8266 control page. (we will be setting it to 192.168.50.11)
  Have your serial monitor connected when you re-set the module to confirm correct IP Address.

  "Access Point with Pasword (AP) mode"
  This is same as above, but you need a password to connect to the WiFi. (it currently is: 12345678)

  "Station (STA) mode"
  In STA mode, the ESP8266 connects to an existing WiFi network created by your wireless router.
  You will need to set the acces credentials for your local network.

  Un-Comment which way you want to connect.
*/

//#define ACCESS_POINT
//#define ACCESS_POINT_PW
#define ACCESS_POINT_STA

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "FS.h"
#include <Wire.h>

/*
  The name of network of the ESP8266
  The Password if used

  Note!
  If changing password, make sure it is at least 8 charectors long.
  If the password is too short, it will not be updated.
  Also:
	To change WiFi Setings, make sure that the "Erase Flash" setting is set correctly in "Tools" of the Arduino IDE.
	Only use "all" if you want to remove also files in SPIFFS.

	ESP8266_SSID_AP is required if using the "Access Point (AP) mode" or "Access Point with Pasword (AP) mode".
	ESP8266_PASSWORD_AP is required if using the "Access Point with Pasword (AP) mode".

*/
const char* ESP8266_SSID_AP = "Tims Track Control";
const char* ESP8266_PASSWORD_AP = "12345678";
/*
  The name of your local network.
  The Password.

	These are required if using the "Station (STA) mode".
*/
//const char* LOCAL_SSID_STA = "Your local network name";		/*	Your local network name.		*/
//const char* LOCAL_PASSWORD_STA = "Your local network password";	/*	Your local network password.	*/
/*
  Server
*/
IPAddress local_ip(192, 168, 50, 11);   /*  IP for AP mode    */
IPAddress gateway(192, 168, 100, 1);    /*  IP for AP mode    */
IPAddress subnet(255, 255, 255, 0);     /*  IP for AP mode    */
ESP8266WebServer server(80);

#define WEB_PAGE		"/index.html"
#define TRACKS_PNG		"/Tracks.png"
#define BF1_SVG			"/BF1.svg"
#define BF2_SVG			"/BF2.svg"
#define BR1_SVG			"/BR1.svg"
#define BR2_SVG			"/BR2.svg"
#define BRL_SVG			"/BRL.svg"
#define BRR_SVG			"/BRR.svg"
#define BS_SVG			"/BS.svg"


//#define TARGET_I2C_ADDRESS	0x30    /*  0x30=48 0x60=96 0x32=50Target Address.	*/
int Target_I2C_Address = 0x30;

#define BUFF_64				64      /*  What is the longest message Arduino can store?	128 max for ESP8266	*/
#define BUFF_32				32

#define SDA_PIN 4
#define SCL_PIN 5

/*	SERIAL MESSAGES	*/
#define I2C_MESSAGE_DELAY	50
char Buffer_TX[BUFF_64];		/* message				*/
uint8_t Buffer_RX[BUFF_32];		/* message				*/
unsigned int no_data = 0;
unsigned int ByteCount;				/* size of Buffer_TX	*/
bool isComment = false;
bool procsessingString = false;
short cmd = -1;
bool slaveProcesing = false;
bool ProcsessingCommand = false;

/*	I2C Motor Driver	*/
#define MOTOR_DRIVER_ADD 0x30
char* Speed_Array = "0";
volatile byte Speed_Array_Length=1;
volatile byte i = 0;
volatile char Motor = '3';

/*	Debug	*/
long TimeNow = millis();
long Period = 1000;
long TimeOut = TimeNow + Period;

/*	====	HTML CLIENT INPUT	====	*/

/*
  Read motor command sent from client
*/
void Handle_Set_Drive_State() {

	Track_Control(server.arg("value"));

	Serial.print("Command: ");
	Serial.println(server.arg("value"));
	server.send(200, "text/plain", "OK");
	Flash_LED();

}
/*
	Movement

	Movement is controlled from two of my motor drivers.
		There are two drivers.

	The drivers take commands via the I2C.
		Arrdesses:
			Left:	0x30	(48)
			Right:	0x31	(49)

			F0		=	Stop
			R0		=	Stop
			F1		=	Forward
			R1		=	Reverse
			S800	=	speed 800
*/
void Track_Control(String command) {

	/*	Initialize Buffer_TX with NULL values (0)	*/
	memset(Buffer_TX, 0, sizeof(Buffer_TX));

	/*

	WEB
		"ST"	Both	Stop
		"LF"	Left	Forward
		"RF"	Right	Forward
		"BF"	Both	Forward
		"LR"	Left	Reverse
		"RR"	Right	Reverse
		"BR"	Both	Reverse
		"RoL"	Rotate	Right
		"RoR"	Rotate	Left

	*/

	/*	STOP	*/
	if (command == "ST") {

		Buffer_TX[0] = 'R';	/*	R	*/
		Buffer_TX[1] = '0';
		Buffer_TX[2] = ' ';
		Buffer_TX[3] = 'M';
		Buffer_TX[4] = '3';	/*	Motor Both	*/
		Buffer_TX[5] = '\r';
		Buffer_TX[6] = '\n';
		Buffer_TX[7] = 0;
		ByteCount = 8;

		SendBufferOnI2C(MOTOR_DRIVER_ADD);
	}

	/*	FORWARD	*/
	else if (command == "LF") {

		Buffer_TX[0] = 'F';	/*	F	*/
		Buffer_TX[1] = '1';
		Buffer_TX[2] = ' ';
		Buffer_TX[3] = 'S';

		for (i = 0; i < Speed_Array_Length; i++) {
			Buffer_TX[i + 4] = Speed_Array[i];
		}

		Buffer_TX[i + 4] = ' ';
		Buffer_TX[i + 5] = 'M';
		Buffer_TX[i + 6] = '2';	/*	Motor Left	*/
		Buffer_TX[i + 7] = '\r';
		Buffer_TX[i + 8] = '\n';
		Buffer_TX[i + 9] = 0;
		ByteCount = i + 10;

		SendBufferOnI2C(MOTOR_DRIVER_ADD);
	}
	else if (command == "RF") {

		Buffer_TX[0] = 'F';	/*	F	*/
		Buffer_TX[1] = '1';
		Buffer_TX[2] = ' ';
		Buffer_TX[3] = 'S';

		for (i = 0; i < Speed_Array_Length; i++) {
			Buffer_TX[i + 4] = Speed_Array[i];
		}

		Buffer_TX[i + 4] = ' ';
		Buffer_TX[i + 5] = 'M';
		Buffer_TX[i + 6] = '1';	/*	Motor Right	*/
		Buffer_TX[i + 7] = '\r';
		Buffer_TX[i + 8] = '\n';
		Buffer_TX[i + 9] = 0;
		ByteCount = i + 10;

		SendBufferOnI2C(MOTOR_DRIVER_ADD);
	}
	else if (command == "BF") {

		Buffer_TX[0] = 'F';	/*	F	*/
		Buffer_TX[1] = '1';
		Buffer_TX[2] = ' ';
		Buffer_TX[3] = 'S';

		for (i = 0; i < Speed_Array_Length; i++) {
			Buffer_TX[i + 4] = Speed_Array[i];
		}

		Buffer_TX[i + 4] = ' ';
		Buffer_TX[i + 5] = 'M';
		Buffer_TX[i + 6] = '3';	/*	Motor Both	*/
		Buffer_TX[i + 7] = '\r';
		Buffer_TX[i + 8] = '\n';
		Buffer_TX[i + 9] = 0;
		ByteCount = i + 10;

		SendBufferOnI2C(MOTOR_DRIVER_ADD);
	}

	/*	REVERSE	*/
	else if (command == "LR") {

		Buffer_TX[0] = 'R';	/*	R	*/
		Buffer_TX[1] = '1';
		Buffer_TX[2] = ' ';
		Buffer_TX[3] = 'S';

		for (i = 0; i < Speed_Array_Length; i++) {
			Buffer_TX[i + 4] = Speed_Array[i];
		}

		Buffer_TX[i + 4] = ' ';
		Buffer_TX[i + 5] = 'M';
		Buffer_TX[i + 6] = '2';	/*	Motor Left	*/
		Buffer_TX[i + 7] = '\r';
		Buffer_TX[i + 8] = '\n';
		Buffer_TX[i + 9] = 0;
		ByteCount = i + 10;

		SendBufferOnI2C(MOTOR_DRIVER_ADD);
	}
	else if (command == "RR") {

		Buffer_TX[0] = 'R';	/*	R	*/
		Buffer_TX[1] = '1';
		Buffer_TX[2] = ' ';
		Buffer_TX[3] = 'S';

		for (i = 0; i < Speed_Array_Length; i++) {
			Buffer_TX[i + 4] = Speed_Array[i];
		}

		Buffer_TX[i + 4] = ' ';
		Buffer_TX[i + 5] = 'M';
		Buffer_TX[i + 6] = '1';	/*	Motor Right	*/
		Buffer_TX[i + 7] = '\r';
		Buffer_TX[i + 8] = '\n';
		Buffer_TX[i + 9] = 0;
		ByteCount = i + 10;

		SendBufferOnI2C(MOTOR_DRIVER_ADD);
	}
	else if (command == "BR") {

		Buffer_TX[0] = 'R';	/*	R	*/
		Buffer_TX[1] = '1';
		Buffer_TX[2] = ' ';
		Buffer_TX[3] = 'S';

		for (i = 0; i < Speed_Array_Length; i++) {
			Buffer_TX[i + 4] = Speed_Array[i];
		}

		Buffer_TX[i + 4] = ' ';
		Buffer_TX[i + 5] = 'M';
		Buffer_TX[i + 6] = '3';	/*	Motor Both	*/
		Buffer_TX[i + 7] = '\r';
		Buffer_TX[i + 8] = '\n';
		Buffer_TX[i + 9] = 0;
		ByteCount = i + 10;

		SendBufferOnI2C(MOTOR_DRIVER_ADD);
	}
	/*	ROTATE	*/
	else if (command == "RoL") {

		Buffer_TX[0] = 'R';	/*	R	*/
		Buffer_TX[1] = '1';
		Buffer_TX[2] = ' ';
		Buffer_TX[3] = 'S';

		for (i = 0; i < Speed_Array_Length; i++) {
			Buffer_TX[i + 4] = Speed_Array[i];
		}

		Buffer_TX[i + 4] = ' ';
		Buffer_TX[i + 5] = 'M';
		Buffer_TX[i + 6] = '2';	/*	Motor Left	*/
		Buffer_TX[i + 7] = '\r';
		Buffer_TX[i + 8] = '\n';
		Buffer_TX[i + 9] = 0;
		ByteCount = i + 10;

		SendBufferOnI2C(MOTOR_DRIVER_ADD);

		Buffer_TX[0] = 'F';	/*	F	*/
		Buffer_TX[1] = '1';
		Buffer_TX[2] = ' ';
		Buffer_TX[3] = 'S';

		for (i = 0; i < Speed_Array_Length; i++) {
			Buffer_TX[i + 4] = Speed_Array[i];
		}

		Buffer_TX[i + 4] = ' ';
		Buffer_TX[i + 5] = 'M';
		Buffer_TX[i + 6] = '1';	/*	Motor Right	*/
		Buffer_TX[i + 7] = '\r';
		Buffer_TX[i + 8] = '\n';
		Buffer_TX[i + 9] = 0;
		ByteCount = i + 10;

		SendBufferOnI2C(MOTOR_DRIVER_ADD);

		Buffer_TX[0] = 'M';
		Buffer_TX[1] = '3';	/*	Motor Both, this is change to 3 for speed control	*/
		ByteCount = 2;
		SendBufferOnI2C(MOTOR_DRIVER_ADD);

	}
	else if (command == "RoR") {

		Buffer_TX[0] = 'F';	/*	F	*/
		Buffer_TX[1] = '1';
		Buffer_TX[2] = ' ';
		Buffer_TX[3] = 'S';

		for (i = 0; i < Speed_Array_Length; i++) {
			Buffer_TX[i + 4] = Speed_Array[i];
		}

		Buffer_TX[i + 4] = ' ';
		Buffer_TX[i + 5] = 'M';
		Buffer_TX[i + 6] = '2';	/*	Motor Left	*/
		Buffer_TX[i + 7] = '\r';
		Buffer_TX[i + 8] = '\n';
		Buffer_TX[i + 9] = 0;
		ByteCount = i + 10;

		SendBufferOnI2C(MOTOR_DRIVER_ADD);

		Buffer_TX[0] = 'R';	/*	R	*/
		Buffer_TX[1] = '1';
		Buffer_TX[2] = ' ';
		Buffer_TX[3] = 'S';

		for (i = 0; i < Speed_Array_Length; i++) {
			Buffer_TX[i + 4] = Speed_Array[i];
		}

		Buffer_TX[i + 4] = ' ';
		Buffer_TX[i + 5] = 'M';
		Buffer_TX[i + 6] = '1';	/*	Motor Right	*/
		Buffer_TX[i + 7] = '\r';
		Buffer_TX[i + 8] = '\n';
		Buffer_TX[i + 9] = 0;
		ByteCount = i + 10;

		SendBufferOnI2C(MOTOR_DRIVER_ADD);

		Buffer_TX[0] = 'M';
		Buffer_TX[1] = '3';	/*	Motor Both, this is change to 3 for speed control	*/
		ByteCount = 2;
		SendBufferOnI2C(MOTOR_DRIVER_ADD);

	}
	Motor = Buffer_TX[4];

	delay(I2C_MESSAGE_DELAY);
}
/*
  Read speed command sent from client
*/
void Handle_Set_Speed() {


	Speed_Array = new char[server.arg("value").length() + 1];
	Speed_Array_Length = server.arg("value").length();
	server.arg("value").toCharArray(Speed_Array, server.arg("value").length() + 1);

	Buffer_TX[0] = 'S';
	byte i = 0;
	for (i = 0; i < Speed_Array_Length; i++) {
		Buffer_TX[i + 1] = Speed_Array[i];
	}
	Buffer_TX[i + 1] = ' ';
	Buffer_TX[i + 2] = 'M';
	Buffer_TX[i + 3] = Motor;
	Buffer_TX[i + 4] = '\r';
	Buffer_TX[i + 5] = '\n';
	Buffer_TX[i + 6] = 0;
	ByteCount = i + 8;

	SendBufferOnI2C(MOTOR_DRIVER_ADD);
	delay(I2C_MESSAGE_DELAY);

	Serial.print("Command: ");
	Serial.println(server.arg("value"));
	server.send(200, "text/plain", "OK");
	Flash_LED();

}
/*
  HTTP request: 404
*/
void Handle_Not_Found() {
	Serial.println("Error 404 not found");
	server.send(404, "text/plain", "Not found");
}

/*	====	SERIAL INPUT	====	*/

/*
	Read Data in Serial Buffer if available.
*/
void ReadSerial() {
	/*
		Holder
	*/
	char c;
	/*
		Read in characters if available.
	*/
	if (Serial.available() > 0) {
		c = Serial.read();
		no_data = 0;
		/*
			Chech for newlines.
		*/
		if (c != '\r' || c != '\n') {
			/*
				Check for comments.
					These are used in G-Code.
			*/
			if (c == '(' || c == ';') {
				isComment = true;
			}
			/*
				If we're not in "isComment" mode, add it to our array.
			*/
			if (!isComment) {
				Buffer_TX[ByteCount++] = c;
			}
			/*
				End of isComment - start listening again.
			*/
			if (c == ')') {
				isComment = false;
			}
		}
	}
	else {
		no_data++;
		delayMicroseconds(100); // 100
		/*
			Process any code that has been recived
		*/
		if (ByteCount && (c == '\n' || c == '\r' || no_data > 100)) {
			no_data = 0;
			ByteCount--;

#ifdef DEBUG
			Serial.println(Buffer_TX);
			delay(50);
#endif    /*  DEBUG	*/

			procsessingString = true;
			processCommand();
			init_process_string();
		}
	}

}
/*
	Set ready for next command.
*/
void init_process_string() {

	for (byte i = 0; i < BUFF_64; i++) { Buffer_TX[i] = 0; }
	ByteCount = 0;
	procsessingString = false;
	isComment = false;
	cmd = -1;
	Wire.flush();
	Serial.flush();
	Serial.println("ok");
	delay(4);
}
/*
	Change command numbers to a long.
*/
long Parse_Number(char code, long val) {
	/*
		start at the beginning of Buffer_TX.
	*/
	char* ptr = Buffer_TX;
	/*
	  Go char to char through string.
	*/
	while ((long)ptr > 1 && (*ptr) && (long)ptr < (long)Buffer_TX + ByteCount) {
		/*
			if you find code as you go through string.
		*/
		if (*ptr == code) {
			/*
				convert the digits that follow into a long and return it.
			*/
			return atol(ptr + 1);
		}
		/*
			take a step from here to the next char after the next space.
		*/
		ptr = strchr(ptr, ' ') + 1;
	}
	/*
		If the end is reached and nothing found, return val.
	*/
	return val;
}
/*
	Process Commands.
*/
void processCommand() {

	uint32_t _Val = 0;
	uint8_t _Flag1 = 0;
	uint8_t _Flag2 = 0;

	/*	Get I2C Address if given. (defalt 0x30 or last sent)	*/
	Target_I2C_Address = Parse_Number('D', Target_I2C_Address);

	/*	get start of Buffer_TX (see what the command starts with)	*/
	char* ptr = Buffer_TX; // 

	/*
		======================
		Chose GET, Process or SEND
		======================
	*/
	if (*ptr == 'X') {
		/*	GET	*/
		int _numberOfBytes = Parse_Number('X', -1);
		if (_numberOfBytes == 0) {
			_numberOfBytes = BUFF_32;
		}

		/*	Clear Buffer	*/
		for (size_t i = 0; i < BUFF_32; i++) {
			Buffer_RX[i] = 0;
		}

		Wire.requestFrom(Target_I2C_Address, _numberOfBytes);
		delayMicroseconds(5);

		if (Wire.available()) {
			/*	Read Buffer	*/
			for (size_t i = 0; i < _numberOfBytes; i++) {
				uint8_t C = Wire.read();
				Buffer_RX[i] = C;
				//Serial.print(C);
				//Serial.print(", ");
			}
			//Serial.println();
		}
		/*	Buffer in Binary	*/
		if (_numberOfBytes > 7) {
			for (size_t i = BUFF_32; i > 0; i--) {

				Serial.print("Buffer_RX[");
				Serial.print(i - 1);
				Serial.print("] \tBIN = ");
				Serial.println(Buffer_RX[i - 1], BIN);
			}
		}


		/*
			===============
			Use and Format Info
			===============
		*/

		/*	Slave_Buffer_Tx[7] = Flags	*/
		_Flag1 = Buffer_RX[0];
		if (Buffer_RX[6] == 0) {
			Serial.println();
			Serial.print("No Address ");
			Serial.print(Target_I2C_Address, DEC);
			Serial.print(" (0x");
			Serial.print(Target_I2C_Address, HEX);
			Serial.print(") found.");
			Serial.println();
		}
		else {
			Serial.println();
			Serial.println("Formated:");
			Serial.println();

			/*	Currrent Possition	*/
			//_Val = Buffer_RX[5];
			//_Val = _Val << 0x08;
			_Val |= Buffer_RX[4];
			_Val = _Val << 0x08;
			_Val |= Buffer_RX[3];
			_Val = _Val << 0x08;
			_Val |= Buffer_RX[2];
			Serial.print("Currrent Position\tDEC = ");
			_Flag1 = (Buffer_RX[0] & 1);
			if (_Flag1 == 1) { Serial.print("-"); }
			else { Serial.print(" "); }
			Serial.println(_Val, DEC);

			/*	Limit Switches	*/
			_Flag2 = (Buffer_RX[1] & 1);
			if (_Flag2 == 1) { Serial.println("MIN Stop Hit"); }
			else { Serial.println("MIN OK"); }
			_Flag2 = (Buffer_RX[1] & 2);
			if (_Flag2 == 2) { Serial.println("MAX Stop Hit"); }
			else { Serial.println("MAX OK"); }

			if (_numberOfBytes > 7) {

				/*	My_I2C_Address		*/
				Serial.print("I2C Address\t\t\tDEC =  ");
				Serial.print(Buffer_RX[6], DEC);
				Serial.print("(HEX = 0x");
				Serial.print(Buffer_RX[6], HEX);
				Serial.println(")");

				/*  I2C Speed  */
				Serial.print("I2C Speed\t\t\tDEC =  ");
				_Flag2 = (Buffer_RX[1] & 4);
				if (_Flag2 == 4) { Serial.println("400000"); }
				else { Serial.println("100000"); }


				/*	Motor_Slowdown		*/
				_Val = Buffer_RX[8];
				_Val = _Val << 0x08;
				_Val |= Buffer_RX[7];
				Serial.print("Motor Slowdown\t\tDEC =  ");
				Serial.println(_Val);

				/*	MaxPossition		*/
				//_Val = Buffer_RX[12];
				//_Val = _Val << 0x08;
				_Val = Buffer_RX[11];
				_Val = _Val << 0x08;
				_Val |= Buffer_RX[10];
				_Val = _Val << 0x08;
				_Val |= Buffer_RX[9];
				Serial.print("Max Possition    \tDEC = ");
				_Flag1 = (Buffer_RX[0] & 2);
				if (_Flag1 == 2) { Serial.print("-"); }
				else { Serial.print(" "); }
				Serial.println(_Val, DEC);

				/*	MinPossition		*/
				//_Val = Buffer_RX[16];
				//_Val = _Val << 0x08;
				_Val = Buffer_RX[15];
				_Val = _Val << 0x08;
				_Val |= Buffer_RX[14];
				_Val = _Val << 0x08;
				_Val |= Buffer_RX[13];
				Serial.print("Min Possition    \tDEC = ");
				_Flag1 = (Buffer_RX[0] & 4);
				if (_Flag1 == 4) { Serial.print("-"); }
				else { Serial.print(" "); }
				Serial.println(_Val, DEC);

				/*	Station F		*/
				//_Val = Buffer_RX[20];
				//_Val = _Val << 0x08;
				//_Val |= Buffer_RX[19];
				//_Val = _Val << 0x08;
				_Val = Buffer_RX[18];
				_Val = _Val << 0x08;
				_Val |= Buffer_RX[17];
				Serial.print("Station Forward\t\tDEC = ");
				_Flag1 = (Buffer_RX[0] & 8);
				if (_Flag1 == 8) { Serial.print("-"); }
				else { Serial.print(" "); }
				Serial.println(_Val, DEC);

				/*	Station R		*/
				//_Val = Buffer_RX[24];
				//_Val = _Val << 0x08;
				//_Val |= Buffer_RX[23];
				//_Val = _Val << 0x08;
				_Val = Buffer_RX[22];
				_Val = _Val << 0x08;
				_Val |= Buffer_RX[21];
				Serial.print("Station Reverse\t\tDEC = ");
				_Flag1 = (Buffer_RX[0] & 16);
				if (_Flag1 == 16) { Serial.print("-"); }
				else { Serial.print(" "); }
				Serial.println(_Val, DEC);

				/*	HasFlags		*/
				_Flag1 = (Buffer_RX[0] & 32);
				if (_Flag1 == 32) { Serial.println("\tHasMAX       = true"); }
				else { Serial.println("\tHasMAX       = false"); }
				_Flag1 = (Buffer_RX[0] & 64);
				if (_Flag1 == 64) { Serial.println("\tHasMIN       = true"); }
				else { Serial.println("\tHasMIN       = false"); }
				_Flag1 = (Buffer_RX[0] & 128);
				if (_Flag1 == 128) { Serial.println("\tHasStation   = true"); }
				else { Serial.println("\tHasStation   = false"); }

				/*	Motor Speed Reduction	*/
				Serial.print("Motor Speed Reduction = ");
				Serial.print(Buffer_RX[25]);
				Serial.println("%");

				/*	Tim			*/
				for (size_t i = 29; i < 32; i++) {
					Serial.print((char)Buffer_RX[i]);
				}
				Serial.println();

			}

			delayMicroseconds(5);
		}
	}
	else if (*ptr == 'P') {

	}
	else {
		/*	SEND	*/
		ProcsessingCommand = true;
		Wire.flush();
		SendBufferOnI2C(Target_I2C_Address);
		delay(4);
		ProcsessingCommand = false;

	}
}
/*
	Send Recived Commands on to a Device on the I2C bus.
*/
void SendBufferOnI2C(int I2C_address) {

	Wire.beginTransmission(I2C_address);	/*	Connect with device at I2C_address.	*/
	Wire.write('#');						/*	Send garbage for the first byte of data that will be ignored.	*/
	Wire.write(Buffer_TX, ByteCount);			/*	Send Buffer_TX.						*/
	Wire.endTransmission();					/*	Disconnect.							*/

#ifdef DEBUG
	Serial.print("I2C_address 0x");
	Serial.print(I2C_address, HEX);
	Serial.print(" (");
	Serial.print(I2C_address);
	Serial.print(")");
	Serial.print("\t Buffer_TX ");
	Serial.print(Buffer_TX);
	Serial.print("\t ByteCount ");
	Serial.println(ByteCount);
	delay(50);
#endif  /*  DEBUG	*/

}
/*
	I2C Request Data
*/
void Wire_Request() {

}

/*	Debug	*/
void Flash_LED() {

	digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
	delay(100);
	digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
	delay(100);
	digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

/*	===	SETUP	===	*/
void setup() {

	/*
	  Start serial.
	*/
	Serial.begin(115200);
	delay(1000);	/*	Wait for Serial to begin	*/

	/*
	  Start Wire.
		  Used only as Master atm. (address optional for master)
			Wire.begin(SDA Pin, SCL Pin, Master Address);

		  Wire.setClockStretchLimit(4000L);
			An amount of time in milliseconds that as slave can hold the line while it gets the requested data.

	*/
	Wire.begin(SDA_PIN, SCL_PIN);
	Wire.setClock(400000);
	Wire.onRequest(Wire_Request);


	/*	Debug	*/
	pinMode(LED_BUILTIN, OUTPUT);

	/*
		Start SPI Flash Filing System.
	*/
	if (SPIFFS.begin()) {
		Serial.println("SPIFFS Active");
	}
	else {
		Serial.println("Unable to activate SPIFFS");
	}
	delay(500);

	/*
		===	Wi-Fi	===
	*/

	/*	Connect to ESP8266 Wi-Fi network with no password.	*/
#ifdef ACCESS_POINT
	WiFi.mode(WIFI_AP);
	Serial.println("ESP8266 in AP mode");
	WiFi.softAPConfig(local_ip, gateway, subnet);
	Serial.println("Setting AP (Access Point)");
	WiFi.softAP(ESP8266_SSID_AP);
	Serial.print("Connect you device to network '");
	Serial.print(ESP8266_SSID_AP);
	Serial.println("'\r\nThere is no Password.");
	Serial.print("Open browser to ");
	Serial.println(WiFi.softAPIP());

#endif // ACCESS_POINT

	/*	Connect to ESP8266 Wi-Fi network with a password.	*/
#ifdef ACCESS_POINT_PW
	WiFi.mode(WIFI_AP);
	Serial.println("ESP8266 in AP mode with password");
	WiFi.softAPConfig(local_ip, gateway, subnet);
	Serial.println("Setting AP (Access Point)");
	WiFi.softAP(ESP8266_SSID_AP, ESP8266_PASSWORD_AP);
	Serial.print("Connect you device to network '");
	Serial.print(ESP8266_SSID_AP);
	Serial.println("'");
	Serial.print("Use password '");
	Serial.print(ESP8266_PASSWORD_AP);
	Serial.println("'");
	Serial.print("Open browser to ");
	Serial.println(WiFi.softAPIP());

#endif // ACCESS_POINT_PW

	/*	Connect to your local Wi-Fi network with a password.	*/
#ifdef ACCESS_POINT_STA
	WiFi.mode(WIFI_STA);
	Serial.println("ESP8266 in STA mode");
	WiFi.begin(LOCAL_SSID_STA, LOCAL_PASSWORD_STA);
	Serial.print("Connecting ");
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
#endif

	/*
	  setup web server to handle specific HTTP requests.

	  A connect or refresh from the client, just sends "/"
	  If the client is requesting somethin spesific: "/Something" = what is sent from the client. (Client Rquest)

		To run a sub-routeen:
		  server.on("/Something", HTTP_GET, Handle Somthing);

		To return a file saved on the SPIFFS:
		  server.serveStatic("/Something", SPIFFS, path, optional file type);
	*/
	server.serveStatic("/", SPIFFS, WEB_PAGE, "text/html");
	server.serveStatic("/Tracks.png", SPIFFS, TRACKS_PNG, "image / png");
	server.serveStatic("/BF1.svg", SPIFFS, BF1_SVG, "image/svg");
	server.serveStatic("/BF2.svg", SPIFFS, BF2_SVG, "image/svg");
	server.serveStatic("/BR1.svg", SPIFFS, BR1_SVG, "image/svg");
	server.serveStatic("/BR2.svg", SPIFFS, BR2_SVG, "image/svg");
	server.serveStatic("/BRL.svg", SPIFFS, BRL_SVG, "image/svg");
	server.serveStatic("/BRR.svg", SPIFFS, BRR_SVG, "image/svg");
	server.serveStatic("/BS.svg", SPIFFS, BS_SVG, "image/svg");


	server.on("/Set_Speed", HTTP_GET, Handle_Set_Speed);
	server.on("/Set_Drive_State", HTTP_GET, Handle_Set_Drive_State);

	server.onNotFound(Handle_Not_Found);

	/*
	  start server.
	*/
#ifdef USING_SERVER

	server.begin();
	Serial.println("\nTim's\nTrack Control\nserver started.\n");

#endif // USING_SERVER

	digitalWrite(LED_BUILTIN, LOW);

}
/*	===	LOOP	===	*/
void loop() {

#ifdef USING_SERVER

	/*
		Handle HTTP requests.

			This should be the only thing here.
			All actions should be run through commands to the server,
			by requests from the client (Web Page).
	*/
	server.handleClient();

#else

	/*
		Tick - Tock
	*/
	TimeNow = millis();
	/*
		Get next command when finish current command.
	*/
	if (!procsessingString) { ReadSerial(); }

	if (TimeNow > TimeOut) {
		TimeOut = TimeNow + Period;
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
	}

#endif // USING_SERVER

}
