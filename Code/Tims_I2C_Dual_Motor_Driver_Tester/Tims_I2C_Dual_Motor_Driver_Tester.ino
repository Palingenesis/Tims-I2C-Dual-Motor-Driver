
/*
	A small program to send commands via Serial to Tim's I2C Dual Motor Driver.

	By Tim Jackson.1960
		More info on Instructables. https://www.instructables.com/member/Palingenesis/instructables/


	ESP8266 (2M SPIFFS)
		Variables and constants in RAM (global, static), used 29504 / 80192 bytes (36%)
			SEGMENT  BYTES    DESCRIPTION
			DATA     1516     initialized variables
			RODATA   1364     constants
			BSS      26624    zeroed variables
		Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR), used 62223 / 65536 bytes (94%)
			SEGMENT  BYTES    DESCRIPTION
			ICACHE   32768    reserved space for flash instruction cache
			IRAM     29455    code in IRAM
		Code in flash (default, ICACHE_FLASH_ATTR), used 243028 / 1048576 bytes (23%)
			SEGMENT  BYTES    DESCRIPTION
			IROM     243028   code in flash

	Arduino NANO ATMega328p
		Sketch uses 8628 bytes (28%) of program storage space. Maximum is 30720 bytes.
		Global variables use 968 bytes (47%) of dynamic memory, leaving 1080 bytes for local variables. Maximum is 2048 bytes.


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

Below is just for me to use as I use Visual Studio 2022 Comunity to edit code.
#include <Tims_Arduino_NANO.h>
*/

/*	Debug	*/
#define DEBUG


/*	Arduino IDE	*/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/*	LCD	*/
LiquidCrystal_I2C lcd(0x27, 16, 2);

//#define TARGET_I2C_ADDRESS	0x30    /*  0x30=48 0x60=96 0x32=50Target Address.	*/
int Target_I2C_Address = 0x30;

#define BUFF_64				64      /*  What is the longest message Arduino can store?	*/
#define BUFF_16				16
#define BUFF_24				24
#define BUFF_32				32

#define SDA_PIN 4
#define SCL_PIN 5
const int16_t I2C_MASTER = 0x42;
const int16_t I2C_SLAVE = 0x08;

/*	SERIAL MESSAGES	*/
char Buffer_TX[BUFF_64];		/* message				*/
uint8_t Buffer_RX[BUFF_32];		/* message				*/
unsigned int no_data = 0;
unsigned int sofar;				/* size of Buffer_TX	*/
bool isComment = false;
bool procsessingString = false;
short cmd = -1;
bool slaveProcesing = false;
bool ProcsessingCommand = false;

/*	Debug	*/
long TimeNow = millis();
long Period = 1000;
long TimeOut = TimeNow + Period;

