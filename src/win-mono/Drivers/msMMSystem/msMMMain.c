/*

  Copyright � Grame, Sony CSL-Paris 2001

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

#include <windows.h>

#include "MidiShare.h"
#include "msMMDriver.h"
#include "msMMState.h"
#include "FilterUtils.h"
#include "EventToMidiStream.h"
#include "MidiStreamToEvent.h"

static void Start();
static void Stop();

enum { false, true };
//_________________________________________________________
BOOL WINAPI DllEntryPoint (HINSTANCE  hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	if (MidiGetVersion() < 184) return FALSE;
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			Start();
			break;
		case DLL_PROCESS_DETACH:
			Stop();
			break;
	 }
    return TRUE;
}

static void MSALARMAPI msWakeup (short refnum);
static void MSALARMAPI msSleep  (short refnum);

short gRefNum = 0;
TDriverInfos drvInfos   = { "MMSystem Driver", kMMDriverVersion, 0 };
TDriverOperation drvOps = { msWakeup, msSleep };
TFilter	gFilter;
LinearizeMthTbl gLinMethods;
ParseMethodTbl	gParseTbl;
Status2TypeTbl	gTypeTbl;

//_________________________________________________________
static void Start()
{
//	std::setlocale(LC_ALL, "en_US.utf8");
//	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	gRefNum = MidiRegisterDriver(&drvInfos, &drvOps);
}

//_________________________________________________________
static void Stop()
{
	if (gRefNum > 0) MidiUnregisterDriver (gRefNum);
}

//_________________________________________________________
static void SetupFilter (MidiFilterPtr filter)
{
	short i;
	for (i = 0; i<256; i++) {                                                                               
		RejectBit(filter->evType,i);
		AcceptBit(filter->port,i);
	}
	for (i = 0; i<16; i++)
		AcceptBit(filter->channel,i);               
	for (i = 0; i<typeProcess; i++)
		AcceptBit(filter->evType,i);      
	for (i = typeQuarterFrame; i<=typeRegParam; i++)
		AcceptBit(filter->evType,i);
}

/*____________________________________________________________________*/
static void print(MidiEvPtr e)
{
	MMTrace("date: ", Date(e));
	MMTrace("ev	type: ", (long)EvType(e));
	MMTrace("	chan: ", (long)Chan(e));
	MMTrace("	data 0: ", (long)Data(e)[0]);
	MMTrace("	data 1: ", (long)Data(e)[1]);
}

//_________________________________________________________
static void CALLBACK msWakeup (short refnum)
{
	MidiSetRcvAlarm(refnum, RcvAlarm);
	MidiSetApplAlarm (refnum, ApplAlarm);
    SetupFilter (&gFilter);
    MidiSetFilter (refnum, &gFilter);	
	MidiConnect (MidiShareDrvRef, refnum, true);
	MidiConnect (refnum, MidiShareDrvRef, true);
	MidiStreamInitMthTbl (gLinMethods);
	MidiParseInitMthTbl	 (gParseTbl);
	MidiParseInitTypeTbl (gTypeTbl);
	AddSlots (refnum);
	LoadState(gInSlots, gOutSlots);
	/*
	{
		StreamFifo f; MidiEvPtr e;
		MidiParseInit (&f, gParseTbl, gTypeTbl);
		e = MidiParseByte (&f, 177);		// ctrl change channel 1
//		if (e) printf ("got unexpected event !\n");
		e = MidiParseByte (&f, 7);			// volume
//		if (e) printf ("got unexpected event !\n");
		e = MidiParseByte (&f, 120);		// value
		if (!e) printf ("no event received !\n");
		else print(e);
	}
*/
}

static void CALLBACK msSleep (short refnum)
{
	SaveState (gInSlots, gOutSlots);
	RemoveSlots (refnum);
 }

// intended only to avoid link error
main() { return 0; }
