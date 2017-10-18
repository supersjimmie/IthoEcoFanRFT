/*
 * Author: Klusjesman, modified bij supersjimmie for Arduino/ESP8266
 */

#include "IthoCC1101.h"
#include <string.h>
#include <Arduino.h>
#include <SPI.h>

// default constructor
IthoCC1101::IthoCC1101(uint8_t counter, uint8_t sendTries) : CC1101()
{
	this->outIthoPacket.counter = counter;
	this->outIthoPacket.previous = IthoLow;
	this->sendTries = sendTries;
	this->receiveState = ExpectMessageStart;
	
	//fixed device id
	this->outIthoPacket.deviceId[0] = 101;
	this->outIthoPacket.deviceId[1] = 89;
	this->outIthoPacket.deviceId[2] = 154;
	this->outIthoPacket.deviceId[3] = 153;
	this->outIthoPacket.deviceId[4] = 170;
	this->outIthoPacket.deviceId[5] = 105;
	this->outIthoPacket.deviceId[6] = 154;
	this->outIthoPacket.deviceId[7] = 86;
} //IthoCC1101

// default destructor
IthoCC1101::~IthoCC1101()
{
} //~IthoCC1101

void IthoCC1101::initSendMessage1()
{
	/*
	Configuration reverse engineered from remote print. The commands below are used by IthoDaalderop.
		
	Base frequency		868.299866MHz
	Channel				0
	Channel spacing		199.951172kHz
	Carrier frequency	868.299866MHz
	Xtal frequency		26.000000MHz
	Data rate			8.00896kBaud
	Manchester			disabled
	Modulation			2-FSK
	Deviation			25.390625kHz
	TX power			?
	PA ramping			enabled
	Whitening			disabled
	*/
	writeCommand(CC1101_SRES);
	delayMicroseconds(1);
	writeRegister(CC1101_IOCFG0 ,0x2E);		//High impedance (3-state)
	writeRegister(CC1101_FREQ2 ,0x21);		//00100001	878MHz-927.8MHz
	writeRegister(CC1101_FREQ1 ,0x65);		//01100101
	writeRegister(CC1101_FREQ0 ,0x6A);		//01101010
	writeRegister(CC1101_MDMCFG4 ,0x07);	//00000111
	writeRegister(CC1101_MDMCFG3 ,0x43);	//01000011
	writeRegister(CC1101_MDMCFG2 ,0x00);	//00000000	2-FSK, no manchester encoding/decoding, no preamble/sync
	writeRegister(CC1101_MDMCFG1 ,0x22);	//00100010
	writeRegister(CC1101_MDMCFG0 ,0xF8);	//11111000
	writeRegister(CC1101_CHANNR ,0x00);		//00000000
	writeRegister(CC1101_DEVIATN ,0x40);	//01000000
	writeRegister(CC1101_FREND0 ,0x17);		//00010111	use index 7 in PA table
	writeRegister(CC1101_MCSM0 ,0x18);		//00011000	PO timeout Approx. 146탎 - 171탎, Auto calibrate When going from IDLE to RX or TX (or FSTXON)
	writeRegister(CC1101_FSCAL3 ,0xA9);		//10101001
	writeRegister(CC1101_FSCAL2 ,0x2A);		//00101010
	writeRegister(CC1101_FSCAL1 ,0x00);		//00000000
	writeRegister(CC1101_FSCAL0 ,0x11);		//00010001
	writeRegister(CC1101_FSTEST ,0x59);		//01011001	For test only. Do not write to this register.
	writeRegister(CC1101_TEST2 ,0x81);		//10000001	For test only. Do not write to this register.
	writeRegister(CC1101_TEST1 ,0x35);		//00110101	For test only. Do not write to this register.
	writeRegister(CC1101_TEST0 ,0x0B);		//00001011	For test only. Do not write to this register.
	writeRegister(CC1101_PKTCTRL0 ,0x12);	//00010010	Enable infinite length packets, CRC disabled, Turn data whitening off, Serial Synchronous mode
	writeRegister(CC1101_ADDR ,0x00);		//00000000
	writeRegister(CC1101_PKTLEN ,0xFF);		//11111111	//Not used, no hardware packet handling

	//0x6F,0x26,0x2E,0x8C,0x87,0xCD,0xC7,0xC0
	writeBurstRegister(CC1101_PATABLE | CC1101_WRITE_BURST, (uint8_t*)ithoPaTableSend, 8);

	writeCommand(CC1101_SIDLE);
	writeCommand(CC1101_SIDLE);
	writeCommand(CC1101_SIDLE);

	writeRegister(CC1101_MDMCFG4 ,0x08);	//00001000
	writeRegister(CC1101_MDMCFG3 ,0x43);	//01000011
	writeRegister(CC1101_DEVIATN ,0x40);	//01000000
	writeRegister(CC1101_IOCFG0 ,0x2D);		//GDO0_Z_EN_N. When this output is 0, GDO0 is configured as input (for serial TX data).
	writeRegister(CC1101_IOCFG1 ,0x0B);		//Serial Clock. Synchronous to the data in synchronous serial mode.
	
	writeCommand(CC1101_STX);
	writeCommand(CC1101_SIDLE);
	delayMicroseconds(1);
	writeCommand(CC1101_SIDLE);

	writeRegister(CC1101_MDMCFG4 ,0x08);	//00001000
	writeRegister(CC1101_MDMCFG3 ,0x43);	//01000011
	writeRegister(CC1101_DEVIATN ,0x40);	//01000000
	//writeRegister(CC1101_IOCFG0 ,0x2D);		//GDO0_Z_EN_N. When this output is 0, GDO0 is configured as input (for serial TX data).
	//writeRegister(CC1101_IOCFG1 ,0x0B);		//Serial Clock. Synchronous to the data in synchronous serial mode.
	
	//Itho is using serial mode for transmit. We want to use the TX FIFO with fixed packet length for simplicity.
	writeRegister(CC1101_IOCFG0 ,0x2E);
	writeRegister(CC1101_IOCFG1 ,0x2E);	
	writeRegister(CC1101_PKTLEN , 19);
	writeRegister(CC1101_PKTCTRL0 ,0x00);
	writeRegister(CC1101_PKTCTRL1 ,0x00);	
}