void setup() {
	/*
	  Start serial.
	*/
	Serial.begin(115200);
	/*
	  Start Wire.
		  Used only as Master atm. (address optional for master)
		  Wire.begin(SDA Pin, SCL Pin, Master Address);
	*/
	Wire.begin();
	Wire.setClock(400000);
	Wire.onRequest(Wire_Request);

	/*	LCD	*/
	lcd.init();
	lcd.backlight();
	lcd.print("   Tim's Dual   ");
	lcd.setCursor(0, 1);//bottom line
	lcd.print("  Motor Driver  ");



	/*	Debug	*/
	pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {
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

}
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
				Buffer_TX[sofar++] = c;
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
		if (sofar && (c == '\n' || c == '\r' || no_data > 100)) {
			no_data = 0;
			sofar--;

#ifdef DEBUG
			Serial.println(Buffer_TX);
			delay(50);
#endif  //  DEBUG

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
	sofar = 0;
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
	while ((long)ptr > 1 && (*ptr) && (long)ptr < (long)Buffer_TX + sofar) {
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
		/* Buffer in Binary */
		if (_numberOfBytes > 7) {
			for (size_t i = BUFF_32; i > 0; i--) {
				Serial.print("Buffer_RX[");
				Serial.print(i - 1);
				Serial.print("] \tBIN = ");

				// Convert byte to binary string
				String binaryString = String(Buffer_RX[i - 1], BIN);

				// Calculate the number of leading zeros needed
				int leadingZeros = 8 - binaryString.length();

				// Print the leading zeros
				for (int j = 0; j < leadingZeros; j++) {
					Serial.print("0");
				}

				// Print the binary string
				Serial.println(binaryString);
			}
		}


		/*
			===============
			Use and Format Info
			===============
		*/


		/*	I2C Address Check	*/
		if (Buffer_RX[8] == 0) {
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


			/*	Currrent Position A	*/
			_Val = Buffer_RX[2];
			_Val |= (uint32_t)Buffer_RX[3] << 8;
			_Val |= (uint32_t)Buffer_RX[4] << 16;
			Serial.print("Current Position A\tDEC = ");
			if ((Buffer_RX[0] & 0x01) == 0x01) { Serial.print("-"); }
			Serial.println(_Val, DEC);

			/*	Currrent Position B	*/
			_Val = Buffer_RX[5];
			_Val |= (uint32_t)Buffer_RX[6] << 8;
			_Val |= (uint32_t)Buffer_RX[7] << 16;
			Serial.print("Current Position B\tDEC = ");
			if ((Buffer_RX[0] & 0x02) == 0x02) { Serial.print("-"); }
			Serial.println(_Val, DEC);




			if (_numberOfBytes > 7) {

				/*	My_I2C_Address		*/
				Serial.print("I2C Address\t\t\tDEC =  ");
				Serial.print(Buffer_RX[8], DEC);
				Serial.print("(HEX = 0x");
				Serial.print(Buffer_RX[8], HEX);
				Serial.println(")");

				/*  I2C Speed  */
				Serial.print("I2C Speed\t\t\tDEC =  ");
				if ((Buffer_RX[1] & 4) == 4) { Serial.println("400000"); }
				else { Serial.println("100000"); }

				/*  Station A ON/OFF  */
				Serial.print("Station A =  ");
				if ((Buffer_RX[0] & 64) == 64) { Serial.println("ON"); }
				else { Serial.println("OFF"); }

				/*	Station F A		*/
				_Val = Buffer_RX[10];
				_Val = _Val << 0x08;
				_Val |= Buffer_RX[9];
				Serial.print("Station Forward A\t\tDEC = ");
				if ((Buffer_RX[0] & 4) == 4) { Serial.print("-"); }
				else { Serial.print(" "); }
				Serial.println(_Val, DEC);

				/*	Station R A		*/
				_Val = Buffer_RX[14];
				_Val = _Val << 0x08;
				_Val |= Buffer_RX[13];
				Serial.print("Station Reverse A\t\tDEC = ");
				if ((Buffer_RX[0] & 16) == 16) { Serial.print("-"); }
				else { Serial.print(" "); }
				Serial.println(_Val, DEC);

				/*  Station B ON/OFF  */
				Serial.print("Station B =  ");
				if ((Buffer_RX[0] & 128) == 128) { Serial.println("ON"); }
				else { Serial.println("OFF"); }

				/*	Station F B		*/
				_Val = Buffer_RX[12];
				_Val = _Val << 0x08;
				_Val |= Buffer_RX[11];
				Serial.print("Station Forward B\t\tDEC = ");
				if ((Buffer_RX[0] & 8) == 8) { Serial.print("-"); }
				else { Serial.print(" "); }
				Serial.println(_Val, DEC);

				/*	Station R B		*/
				_Val = Buffer_RX[16];
				_Val = _Val << 0x08;
				_Val |= Buffer_RX[15];
				Serial.print("Station Reverse B\t\tDEC = ");
				if ((Buffer_RX[0] & 32) == 32) { Serial.print("-"); }
				else { Serial.print(" "); }
				Serial.println(_Val, DEC);



				/*	Motor Load A	*/
				_Val = (uint16_t)(Buffer_RX[18] << 8) | Buffer_RX[17];
				Serial.print("Motor Load A: ");
				Serial.println(_Val);


				/*	Motor Load B	*/
				_Val = (uint16_t)(Buffer_RX[20] << 8) | Buffer_RX[19];
				Serial.print("Motor Load B: ");
				Serial.println(_Val);








				/*	Tim			*/
				Serial.print("Tim");
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

	Wire.beginTransmission(I2C_address);	//	Get device at I2C_address attention
	Wire.write('#');						/*	Send garbage for the first byte of data that will be ignored.	*/
	Wire.write(Buffer_TX, sofar);			//	Send Buffer_TX
	Wire.endTransmission();					//	Stop transmitting

#ifdef DEBUG
	Serial.print("Buffer_TX ");
	Serial.print(Buffer_TX);
	Serial.print("\tsofar ");
	Serial.println(sofar);
	delay(50);
#endif  //  DEBUG

}
/*
	I2C Request Data
*/
void Wire_Request() {

}
