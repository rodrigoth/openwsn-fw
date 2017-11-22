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

   uint8_t sink_byte6 = 0xa3;
   uint8_t sink_byte7 = 0x79;

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

   entry[hop].bytes6[0] = 0x90;
   entry[hop].bytes7[0] = 0x76;

   entry[hop].bytes6[1] = 0xb3;
   entry[hop].bytes7[1] = 0x79;

   entry[hop].bytes6[2] = 0xc0;
   entry[hop].bytes7[2] = 0x81;

   entry[hop].bytes6[3] = 0x88;
   entry[hop].bytes7[3] = 0x75;

   entry[hop].bytes6[4] = 0xb2;
   entry[hop].bytes7[4] = 0x80;

   entry[hop].bytes6[5] = 0x84;
   entry[hop].bytes7[5] = 0x72;

   entry[hop].bytes6[6] = 0x95;
   entry[hop].bytes7[6] = 0x67;

   entry[hop].bytes6[7] = 0x91;
   entry[hop].bytes7[7] = 0x75;

   entry[hop].bytes6[8] = 0x00;
   entry[hop].bytes7[8] = 0x00;


   //hop 2

   hop++;

   entry[hop].hop = hop;

   entry[hop].bytes6[0] = 0xb1;
   entry[hop].bytes7[0] = 0x80;

   entry[hop].bytes6[1] = 0xc0;
   entry[hop].bytes7[1] = 0x68;

   entry[hop].bytes6[2] = 0xb2;
   entry[hop].bytes7[2] = 0x68;

   entry[hop].bytes6[3] = 0x93;
   entry[hop].bytes7[3] = 0x77;

   entry[hop].bytes6[4] = 0x92;
   entry[hop].bytes7[4] = 0x83;


   entry[hop].bytes6[5] = 0xb0;
   entry[hop].bytes7[5] = 0x68;

   entry[hop].bytes6[6] = 0x38;
   entry[hop].bytes7[6] = 0x61;

   entry[hop].bytes6[7] = 0x87;
   entry[hop].bytes7[7] = 0x79;

   entry[hop].bytes6[8] = 0x00;
   entry[hop].bytes7[8] = 0x00;



   //hop 3

   hop++;

   entry[hop].hop = hop;

   entry[hop].bytes6[0] = 0x90;
   entry[hop].bytes7[0] = 0x77;

   entry[hop].bytes6[1] = 0xa6;
   entry[hop].bytes7[1] = 0x78;

   entry[hop].bytes6[2] = 0x95;
   entry[hop].bytes7[2] = 0x79;

   entry[hop].bytes6[3] = 0x92;
   entry[hop].bytes7[3] = 0x69;

   entry[hop].bytes6[4] = 0xb8;
   entry[hop].bytes7[4] = 0x81;


   entry[hop].bytes6[5] = 0xa9;
   entry[hop].bytes7[5] = 0x82;

   entry[hop].bytes6[6] = 0xb3;
   entry[hop].bytes7[6] = 0x83;

   entry[hop].bytes6[7] = 0x95;
   entry[hop].bytes7[7] = 0x83;

   entry[hop].bytes6[8] = 0x00;
   entry[hop].bytes7[8] = 0x00;




   //hop 4

   hop++;

   entry[hop].hop = hop;

   entry[hop].bytes6[0] = 0x18;
   entry[hop].bytes7[0] = 0x60;

   entry[hop].bytes6[1] = 0xa4;
   entry[hop].bytes7[1] = 0x78;

   entry[hop].bytes6[2] = 0x94;
   entry[hop].bytes7[2] = 0x83;

   entry[hop].bytes6[3] = 0xa4;
   entry[hop].bytes7[3] = 0x81;

   entry[hop].bytes6[4] = 0x84;
   entry[hop].bytes7[4] = 0x70;

   entry[hop].bytes6[5] = 0x95;
   entry[hop].bytes7[5] = 0x75;

   entry[hop].bytes6[6] = 0xa4;
   entry[hop].bytes7[6] = 0x70;

   entry[hop].bytes6[7] = 0xb1;
   entry[hop].bytes7[7] = 0x82;

   entry[hop].bytes6[8] = 0x00;
   entry[hop].bytes7[8] = 0x00;



   //hop 5

   hop++;

   entry[hop].hop = hop;

   entry[hop].bytes6[0] = 0xa9;
   entry[hop].bytes7[0] = 0x68;

   entry[hop].bytes6[1] = 0xb3;
   entry[hop].bytes7[1] = 0x81;

   entry[hop].bytes6[2] = 0xa5;
   entry[hop].bytes7[2] = 0x82;

   entry[hop].bytes6[3] = 0xa2;
   entry[hop].bytes7[3] = 0x83;

   entry[hop].bytes6[4] = 0xb7;
   entry[hop].bytes7[4] = 0x81;


   entry[hop].bytes6[5] = 0x98;
   entry[hop].bytes7[5] = 0x75;

   entry[hop].bytes6[6] = 0xa5;
   entry[hop].bytes7[6] = 0x83;

   entry[hop].bytes6[7] = 0xb9;
   entry[hop].bytes7[7] = 0x82;

   entry[hop].bytes6[8] = 0x00;
   entry[hop].bytes7[8] = 0x00;


   isBuilt = TRUE;

}