void IthoCC1101::initSendMessage2(IthoCommand command)
{
	//finishTransfer();
	writeCommand(CC1101_SIDLE);
	delayMicroseconds(1);
	writeRegister(CC1101_IOCFG0 ,0x2E);
	delayMicroseconds(1);
	writeRegister(CC1101_IOCFG1 ,0x2E);
	delayMicroseconds(1);
	writeCommand(CC1101_SIDLE);
	writeCommand(CC1101_SPWD);
	delayMicroseconds(1);
	
	/*
	Configuration reverse engineered from remote print. The commands below are used by IthoDaalderop.
		
	Base frequency		868.299866MHz
	Channel				0
	Channel spacing		199.951172kHz
	Carrier frequency	868.299866MHz
	Xtal frequency		26.000000MHz
	Data rate			38.3835kBaud
	Manchester			disabled
	Modulation			2-FSK
	Deviation			50.781250kHz
	TX power			?
	PA ramping			enabled
	Whitening			disabled
	*/	
	writeCommand(CC1101_SRES);
	delayMicroseconds(1);
	writeRegister(CC1101_IOCFG0 ,0x2E);		//High impedance (3-state)
	writeRegister(CC1101_FREQ2 ,0x21);		//00100001	878MHz-927.8MHz
	writeRegister(CC1101_FREQ1 ,0x65);		//01100101
	writeRegister(CC1101_FREQ0 ,0x6A);		//01101010	
	writeRegister(CC1101_MDMCFG4 ,0x5A);	//difference compared to message1
	writeRegister(CC1101_MDMCFG3 ,0x83);	//difference compared to message1
	writeRegister(CC1101_MDMCFG2 ,0x00);	//00000000	2-FSK, no manchester encoding/decoding, no preamble/sync
	writeRegister(CC1101_MDMCFG1 ,0x22);	//00100010
	writeRegister(CC1101_MDMCFG0 ,0xF8);	//11111000
	writeRegister(CC1101_CHANNR ,0x00);		//00000000
	writeRegister(CC1101_DEVIATN ,0x50);	//difference compared to message1
	writeRegister(CC1101_FREND0 ,0x17);		//00010111	use index 7 in PA table
	writeRegister(CC1101_MCSM0 ,0x18);		//00011000	PO timeout Approx. 146탎 - 171탎, Auto calibrate When going from IDLE to RX or TX (or FSTXON)
	writeRegister(CC1101_FSCAL3 ,0xA9);		//10101001
	writeRegister(CC1101_FSCAL2 ,0x2A);		//00101010
	writeRegister(CC1101_FSCAL1 ,0x00);		//00000000
	writeRegister(CC1101_FSCAL0 ,0x11);		//00010001
	writeRegister(CC1101_FSTEST ,0x59);		//01011001	For test only. Do not write to this register.
	writeRegister(CC1101_TEST2 ,0x81);		//10000001	For test only. Do not write to this register.
	writeRegister(CC1101_TEST1 ,0x35);		//00110101	For test only. Do not write to this register.
	writeRegister(CC1101_TEST0 ,0x0B);		//00001011	For test only. Do not write to this register.
	writeRegister(CC1101_PKTCTRL0 ,0x12);	//00010010	Enable infinite length packets, CRC disabled, Turn data whitening off, Serial Synchronous mode
	writeRegister(CC1101_ADDR ,0x00);		//00000000
	writeRegister(CC1101_PKTLEN ,0xFF);		//11111111	//Not used, no hardware packet handling

	//0x6F,0x26,0x2E,0x8C,0x87,0xCD,0xC7,0xC0
	writeBurstRegister(CC1101_PATABLE | CC1101_WRITE_BURST, (uint8_t*)ithoPaTableSend, 8);

	//difference, message1 sends a STX here
	writeCommand(CC1101_SIDLE);
	writeCommand(CC1101_SIDLE);

	writeRegister(CC1101_MDMCFG4 ,0x5A);	//difference compared to message1
	writeRegister(CC1101_MDMCFG3 ,0x83);	//difference compared to message1
	writeRegister(CC1101_DEVIATN ,0x50);	//difference compared to message1
	writeRegister(CC1101_IOCFG0 ,0x2D);		//GDO0_Z_EN_N. When this output is 0, GDO0 is configured as input (for serial TX data).
	writeRegister(CC1101_IOCFG1 ,0x0B);		//Serial Clock. Synchronous to the data in synchronous serial mode.

	writeCommand(CC1101_STX);
	writeCommand(CC1101_SIDLE);

	writeRegister(CC1101_MDMCFG4 ,0x5A);	//difference compared to message1
	writeRegister(CC1101_MDMCFG3 ,0x83);	//difference compared to message1
	writeRegister(CC1101_DEVIATN ,0x50);	//difference compared to message1
	//writeRegister(CC1101_IOCFG0 ,0x2D);		//GDO0_Z_EN_N. When this output is 0, GDO0 is configured as input (for serial TX data).
	//writeRegister(CC1101_IOCFG1 ,0x0B);		//Serial Clock. Synchronous to the data in synchronous serial mode.

	//Itho is using serial mode for transmit. We want to use the TX FIFO with fixed packet length for simplicity.
	writeRegister(CC1101_IOCFG0 ,0x2E);
	writeRegister(CC1101_IOCFG1 ,0x2E);
	writeRegister(CC1101_PKTCTRL0 ,0x00);
	writeRegister(CC1101_PKTCTRL1 ,0x00);
	
	switch (command)
	{
		case IthoJoin:
			writeRegister(CC1101_PKTLEN , 72);
			break;
			
		case IthoLeave:
			writeRegister(CC1101_PKTLEN , 57);
			break;
		
		default:
			writeRegister(CC1101_PKTLEN , 50);		
			break;
	}
}

