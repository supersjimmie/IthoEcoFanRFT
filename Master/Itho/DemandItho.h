//#include "IthoCC1101.h"
#include "CC1101Packet.h"

class DemandIthoCommand {
    public:
		uint8_t length;
		uint8_t data[72];
};

const DemandIthoCommand commandList[] = {
    {3, {1,2,3}},
    {3, {1,2,3}},
    {3, {1,2,3}}
};

class DemandItho //: public IthoCC1101
{
   public:
        void decodeLastCommand();

    public:
        static uint8_t decode(CC1101Packet& packetIn, CC1101Packet& decoded);

        // return bit idx, start countin from byte 0, assume single string of bits
        static bool getBit(CC1101Packet& p, int idx);
        static void setBit(CC1101Packet& p, int idx, bool value);

        //String Packet2str(CC1101Packet& p , bool ashex = true);
};
