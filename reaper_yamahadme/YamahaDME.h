/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * YamahaDME.h
 * Copyright (C) 2011 Rui Barreiros <rbarreiros@gmail.com>
 * http://www.audioluz.net
 */

#include "reaper_plugin.h"

#ifndef __YAMAHA_DME_H__
#define __YAMAHA_DME_H__

#include "csurf.h"

#define MAX_UNSIGNED_INT_VALUE 4294967296
#define MAX_SIGNED_INT_VALUE 2147483648

typedef MIDI_event_t MidiEvt; // Shorter typing

/*
	YamahaDME

	Base class with code common to all desks and
	responsable for all the proper initialization.
*/
class YamahaDME : public IReaperControlSurface
{
public:
	enum SynchDirection {
		NONE,
		TOYAMAHA,
		TOREAPER
	};

	enum DeskType {
		UNDEF = 0x00,
		PM5D = 0x0F,
		M7CL = 0x11,
		LS9 = 0x12
	}; 

	YamahaDME(int inDev, int outDev, SynchDirection dir, int *errStats);
	virtual ~YamahaDME();
	
	// IReaperControlSurface Interface methods required to be implemented and common to all desks
	const char *GetTypeString();
	const char *GetDescString();
	const char *GetConfigString();
	void CloseNoReset();
	void Run();

	// This function is called by Run() to process midi messages
	// it is only called if midi messages are present, not periodically.
	virtual void onMidiEvent(MidiEvt *evt) = 0;

	// Not really needed, stays for future use if necessary, this is normal behaviour
	// as reaper will call all Set* functions that will send all the sysex to yamaha on startup
	virtual void synchToYamaha() {} 
	// This one is required to be implemented, will send all messages requesting parameter info to yamaha.
	virtual void synchToReaper() {}

	// DB Values might vary between consoles, each implementation might be diff
	virtual double getFaderYamahaToReaper(int volume) = 0;
	virtual int getFaderReaperToYamaha(double volume) = 0;

	// Methods called by reaper when parameters change from IReaperControlSurface
	virtual void SetTrackListChange() {}
	virtual void SetSurfaceVolume(MediaTrack *tr, double volume) {}
	virtual void SetSurfacePan(MediaTrack *tr, double pan) {}
	virtual void SetSurfaceMute(MediaTrack *tr, bool mute) {}
	virtual void SetSurfaceSelected(MediaTrack *tr, bool selected) {}
	virtual void SetSurfaceSolo(MediaTrack *tr, bool solo) {}
	virtual void SetSurfaceRecArm(MediaTrack *tr, bool recarm) {}
	virtual void SetPlayState(bool play, bool pause, bool rec) {}
	virtual void SetRepeatState(bool rep) {}
	virtual void SetTrackTitle(MediaTrack *tr, const char *title) {}
	virtual bool GetTouchState(MediaTrack *tr, int isPan) { return false; }
	virtual void SetAutoMode(int mode) {}
	virtual void ResetCachedVolPanStates() {}
	virtual void OnTrackSelection(MediaTrack *tr) {}
	virtual bool IsKeyDown(int key) { return false; }
	virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; }

	/*
		LS9/M7CL/PM5D have no controls for recording, like arm, play, stop, pause, loop, etc
		so were implementing them more or less, by mapping button presses with the select pressed
		for instance to record arm a track, press [SEL] + [CUE] for the channel you wish to arm.

		To see which channels are armed in the desk, press [SEL] and keep it pressed for more than
		3 seconds. This is why we required to move Run() into this class, so we can time funcions
		as Run() is ran all the time at a specific frequency regardless of midi data.

		This methods can be overriden.
	*/
	virtual void sendCurrentArmRecordSet();
	virtual void sendClearCurrentArmRecordSet();
	virtual void onArmRecord(MidiEvt *evt);




#ifdef _DEBUG
	// Utility debug function
	static void __cdecl Debug(const char *format, ...);
#endif

protected:
	// Send data to desks
	void sendToYamaha(unsigned int opcodeA, unsigned int opcodeB, int param, int channel, int data);
	void sendToYamahaRequest(unsigned int opcodeA, unsigned int opcodeB, int param, int channel);

	// Utility functions common to all desks, all self-explanatory
	int getPanReaperToYamaha(double pan);
	double getPanYamahaToReaper(int pan);
	int getMidiDataValue(MidiEvt *evt, bool isSigned = false);
	int getMidiTrackId(MidiEvt *evt);
	MediaTrack *getTrackFromId(int trackid);
	MediaTrack *getTrack(MidiEvt *evt);
	void clearAllSelectedTracks();

	// properties we need to keep track of
	bool m_SelPressed, m_initialized;
	int m_midiInDev, m_midiOutDev;
	midi_Input *m_midiInput;
	midi_Output *m_midiOutput;
	DeskType desk;
	SynchDirection m_synchDir;

	// Max channels we'll eventually handle will be 168 (PM5D)
	unsigned int m_cueSave[200];

	char configtmp[1024];
	WDL_String desc;
};

#endif