void IthoCC1101::finishTransfer()
{
	writeCommand(CC1101_SIDLE);
	delayMicroseconds(1);

	writeRegister(CC1101_IOCFG0 ,0x2E);
	writeRegister(CC1101_IOCFG1 ,0x2E);
	
	writeCommand(CC1101_SIDLE);
	writeCommand(CC1101_SPWD);
}

void IthoCC1101::initReceive()
{
	/*
	Configuration reverse engineered from RFT print.
	
	Base frequency		868.299866MHz
	Channel				0
	Channel spacing		199.951172kHz
	Carrier frequency	868.299866MHz
	Xtal frequency		26.000000MHz
	Data rate			38.3835kBaud
	RX filter BW		325.000000kHz
	Manchester			disabled
	Modulation			2-FSK
	Deviation			50.781250kHz
	TX power			0x6F,0x26,0x2E,0x7F,0x8A,0x84,0xCA,0xC4
	PA ramping			enabled
	Whitening			disabled
	*/	
	writeCommand(CC1101_SRES);

	writeRegister(CC1101_TEST0 ,0x09);
	writeRegister(CC1101_FSCAL2 ,0x00);
	
	//0x6F,0x26,0x2E,0x7F,0x8A,0x84,0xCA,0xC4
	writeBurstRegister(CC1101_PATABLE | CC1101_WRITE_BURST, (uint8_t*)ithoPaTableReceive, 8);
	
	writeCommand(CC1101_SCAL);

	//wait for calibration to finish
	while ((readRegisterWithSyncProblem(CC1101_MARCSTATE, CC1101_STATUS_REGISTER)) != CC1101_MARCSTATE_IDLE) yield();

	writeRegister(CC1101_FSCAL2 ,0x00);
	writeRegister(CC1101_MCSM0 ,0x18);			//no auto calibrate
	writeRegister(CC1101_FREQ2 ,0x21);
	writeRegister(CC1101_FREQ1 ,0x65);
	writeRegister(CC1101_FREQ0 ,0x6A);
	writeRegister(CC1101_IOCFG0 ,0x2E);			//High impedance (3-state)
	writeRegister(CC1101_IOCFG2 ,0x2E);			//High impedance (3-state)
	writeRegister(CC1101_FSCTRL1 ,0x06);
	writeRegister(CC1101_FSCTRL0 ,0x00);
	writeRegister(CC1101_MDMCFG4 ,0x5A);
	writeRegister(CC1101_MDMCFG3 ,0x83);
	writeRegister(CC1101_MDMCFG2 ,0x00);		//Enable digital DC blocking filter before demodulator, 2-FSK, Disable Manchester encoding/decoding, No preamble/sync 
	writeRegister(CC1101_MDMCFG1 ,0x22);		//Disable FEC
	writeRegister(CC1101_MDMCFG0 ,0xF8);
	writeRegister(CC1101_CHANNR ,0x00);
	writeRegister(CC1101_DEVIATN ,0x50);
	writeRegister(CC1101_FREND1 ,0x56);
	writeRegister(CC1101_FREND0 ,0x17);
	writeRegister(CC1101_MCSM0 ,0x18);			//no auto calibrate
	writeRegister(CC1101_FOCCFG ,0x16);
	writeRegister(CC1101_BSCFG ,0x6C);
	writeRegister(CC1101_AGCCTRL2 ,0x43);
	writeRegister(CC1101_AGCCTRL1 ,0x40);
	writeRegister(CC1101_AGCCTRL0 ,0x91);
	writeRegister(CC1101_FSCAL3 ,0xA9);
	writeRegister(CC1101_FSCAL2 ,0x2A);
	writeRegister(CC1101_FSCAL1 ,0x00);
	writeRegister(CC1101_FSCAL0 ,0x11);
	writeRegister(CC1101_FSTEST ,0x59);
	writeRegister(CC1101_TEST2 ,0x81);
	writeRegister(CC1101_TEST1 ,0x35);
	writeRegister(CC1101_TEST0 ,0x0B);
	writeRegister(CC1101_PKTCTRL1 ,0x04);		//No address check, Append two bytes with status RSSI/LQI/CRC OK, 
	writeRegister(CC1101_PKTCTRL0 ,0x32);		//Infinite packet length mode, CRC disabled for TX and RX, No data whitening, Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins 
	writeRegister(CC1101_ADDR ,0x00);
	writeRegister(CC1101_PKTLEN ,0xFF);
	writeRegister(CC1101_TEST0 ,0x09);
	writeRegister(CC1101_FSCAL2 ,0x00);

	writeCommand(CC1101_SCAL);

	//wait for calibration to finish
	while ((readRegisterWithSyncProblem(CC1101_MARCSTATE, CC1101_STATUS_REGISTER)) != CC1101_MARCSTATE_IDLE) yield();

	writeRegister(CC1101_MCSM0 ,0x18);			//no auto calibrate
	
	writeCommand(CC1101_SIDLE);
	writeCommand(CC1101_SIDLE);
	
	writeRegister(CC1101_MDMCFG2 ,0x00);		//Enable digital DC blocking filter before demodulator, 2-FSK, Disable Manchester encoding/decoding, No preamble/sync 
	writeRegister(CC1101_IOCFG0 ,0x0D);			//Serial Data Output. Used for asynchronous serial mode.

	writeCommand(CC1101_SRX);
	
	while ((readRegisterWithSyncProblem(CC1101_MARCSTATE, CC1101_STATUS_REGISTER)) != CC1101_MARCSTATE_RX) yield();
	
	initReceiveMessage1();
}

