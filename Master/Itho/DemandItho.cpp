#include "stdlib.h"
#include "DemandItho.h"

// void DemandItho::decodeLastCommand()
// {
//     printf("decodeLastCommand\n");
//     CC1101Packet p;
//     CC1101Packet received = getLastMessagePacket();
//     uint8_t errorCnt = decode(received, p);
//     printf("\terr = %d\n", errorCnt);
// }

uint8_t DemandItho::decode(CC1101Packet& in, CC1101Packet& decoded)
{
    uint8_t errorCount = 0;
    for (int i = 0; i < 8; i++) {
        printf("skip %08x\n", in.data[i]);
    }

    // init response
    const int numberOfLeadByte = 8;
    int numberOfNibble = ((in.length - 8)*8) / (2*(8+2));

    printf("nn = %d\n", numberOfNibble);

    decoded.length = (numberOfNibble - (numberOfLeadByte *2))/2;
    for (int i = 0; i < decoded.length; i++) {
        decoded.data[i] = 0;
    }

    // skip preamble and first 2 bits
    int bitIndex = (numberOfLeadByte*8) + 2;
    for (int nibbleIndex = 16; nibbleIndex < numberOfNibble; nibbleIndex++) {
        for (int idxInNibble = 0; idxInNibble < 4; idxInNibble++) {
            bool value = getBit(in, bitIndex);
            setBit(decoded, 8*(nibbleIndex / 2) + 4*(1 - (nibbleIndex%2)), value);
            bitIndex++; 
            if (value == getBit(in, bitIndex)) errorCount++;
            bitIndex++; // skip a bit
        }
        // at the end of a nibble, there should be a 0
        if (getBit(in, bitIndex) != false) errorCount++;
    }

    //Serial.println(Packet2str(decoded));
    return errorCount;
     
}

bool DemandItho::getBit(CC1101Packet& p, int idx)
{
    int byteIdx = idx / 8;
    int bitInByte = 7 - (idx % 8);
    return ((p.data[byteIdx] >> bitInByte) & 1);
}

void DemandItho::setBit(CC1101Packet& p, int idx, bool value)
{
    int byteIdx = idx / 8;
    int bitInByte = 7 - (idx % 8);
    if (value) {
        p.data[byteIdx] = p.data[byteIdx] | (1 << bitInByte);
    } else {
        p.data[byteIdx] = p.data[byteIdx] & (0xff ^ (1 << bitInByte));
    }
}

// String DemandItho::Packet2str(CC1101Packet& p , bool ashex) {
//     String str;
//     for (uint8_t i=0; i<p.length;i++) {
//       if (ashex) {
// 	if (p.data[i] == 0) {
// 	  str += "0";
// 	}
// 	str += String(p.data[i], HEX);
//       }
//       else {
// 	str += String(p.data[i]);
//       }
//       if (i<p.length-1) str += ":";
//     }
//     return str;
// }
