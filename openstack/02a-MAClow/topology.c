#include "opendefs.h"
#include "topology.h"
#include "idmanager.h"

//=========================== defines =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================
void buildTopologyStructure();
uint8_t getHop(uint8_t byte6, uint8_t byte7);
//=========================== public ==========================================

topologyEntry_t entry[5];
bool isBuilt = FALSE;

bool topology_isAcceptablePacket(ieee802154_header_iht* ieee802514_header) {
#ifdef FORCETOPOLOGY
   bool returnVal=FALSE;

   uint8_t sink_byte6 = 0x96;
   uint8_t sink_byte7 = 0x63;

   if(!isBuilt) {buildTopologyStructure();}

   uint8_t senderByte6 = ieee802514_header->src.addr_64b[6];
   uint8_t senderByte7 = ieee802514_header->src.addr_64b[7];
   uint8_t senderHop = getHop(senderByte6,senderByte7);

   if (idmanager_getIsDAGroot()) {
      if (senderHop == 0) {
         returnVal = TRUE;
      }
   } else {
       uint8_t myByte6 =  idmanager_getMyID(ADDR_64B)->addr_64b[6];
       uint8_t myByte7 =  idmanager_getMyID(ADDR_64B)->addr_64b[7];;
       uint8_t myHop = getHop(myByte6,myByte7);

      if (myHop == 0 && senderByte6 == sink_byte6 && senderByte7 == sink_byte7) {
         returnVal=TRUE;
      } else {
         if (senderHop == myHop - 1 || senderHop == myHop + 1 ) {
            returnVal=TRUE;
         }
      }
   }
   return returnVal;
#else
   return TRUE;
#endif
}

//=========================== private =========================================

uint8_t getHop(uint8_t byte6, uint8_t byte7) {
    uint8_t hopNumber;
    uint8_t nodeIndex;
    for (hopNumber=0; hopNumber <= 4; hopNumber++) {
        for(nodeIndex = 0; nodeIndex <= 8; nodeIndex++ ) {
            if(entry[hopNumber].bytes6[nodeIndex] == byte6 && entry[hopNumber].bytes7[nodeIndex] == byte7) {
                return hopNumber;
            }
        }

    }
    return 99;
}

void buildTopologyStructure() {
   uint8_t hop = 0;

   //hop 1
   entry[hop].hop = hop;

   entry[hop].bytes6[0] = 0xb5;
   entry[hop].bytes7[0] = 0x58;

   entry[hop].bytes6[1] = 0x85;
   entry[hop].bytes7[1] = 0x61;

   entry[hop].bytes6[2] = 0x20;
   entry[hop].bytes7[2] = 0x84;

   entry[hop].bytes6[3] = 0x94;
   entry[hop].bytes7[3] = 0x64;

   entry[hop].bytes6[4] = 0xa0;
   entry[hop].bytes7[4] = 0x61;

   entry[hop].bytes6[5] = 0xa6;
   entry[hop].bytes7[5] = 0x64;

   entry[hop].bytes6[6] = 0xa8;
   entry[hop].bytes7[6] = 0x61;

   entry[hop].bytes6[7] = 0xa8;
   entry[hop].bytes7[7] = 0x51;

   entry[hop].bytes6[8] = 0x17;
   entry[hop].bytes7[8] = 0x85;


   //hop 2

   hop++;

   entry[hop].hop = hop;

   entry[hop].bytes6[0] = 0x20;
   entry[hop].bytes7[0] = 0x87;

   entry[hop].bytes6[1] = 0x99;
   entry[hop].bytes7[1] = 0x58;

   entry[hop].bytes6[2] = 0x28;
   entry[hop].bytes7[2] = 0x87;

   entry[hop].bytes6[3] = 0x35;
   entry[hop].bytes7[3] = 0x87;

   entry[hop].bytes6[4] = 0x89;
   entry[hop].bytes7[4] = 0x61;


   entry[hop].bytes6[5] = 0x16;
   entry[hop].bytes7[5] = 0x85;

   entry[hop].bytes6[6] = 0x89;
   entry[hop].bytes7[6] = 0x59;

   entry[hop].bytes6[7] = 0x86;
   entry[hop].bytes7[7] = 0x62;

   entry[hop].bytes6[8] = 0xa1;
   entry[hop].bytes7[8] = 0x50;



   //hop 3

   hop++;

   entry[hop].hop = hop;

   entry[hop].bytes6[0] = 0xa9;
   entry[hop].bytes7[0] = 0x64;

   entry[hop].bytes6[1] = 0x88;
   entry[hop].bytes7[1] = 0x59;

   entry[hop].bytes6[2] = 0xa3;
   entry[hop].bytes7[2] = 0x60;

   entry[hop].bytes6[3] = 0xb3;
   entry[hop].bytes7[3] = 0x62;

   entry[hop].bytes6[4] = 0xc0;
   entry[hop].bytes7[4] = 0x62;


   entry[hop].bytes6[5] = 0xb1;
   entry[hop].bytes7[5] = 0x59;

   entry[hop].bytes6[6] = 0x86;
   entry[hop].bytes7[6] = 0x61;

   entry[hop].bytes6[7] = 0xa0;
   entry[hop].bytes7[7] = 0x50;

   entry[hop].bytes6[8] = 0x15;
   entry[hop].bytes7[8] = 0x85;




   //hop 4

   hop++;

   entry[hop].hop = hop;

   entry[hop].bytes6[0] = 0x13;
   entry[hop].bytes7[0] = 0x85;

   entry[hop].bytes6[1] = 0x38;
   entry[hop].bytes7[1] = 0x85;

   entry[hop].bytes6[2] = 0x20;
   entry[hop].bytes7[2] = 0x85;

   entry[hop].bytes6[3] = 0x34;
   entry[hop].bytes7[3] = 0x86;

   entry[hop].bytes6[4] = 0x21;
   entry[hop].bytes7[4] = 0x87;

   entry[hop].bytes6[5] = 0xb5;
   entry[hop].bytes7[5] = 0x62;

   entry[hop].bytes6[6] = 0x25;
   entry[hop].bytes7[6] = 0x84;

   entry[hop].bytes6[7] = 0x18;
   entry[hop].bytes7[7] = 0x86;

   entry[hop].bytes6[8] = 0x29;
   entry[hop].bytes7[8] = 0x85;



   //hop 5

   hop++;

   entry[hop].hop = hop;

   entry[hop].bytes6[0] = 0x36;
   entry[hop].bytes7[0] = 0x87;

   entry[hop].bytes6[1] = 0xa5;
   entry[hop].bytes7[1] = 0x62;

   entry[hop].bytes6[2] = 0x88;
   entry[hop].bytes7[2] = 0x60;

   entry[hop].bytes6[3] = 0x23;
   entry[hop].bytes7[3] = 0x85;

   entry[hop].bytes6[4] = 0xc2;
   entry[hop].bytes7[4] = 0x64;


   entry[hop].bytes6[5] = 0x32;
   entry[hop].bytes7[5] = 0x85;

   entry[hop].bytes6[6] = 0x25;
   entry[hop].bytes7[6] = 0x86;

   entry[hop].bytes6[7] = 0x25;
   entry[hop].bytes7[7] = 0x85;

   entry[hop].bytes6[8] = 0x30;
   entry[hop].bytes7[8] = 0x87;


   isBuilt = TRUE;

}