void IthoCC1101::initReceiveMessage1()
{
	uint8_t marcState;
	
	writeCommand(CC1101_SIDLE);	//idle
	
	//set datarate
	writeRegister(CC1101_MDMCFG4 ,0x08);
	writeRegister(CC1101_MDMCFG3 ,0x43);
	writeRegister(CC1101_DEVIATN ,0x40);
		
	//set fifo mode with fixed packet length and sync bytes
	writeRegister(CC1101_PKTLEN , 15);		//15 bytes message (sync at beginning of message is removed by CC1101)
	writeRegister(CC1101_PKTCTRL0 ,0x00);
	writeRegister(CC1101_SYNC1 ,170);		//message1 byte2
	writeRegister(CC1101_SYNC0 ,173);		//message1 byte3
	writeRegister(CC1101_MDMCFG2 ,0x02);
	writeRegister(CC1101_PKTCTRL1 ,0x00);	
	
	writeCommand(CC1101_SRX);				//switch to RX state

	// Check that the RX state has been entered
	while (((marcState = readRegisterWithSyncProblem(CC1101_MARCSTATE, CC1101_STATUS_REGISTER)) & CC1101_BITS_MARCSTATE) != CC1101_MARCSTATE_RX)
	{
		if (marcState == CC1101_MARCSTATE_RXFIFO_OVERFLOW) // RX_OVERFLOW
			writeCommand(CC1101_SFRX); //flush RX buffer
		yield();
	}
	
	receiveState = ExpectMessageStart;
}

void IthoCC1101::initReceiveMessage2(IthoCommand expectedCommand)
{
	uint8_t marcState;
	
	writeCommand(CC1101_SIDLE);	//idle
	
	//set datarate	
	writeRegister(CC1101_MDMCFG4 ,0x5A);
	writeRegister(CC1101_MDMCFG3 ,0x83);
	writeRegister(CC1101_DEVIATN ,0x50);
	
	//set packet length based on expected message
	switch (expectedCommand)
	{
		case IthoJoin:
			writeRegister(CC1101_PKTLEN ,64);
			receiveState = ExpectJoinCommand;
			break;
			
		case IthoLeave:
			writeRegister(CC1101_PKTLEN ,57);
			receiveState = ExpectLeaveCommand;
			break;					
			
		default:
			writeRegister(CC1101_PKTLEN ,42);			//42 bytes message (sync at beginning of message is removed by CC1101)
			receiveState = ExpectNormalCommand;
			break;
	}
	
	//set fifo mode with fixed packet length and sync bytes
	writeRegister(CC1101_PKTCTRL0 ,0x00);
	writeRegister(CC1101_SYNC1 ,170);			//message2 byte6
	writeRegister(CC1101_SYNC0 ,171);			//message2 byte7
	writeRegister(CC1101_MDMCFG2 ,0x02);
	writeRegister(CC1101_PKTCTRL1 ,0x00);	
	
	writeCommand(CC1101_SRX); //switch to RX state

	// Check that the RX state has been entered
	while (((marcState = readRegisterWithSyncProblem(CC1101_MARCSTATE, CC1101_STATUS_REGISTER)) & CC1101_BITS_MARCSTATE) != CC1101_MARCSTATE_RX)
	{
		if (marcState == CC1101_MARCSTATE_RXFIFO_OVERFLOW) // RX_OVERFLOW
			writeCommand(CC1101_SFRX); //flush RX buffer
		yield();
	}
}

bool IthoCC1101::checkForNewPacket()
{
	bool result = false;	
	uint8_t length;
	
	switch (receiveState)
	{
		case ExpectMessageStart:
			length = receiveData(&inMessage1, 15);
	
			//check if message1 is received
			if ((length > 0) && (isValidMessageStart()))
			{
				parseMessageStart();
					
				//switch to message2 RF settings
				initReceiveMessage2(inIthoPacket.command);
				
				lastMessage1Received = millis();
			}
						
			break;
		
		case ExpectNormalCommand:
			length = receiveData(&inMessage2, 42);
						
			//check if message2 is received
			if ((length > 0) && (isValidMessageCommand()))
			{
				parseMessageCommand();
				
				//bug detection
				//testCreateMessage();				
								
				//switch back to message1 RF settings
				initReceiveMessage1();
							
				//both messages are received
				result = true;
			}	
			else
			{
				//check if waiting timeout is reached (message2 is expected within 22ms after message1)
				if (millis() - lastMessage1Received > 24)
				{
					//switch back to message1 RF settings
					initReceiveMessage1();
				}
			}
				
			break;
			
		case ExpectJoinCommand:
			length = receiveData(&inMessage2, 64);
		
			//check if message2 is received
			if ((length > 0) && (isValidMessageCommand()) && isValidMessageJoin())
			{
				parseMessageCommand();
				parseMessageJoin();
				
				//bug detection
				//testCreateMessage();
				
				//switch back to message1 RF settings
				initReceiveMessage1();
				
				//both messages are received
				result = true;
			}
			else
			{
				//check if waiting timeout is reached (message2 is expected within 22ms after message1)
				if (millis() - lastMessage1Received > 24)
				{
					//switch back to message1 RF settings
					initReceiveMessage1();
				}
			}
		
			break;	
			
		case ExpectLeaveCommand:
			length = receiveData(&inMessage2, 45);
			
			//check if message2 is received
			if ((length > 0) && (isValidMessageCommand()) && isValidMessageLeave())
			{
				parseMessageCommand();
				parseMessageLeave();
				
				//bug detection
				//testCreateMessage();
				
				//switch back to message1 RF settings
				initReceiveMessage1();
				
				//both messages are received
				result = true;
			}
			else
			{
				//check if waiting timeout is reached (message2 is expected within 22ms after message1)
				if (millis() - lastMessage1Received > 24)
				{
					//switch back to message1 RF settings
					initReceiveMessage1();
				}
			}
			
			break;
								
	}
	
	return result;
}

bool IthoCC1101::isValidMessageStart()
{
	if (inMessage1.data[12] != 170)	
	{
		return false;
	}
	
	return true;
}

bool IthoCC1101::isValidMessageCommand()
{
	if (inMessage2.data[37] != 170) 
	{
		return false;
	}
	
	return true;
}

