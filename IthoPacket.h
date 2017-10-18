/*
 * Author: Klusjesman, modified bij supersjimmie for Arduino/ESP8266
 */

#ifndef ITHOPACKET_H_
#define ITHOPACKET_H_


//do not change enum because they are used in calculations!
typedef enum IthoCommand 
{
	IthoUnknown = 0,
		
	IthoJoin = 4,
	IthoLeave = 8,
				
	IthoLow = 35,	
	IthoMedium = 36,	
	IthoFull = 37,
	
	IthoTimer1 = 41,
	IthoTimer2 = 51,
	IthoTimer3 = 61
};


class IthoPacket
{
	public:
		uint8_t deviceId[8];
		IthoCommand command;
		IthoCommand previous;
		
		uint8_t counter;		//0-255, counter is increased on every remote button press
};


#endif /* ITHOPACKET_H_ */