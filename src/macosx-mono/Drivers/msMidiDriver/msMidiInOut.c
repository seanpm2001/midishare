/*

  Copyright � Grame 2002

  This library is free software; you can redistribute it and modify it under 
  the terms of the GNU Library General Public License as published by the 
  Free Software Foundation version 2 of the License, or any later version.

  This library is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License 
  for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  Grame Research Laboratory, 9, rue du Garet 69001 Lyon - France
  grame@grame.fr
  
*/


#include "msMidiInOut.h"
#include "lffifo.h"
#include <mach/mach_time.h>

#define PRINT(x) { printf x; fflush(stdout); }
#define DBUG(x)    /* PRINT(x) */


extern short gRefNum;

// Buffer for sending

static int min(a,b) {return (a<b)?a:b;}
static void CompletionProc( MIDISysexSendRequest *request );

//____________________________________________________________________________________________________________
// Timing functions found in /Developer/Examples/CoreAudio/MIDI/SampleUSBDriver/Shared/MIDIDriverUtils.cpp

static bool		sIsInited = false;
static Float64	sNanoRatio;

//_________________________________________________________
static void InitHostTime()
{
	struct mach_timebase_info info;

	mach_timebase_info(&info);
	sNanoRatio = (double)info.numer / (double)info.denom;
	sIsInited = true;
}

//_________________________________________________________
MIDITimeStamp	MIDIGetCurrentHostTime()
{
	return mach_absolute_time();
}

//_________________________________________________________
static void LMM2MS (SlotPtr slot, MIDIPacket *packet)
{
	MidiEvPtr e; 
	long n = packet->length;
	unsigned char *ptr = packet->data;
	
	while (n--) {
		e = MidiParseByte (&slot->in, *ptr++);
		if (e) {
			Port(e) = (Byte)Slot(slot->refNum);
			MidiSendIm (gRefNum, e);
		}	
	}
}

//_________________________________________________________
static void SendSysExAux(SlotPtr slot)
{
	int i,bytestosend;
	unsigned char* ptr;
	OSErr err;

	// Prepare bytes
	bytestosend = min(MAX_SYSEX_BYTES,slot->remaining);
	slot->remaining-= bytestosend;
	ptr = slot->data;
	for (i = 0 ; i < bytestosend ; i++) MidiStreamGetByte (&slot->outsysex, ptr++);
 	
	// Init completion structure
	slot->request.destination = slot->dest;
	slot->request.data = slot->data;
	slot->request.bytesToSend = bytestosend;
	slot->request.complete = FALSE;
	slot->request.completionProc = CompletionProc;
	slot->request.completionRefCon = slot;
	
	err = MIDISendSysex( &slot->request);
	slot->sending = (err == noErr);
 }
 
//_________________________________________________________
static MidiEvPtr SendSysEx(SlotPtr slot,MidiEvPtr e)
{
	slot->remaining = (EvType(e) == typeSysEx) ? (MidiCountFields(e)+2) : MidiCountFields(e);
	
	// Write event to be sent
	e = MidiStreamPutEvent (&slot->outsysex, e);
	SendSysExAux(slot);
	return e;
}

//_________________________________________________________
static MidiEvPtr SendSmallEv(SlotPtr slot, MidiEvPtr e, sendState* state)
{ 
	MIDIPacketList* pktlist = (MIDIPacketList*)state->packetBuffer;
	MIDIPacket* packet = MIDIPacketListInit(pktlist);
	unsigned char * ptr = state->evBuffer;
	int  n = 0;
  	 
	e = MidiStreamPutEvent (&state->outsmall, e);
	while (MidiStreamGetByte (&state->outsmall, ptr++)) {n++;}
	
	MIDIPacketListAdd(pktlist,sizeof(state->packetBuffer),packet,MIDIGetCurrentHostTime(),n,state->evBuffer);
	MIDISend(slot->port,slot->dest,pktlist);
	
	return e;
 }

//_________________________________________________________
static void CompletionProc( MIDISysexSendRequest *request )
{
    SlotPtr slot =  (SlotPtr)request->completionRefCon;
    MidiEvPtr ev;
    
    if (slot->remaining == 0){
        slot->sending = FALSE;
        
        while ((ev = (MidiEvPtr)fifoget(&slot->pending))) {
            // If typeSysEx or typeStream : send one, pending events will be sent by the CompletionProc
            if ((EvType(ev) == typeSysEx) || (EvType(ev) == typeStream)) {
                    SendSysEx(slot,ev);
                    break;
            // Send all pending small events
            }else{ 
                    SendSmallEv(slot,ev,&slot->state2);
            }
        }
         
    } else {
        SendSysExAux(slot);
    }
}

//________________________________________________________________________________________
 MidiEvPtr MS2MM (SlotPtr slot, MidiEvPtr e)
{
	if ((EvType(e) >= typeClock) && (EvType(e) <= typeReset)){
		return SendSmallEv(slot,e,&slot->state1);
	}else if (slot->sending) {
		fifoput(&slot->pending,(cell*)e);
		return 0;
 	}else  if ((EvType(e) == typeSysEx) || (EvType(e) == typeStream)) {
		return SendSysEx(slot,e);
	}else {
		return SendSmallEv(slot,e,&slot->state1);
	}
}


//________________________________________________________________________________________
void ReadProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
	SlotPtr slot = (SlotPtr)connRefCon;
	MIDIPacket *packet = (MIDIPacket *)pktlist->packet;	
	int i;
	DBUG(("ReadProc \n"));
	for (i = 0; i < pktlist->numPackets; ++i) {
		LMM2MS( slot, packet);
		packet = MIDIPacketNext(packet);
	}
        DBUG(("ReadProc OK\n"));
}