bool IthoCC1101::isValidMessageJoin()
{
	/*
	if (inMessage2.data[37] != 170)
	{
		return false;
	}
	*/
	return true;
}

bool IthoCC1101::isValidMessageLeave()
{
	/*
	if (inMessage2.data[37] != 170)
	{
		return false;
	}
	*/
	return true;
}

void IthoCC1101::parseMessageStart()
{
	bool isFullCommand = true;
	bool isMediumCommand = true;
	bool isLowCommand = true;
	bool isTimer1Command = true;
	bool isTimer2Command = true;
	bool isTimer3Command = true;
	bool isJoinCommand = true;
	bool isLeaveCommand = true;
	
	//copy device id from packet
	// TODO: verify if this really is this the device id
	inIthoPacket.deviceId[0] = inMessage1.data[2];
	inIthoPacket.deviceId[1] = inMessage1.data[3];
	inIthoPacket.deviceId[2] = inMessage1.data[4] & 0b11111110;	//last bit is part of command
	
	//copy command data from packet
	//message1 command starts at index 5, last bit!
	uint8_t commandBytes[7];
	commandBytes[0] = inMessage1.data[5] & 0b00000001;
	commandBytes[1] = inMessage1.data[6];
	commandBytes[2] = inMessage1.data[7];
	commandBytes[3] = inMessage1.data[8];
	commandBytes[4] = inMessage1.data[9];
	commandBytes[5] = inMessage1.data[10];
	commandBytes[6] = inMessage1.data[11];
	
	//match received commandBytes with known command bytes
	for (int i=0; i<7; i++)
	{
		if (commandBytes[i] != ithoMessage1FullCommandBytes[i]) isFullCommand = false;
		if (commandBytes[i] != ithoMessage1MediumCommandBytes[i]) isMediumCommand = false;
		if (commandBytes[i] != ithoMessage1LowCommandBytes[i]) isLowCommand = false;
		if (commandBytes[i] != ithoMessage1Timer1CommandBytes[i]) isTimer1Command = false;
		if (commandBytes[i] != ithoMessage1Timer2CommandBytes[i]) isTimer2Command = false;
		if (commandBytes[i] != ithoMessage1Timer3CommandBytes[i]) isTimer3Command = false;
		if (commandBytes[i] != ithoMessage1JoinCommandBytes[i]) isJoinCommand = false;
		if (commandBytes[i] != ithoMessage1LeaveCommandBytes[i]) isLeaveCommand = false;
	}
	
	//determine command
	inIthoPacket.command = IthoUnknown;
	if (isFullCommand) inIthoPacket.command = IthoFull;
	if (isMediumCommand) inIthoPacket.command = IthoMedium;
	if (isLowCommand) inIthoPacket.command = IthoLow;
	if (isTimer1Command) inIthoPacket.command = IthoTimer1;
	if (isTimer2Command) inIthoPacket.command = IthoTimer2;
	if (isTimer3Command) inIthoPacket.command = IthoTimer3;
	if (isJoinCommand) inIthoPacket.command = IthoJoin;
	if (isLeaveCommand) inIthoPacket.command = IthoLeave;	
	
	//previous command
	inIthoPacket.previous = getMessage1PreviousCommand(inMessage1.data[14]);
}

void IthoCC1101::parseMessageCommand()
{
	bool isFullCommand = true;
	bool isMediumCommand = true;
	bool isLowCommand = true;
	bool isTimer1Command = true;
	bool isTimer2Command = true;
	bool isTimer3Command = true;
	bool isJoinCommand = true;
	bool isLeaveCommand = true;
		
	//device id
	inIthoPacket.deviceId[0] = inMessage2.data[8];
	inIthoPacket.deviceId[1] = inMessage2.data[9];
	inIthoPacket.deviceId[2] = inMessage2.data[10];
	inIthoPacket.deviceId[3] = inMessage2.data[11];
	inIthoPacket.deviceId[4] = inMessage2.data[12];
	inIthoPacket.deviceId[5] = inMessage2.data[13];
	inIthoPacket.deviceId[6] = inMessage2.data[14];
	inIthoPacket.deviceId[7] = inMessage2.data[15];
		
	//counter1
	uint8_t row0 = inMessage2.data[16];
	uint8_t row1 = inMessage2.data[17];
	uint8_t row2 = inMessage2.data[18] & 0b11110000;	//4 bits are part of command
	inIthoPacket.counter = calculateMessageCounter(row0, row1, row2);
	
	//copy command data from packet
	uint8_t commandBytes[15];
	commandBytes[0] = inMessage2.data[18] & 0b00001111;	//4 bits are part of counter1
	commandBytes[1] = inMessage2.data[19];
	commandBytes[2] = inMessage2.data[20];
	commandBytes[3] = inMessage2.data[21];
	commandBytes[4] = inMessage2.data[22];		
	commandBytes[5] = inMessage2.data[23];		
	commandBytes[6] = inMessage2.data[24];		
	commandBytes[7] = inMessage2.data[25];		
	commandBytes[8] = inMessage2.data[26];		
	commandBytes[9] = inMessage2.data[27];		
	commandBytes[10] = inMessage2.data[28];		
	commandBytes[11] = inMessage2.data[29];
	commandBytes[12] = inMessage2.data[30];		
	commandBytes[13] = inMessage2.data[31];		
	commandBytes[14] = inMessage2.data[32];				
						
	//match received commandBytes with known command bytes
	for (int i=0; i<15; i++)
	{
		if (commandBytes[i] != ithoMessage2FullCommandBytes[i]) isFullCommand = false;
		if (commandBytes[i] != ithoMessage2MediumCommandBytes[i]) isMediumCommand = false;
		if (commandBytes[i] != ithoMessage2LowCommandBytes[i]) isLowCommand = false;
		if (commandBytes[i] != ithoMessage2Timer1CommandBytes[i]) isTimer1Command = false;
		if (commandBytes[i] != ithoMessage2Timer2CommandBytes[i]) isTimer2Command = false;
		if (commandBytes[i] != ithoMessage2Timer3CommandBytes[i]) isTimer3Command = false;
		if (commandBytes[i] != ithoMessage2JoinCommandBytes[i]) isJoinCommand = false;
		if (commandBytes[i] != ithoMessage2LeaveCommandBytes[i]) isLeaveCommand = false;
	}	
		
	//determine command
	inIthoPacket.command = IthoUnknown;
	if (isFullCommand) inIthoPacket.command = IthoFull;
	if (isMediumCommand) inIthoPacket.command = IthoMedium;
	if (isLowCommand) inIthoPacket.command = IthoLow;
	if (isTimer1Command) inIthoPacket.command = IthoTimer1;
	if (isTimer2Command) inIthoPacket.command = IthoTimer2;
	if (isTimer3Command) inIthoPacket.command = IthoTimer3;
	if (isJoinCommand) inIthoPacket.command = IthoJoin;
	if (isLeaveCommand) inIthoPacket.command = IthoLeave;	
}

void IthoCC1101::parseMessageJoin()
{
	/*
	for (int i=0;i<inMessage2.length;i++)
	{
		Serial.print(inMessage2.data[i]);
		Serial.print("\n");
		
	}	
	*/
}

void IthoCC1101::parseMessageLeave()
{
	/*
	for (int i=0;i<inMessage2.length;i++)
	{
		Serial.print(inMessage2.data[i]);
		Serial.print("\n");
		
	}	
	*/
}

//check if we can generate the same input message based on the counter and command only (yes we can!)
void IthoCC1101::testCreateMessage()
{
	CC1101Packet outMessage1;
	CC1101Packet outMessage2;
	
	//update itho packet data
	outIthoPacket.previous = inIthoPacket.previous;
	outIthoPacket.command = inIthoPacket.command;
	outIthoPacket.counter =  inIthoPacket.counter;
	
	//get message1 bytes
	createMessageStart(&outIthoPacket, &outMessage1);
	
	for (int i=0; i<inMessage1.length;i++)
	{
		if (inMessage1.data[i] != outMessage1.data[i+4])
		{
			Serial.print("message1 not ok=byte");
			Serial.print(i+4);
			Serial.print(" (");
			Serial.print(inMessage1.data[i]);
			Serial.print("/");
			Serial.print(outMessage1.data[i+4]);
			Serial.print(")\n");
		}
	}
	
	//get message2 bytes
	switch (outIthoPacket.command)
	{
		case IthoJoin:
			createMessageJoin(&outIthoPacket, &outMessage2);						
			break;
			
		case IthoLeave:
			createMessageLeave(&outIthoPacket, &outMessage2);						
			break;
			
		default:
			createMessageCommand(&outIthoPacket, &outMessage2);				
			break;
	}
	
	for (int i=0; i<inMessage2.length;i++)
	{
		if (inMessage2.data[i] != outMessage2.data[i+8])
		{
			Serial.print("message2 not ok=byte");
			Serial.print(i+8);
			Serial.print(" (");
			Serial.print(inMessage2.data[i]);
			Serial.print("/");
			Serial.print(outMessage2.data[i+8]);
			Serial.print(")\n");
		}
	}
}

void IthoCC1101::sendCommand(IthoCommand command)
{
	CC1101Packet outMessage1;
	CC1101Packet outMessage2;
	uint8_t maxTries = sendTries;
	uint8_t delaytime = 40;
	
	//update itho packet data
	outIthoPacket.previous = outIthoPacket.command;
	outIthoPacket.command = command;
	outIthoPacket.counter += 1;
	
	//get message1 bytes
	createMessageStart(&outIthoPacket, &outMessage1);
	
	//get message2 bytes
	switch (command)
	{
		case IthoJoin:
			createMessageJoin(&outIthoPacket, &outMessage2);
			break;
		
		case IthoLeave:
			createMessageLeave(&outIthoPacket, &outMessage2);
			//the leave command needs to be transmitted for 1 second according the manual
			maxTries = 30;
			delaytime = 4;
			break;
		
		default:
			createMessageCommand(&outIthoPacket, &outMessage2);
			break;
	}
	
	Serial.print("send\n");
	
	//send messages
	for (int i=0;i<maxTries;i++)
	{
		//message1
		initSendMessage1();
		sendData(&outMessage1);
		
		delay(4);
		
		//message2
		initSendMessage2(outIthoPacket.command);
		sendData(&outMessage2);
		
		finishTransfer();
		delay(delaytime);
	}
}

void IthoCC1101::createMessageStart(IthoPacket *itho, CC1101Packet *packet)
{
	packet->length = 19;

	//fixed
	packet->data[0] = 170;
	packet->data[1] = 170;
	packet->data[2] = 170;	
	packet->data[3] = 173;
	
	//??
	packet->data[4] = 51;
	packet->data[5] = 83;	
	packet->data[6] = 51;
	packet->data[7] = 43;
	packet->data[8] = 84;
	packet->data[9] = 204;	//last bit is part of command

	//command
	uint8_t *commandBytes = getMessage1CommandBytes(itho->command);
	packet->data[9] = packet->data[9] | commandBytes[0];	//only last bit is set
	packet->data[10] = commandBytes[1];
	packet->data[11] = commandBytes[2];
	packet->data[12] = commandBytes[3];
	packet->data[13] = commandBytes[4];
	packet->data[14] = commandBytes[5];
	packet->data[15] = commandBytes[6];
	
	//fixed
	packet->data[16] = 170;
	packet->data[17] = 171;
	
	//previous command
	packet->data[18] = getMessage1Byte18(itho->previous);
}

void IthoCC1101::createMessageCommand(IthoPacket *itho, CC1101Packet *packet)
{
	packet->length = 50;
	
	//fixed
	packet->data[0] = 170;
	packet->data[1] = 170;
	packet->data[2] = 170;
	packet->data[3] = 170;
	packet->data[4] = 170;
	packet->data[5] = 170;
	packet->data[6] = 170;				
	packet->data[7] = 171;	
	packet->data[8] = 254;				
	packet->data[9] = 0;				
	packet->data[10] = 179;				
	packet->data[11] = 42;				
	packet->data[12] = 171;				
	packet->data[13] = 42;				
	packet->data[14] = 149;				
	packet->data[15] = 154;				
	
	//device id
	packet->data[16] = itho->deviceId[0];
	packet->data[17] = itho->deviceId[1];
	packet->data[18] = itho->deviceId[2];
	packet->data[19] = itho->deviceId[3];
	packet->data[20] = itho->deviceId[4];
	packet->data[21] = itho->deviceId[5];
	packet->data[22] = itho->deviceId[6];
	packet->data[23] = itho->deviceId[7];
	
	//counter bytes
	packet->data[24] = calculateMessage2Byte24(itho->counter);
	packet->data[25] = calculateMessage2Byte25(itho->counter);
	packet->data[26] = calculateMessage2Byte26(itho->counter);

	//command
	uint8_t *commandBytes = getMessage2CommandBytes(itho->command);
	packet->data[26] = packet->data[26] | commandBytes[0];
	packet->data[27] = commandBytes[1];
	packet->data[28] = commandBytes[2];
	packet->data[29] = commandBytes[3];
	packet->data[30] = commandBytes[4];
	packet->data[31] = commandBytes[5];
	packet->data[32] = commandBytes[6];
	packet->data[33] = commandBytes[7];
	packet->data[34] = commandBytes[8];
	packet->data[35] = commandBytes[9];
	packet->data[36] = commandBytes[10];
	packet->data[37] = commandBytes[11];
	packet->data[38] = commandBytes[12];
	packet->data[39] = commandBytes[13];	
	packet->data[40] = commandBytes[14];

	//counter bytes
	packet->data[41] = calculateMessage2Byte41(itho->counter, itho->command);
	packet->data[42] = calculateMessage2Byte42(itho->counter, itho->command);
	packet->data[43] = calculateMessage2Byte43(itho->counter, itho->command);
	
	//fixed
	packet->data[44] = 172;
	packet->data[45] = 170;
	packet->data[46] = 170;
	packet->data[47] = 170;	
	packet->data[48] = 170;	
	packet->data[49] = 170;	
}

void IthoCC1101::createMessageJoin(IthoPacket *itho, CC1101Packet *packet)
{
	//message3 is an extension on message2
	createMessageCommand(itho, packet);
		
	packet->length = 72;
	
	//part of device id??
	packet->data[44] = itho->deviceId[3];
	packet->data[45] = itho->deviceId[4];
	packet->data[46] = itho->deviceId[5];
	packet->data[47] = itho->deviceId[6];
	packet->data[48] = itho->deviceId[7];
	
	//command join
	packet->data[49] = 85;
	
	//fixed
	packet->data[50] = 165;
	packet->data[51] = 105;
	packet->data[52] = 89;
	packet->data[53] = 86;
	packet->data[54] = 106;
	packet->data[55] = 149;
	
	//device id
	packet->data[56] = itho->deviceId[0];
	packet->data[57] = itho->deviceId[1];
	packet->data[58] = itho->deviceId[2];
	packet->data[59] = itho->deviceId[3];
	packet->data[60] = itho->deviceId[4];
	packet->data[61] = itho->deviceId[5];
	packet->data[62] = itho->deviceId[6];
	packet->data[63] = itho->deviceId[7];
	
	//counter bytes
	packet->data[64] = calculateMessage2Byte64(itho->counter);
	packet->data[65] = calculateMessage2Byte65(itho->counter);
	packet->data[66] = calculateMessage2Byte66(itho->counter);
	
	//fixed
	packet->data[67] = 202;
	packet->data[68] = 170;
	packet->data[69] = 170;
	packet->data[70] = 170;
	packet->data[71] = 170;
}

void IthoCC1101::createMessageLeave(IthoPacket *itho, CC1101Packet *packet)
{
	//message3 is an extension on message2
	createMessageCommand(itho, packet);
	
	packet->length = 57;
	
	//part of device id??
	packet->data[44] = itho->deviceId[3];
	packet->data[45] = itho->deviceId[4];
	packet->data[46] = itho->deviceId[5];
	packet->data[47] = itho->deviceId[6];
	packet->data[48] = itho->deviceId[7];
	
	//counter bytes
	packet->data[49] = calculateMessage2Byte49(itho->counter);
	packet->data[50] = calculateMessage2Byte50(itho->counter);
	packet->data[51] = calculateMessage2Byte51(itho->counter);
		
	//fixed
	packet->data[52] = 202;
	packet->data[53] = 170;
	packet->data[54] = 170;
	packet->data[55] = 170;
	packet->data[56] = 170;
}

//calculate 0-255 number out of 3 counter bytes
uint8_t IthoCC1101::calculateMessageCounter(uint8_t byte24, uint8_t byte25, uint8_t byte26)
{
	uint8_t result;
	
	uint8_t a = getCounterIndex(&counterBytes24a[0],2,byte24 & 0b00000011);	//last 2 bits only
	uint8_t b = getCounterIndex(&counterBytes24b[0],8,byte24 & 0b11111100);	//first 6 bits
	uint8_t c = getCounterIndex(&counterBytes25[0],8,byte25);
	uint8_t d = getCounterIndex(&counterBytes26[0],2,byte26);
	
	result = (a * 128) + (b * 16) + (d * 8) + c;
	
	return result;
}

IthoCommand IthoCC1101::getMessage1PreviousCommand(uint8_t byte18)
{
	switch (byte18)
	{
		case 77:
			return IthoJoin;
			
		case 82:
			return IthoLeave;
			
		case 85:
			return IthoLow;
	}
}

uint8_t IthoCC1101::getMessage1Byte18(IthoCommand command)
{
	switch (command)
	{
		case IthoJoin:
			return 77;
		
		case IthoLeave:
			return 82;
		
		default:
			return 85;
	}
}

uint8_t IthoCC1101::calculateMessage2Byte24(uint8_t counter)
{
	//the following code skips the counterBytes arrays, this saves 10 bytes of memory but will increase program size with 30 bytes
	//uint8_t index = (counter & 0x7F) >> 4;
	//return ((counter >> 7) + 1) | (84 + (((index + 1) >> 1) << 6) - ((index >> 1) * 48) - (index >= 4 ? 28 : 0));
		
	return counterBytes24a[(counter / 128)] | counterBytes24b[(counter % 128) / 16];	
}

uint8_t IthoCC1101::calculateMessage2Byte25(uint8_t counter)
{
	return counterBytes25[(counter % 16) % 8];
}

uint8_t IthoCC1101::calculateMessage2Byte26(uint8_t counter)
{
	return counterBytes26[(counter % 16) / 8];
}

uint8_t IthoCC1101::calculateMessage2Byte41(uint8_t counter, IthoCommand command)
{
	int var = 0;
	uint8_t hi = 0;
	
	switch (command)
	{
		case IthoTimer1:
		case IthoTimer3:
			hi = 160;
			var = 48 - command;
			if (counter < var) counter = 64 - counter;
			break;
			
		case IthoJoin:
			hi = 96;
			counter = 0;
			break;
				
		case IthoLeave:
			hi = 160;
			counter = 0;
			break;
							
		case IthoLow:
		case IthoMedium:
		case IthoFull:
		case IthoTimer2:
			hi = 96;
			var = 48 - command;
			if (counter < var) counter = 74 - counter;
			break;		
	}

	return (hi | counterBytes41[((counter - var) % 64) / 16]);
}

uint8_t IthoCC1101::calculateMessage2Byte42(uint8_t counter, IthoCommand command)
{
	uint8_t result;
	
	if (command == IthoJoin || command == IthoLeave)
	{
		counter = 1;
	}
	else
	{
		counter += command;
	}

	result = counterBytes42[counter / 64];

	if (counter % 2 == 1) result -= 1;
		
	return result;
}

uint8_t IthoCC1101::calculateMessage2Byte43(uint8_t counter, IthoCommand command)
{
	switch (command)
	{
		case IthoMedium:
			break;
				
		case IthoLow:
		case IthoTimer2:
			if (counter % 2 == 0) counter -= 1;
			break;
			
		case IthoFull:
			counter += 2;
			if (counter % 2 == 0) counter -= 1;
			break;
			
		case IthoTimer1:
			counter += 6;
			if (counter % 2 == 0) counter -= 1;		
			break;
			
		case IthoTimer3:
			counter += 10;
			if (counter % 2 == 0) counter -= 1;		
			break;
			
		case IthoJoin:
		case IthoLeave:
			counter = 0;
			break;	
	}

	return counterBytes43[(counter % 16) / 2];
}

uint8_t IthoCC1101::calculateMessage2Byte49(uint8_t counter)
{
	counter += 47;
	return counterBytes64[(counter / 16)];
}

uint8_t IthoCC1101::calculateMessage2Byte50(uint8_t counter)
{
	counter -= 4;
	return counterBytes65[counter % 8];
}

uint8_t IthoCC1101::calculateMessage2Byte51(uint8_t counter)
{
	counter -= 1;
	return counterBytes66[(counter % 16) / 8];
}

uint8_t IthoCC1101::calculateMessage2Byte64(uint8_t counter)
{
	counter += 3;	
	return counterBytes64[counter / 16];
}

uint8_t IthoCC1101::calculateMessage2Byte65(uint8_t counter)
{
	return counterBytes65[counter % 8];
}

uint8_t IthoCC1101::calculateMessage2Byte66(uint8_t counter)
{
	counter -= 13;
	return counterBytes66[(counter % 16) / 8];
}

uint8_t* IthoCC1101::getMessage1CommandBytes(IthoCommand command)
{
	switch (command)
	{
		case IthoFull:
		return (uint8_t*)&ithoMessage1FullCommandBytes[0];
		case IthoMedium:
		return (uint8_t*)&ithoMessage1MediumCommandBytes[0];
		case IthoLow:
		return (uint8_t*)&ithoMessage1LowCommandBytes[0];
		case IthoTimer1:
		return (uint8_t*)&ithoMessage1Timer1CommandBytes[0];
		case IthoTimer2:
		return (uint8_t*)&ithoMessage1Timer2CommandBytes[0];
		case IthoTimer3:
		return (uint8_t*)&ithoMessage1Timer3CommandBytes[0];
		case IthoJoin:
		return (uint8_t*)&ithoMessage1JoinCommandBytes[0];
		case IthoLeave:
		return (uint8_t*)&ithoMessage1LeaveCommandBytes[0];
	}
}

uint8_t* IthoCC1101::getMessage2CommandBytes(IthoCommand command)
{
	switch (command)
	{
		case IthoFull:
		return (uint8_t*)&ithoMessage2FullCommandBytes[0];
		case IthoMedium:
		return (uint8_t*)&ithoMessage2MediumCommandBytes[0];
		case IthoLow:
		return (uint8_t*)&ithoMessage2LowCommandBytes[0];
		case IthoTimer1:
		return (uint8_t*)&ithoMessage2Timer1CommandBytes[0];
		case IthoTimer2:
		return (uint8_t*)&ithoMessage2Timer2CommandBytes[0];
		case IthoTimer3:
		return (uint8_t*)&ithoMessage2Timer3CommandBytes[0];
		case IthoJoin:
		return (uint8_t*)&ithoMessage2JoinCommandBytes[0];
		case IthoLeave:
		return (uint8_t*)&ithoMessage2LeaveCommandBytes[0];
	}
}

//lookup value in array
uint8_t IthoCC1101::getCounterIndex(const uint8_t *arr, uint8_t length, uint8_t value)
{
	for (uint8_t i=0; i<length; i++)
		if (arr[i] == value)
			return i;
	
	//-1 should never be returned!
	return -1;
}