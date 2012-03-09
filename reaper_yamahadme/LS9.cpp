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
 * LS9.cpp
 * Copyright (C) 2011 Rui Barreiros <rbarreiros@gmail.com>
 * http://www.audioluz.net
 */

#include "LS9.h"

typedef bool (LS9::*MidiHandlerFunc)(MIDI_event_t*);

struct EventHandler
{
	unsigned int opcodeA;
	unsigned int opcodeB;
	unsigned int param;
	MidiHandlerFunc func;
};

// Our event tables
static const int nHandlers = 6;
static const EventHandler events[nHandlers] = {
	{ 0x00, 0x31, 0x00, &LS9::OnInputOnChange },
	{ 0x00, 0x32, 0x01, &LS9::OnInputPanChange },
	{ 0x00, 0x33, 0x00, &LS9::OnInputFaderChange },
	{ 0x01, 0x5e, 0x00, &LS9::OnInputCueChange },
	{ 0x02, 0x39, 0x10, &LS9::OnChannelSelected },
	{ 0x02, 0x41, 0x00, &LS9::OnChannelSelectPush }, //receives 2 events, on sel key press and on sel key release
};

double LS9::getFaderYamahaToReaper(int volume)
{
	return DB2VAL(LS9_val2db[volume]);
}

int LS9::getFaderReaperToYamaha(double volume)
{
	float db = (float)(VAL2DB(volume));
	int lastLowestVal = 0;

	// round it to 2 decimal places
	db = floorf(db * 100 + (float)0.5) / 100;

	for(int i = 0; i < LS9_nVolumeValues; i++)
	{
		if(LS9_val2db[i] == db)
			return i;
		else if(LS9_val2db[i] < db) // closest lowest value if we don't get a match
			lastLowestVal = i;
	}

	return lastLowestVal;
}

/*******************************************
	Yamaha to Reaper
********************************************/

void LS9::onMidiEvent(MidiEvt *evt)
{
	if(m_initialized == false)
	{
		if(m_synchDir == YamahaDME::TOREAPER)
		{
			OutputDebugString("Calling synchToReaper()\n");
			synchToReaper();
		}
		else if(m_synchDir == YamahaDME::TOYAMAHA) // shouldn't really need this at all, stays if it's needed in future
		{
			OutputDebugString("Calling synchToYamaha()\n");
			synchToYamaha();
		}
		m_initialized = true;
	}

	if(evt->size != 18) return; // we don't handle requests, only info
	if(evt->midi_message[0] != 0xf0 || evt->midi_message[17] != 0xf7) return; // invalid info packet

	unsigned int opcodeA = evt->midi_message[6];
	unsigned int opcodeB = evt->midi_message[7];

	for(int i = 0; i < nHandlers; i++)
	{
		EventHandler handler = events[i];

		if(opcodeA == handler.opcodeA && opcodeB == handler.opcodeB && handler.func != NULL)
		{
			(this->*handler.func)(evt);
		}
	}
}

bool LS9::OnInputOnChange(MidiEvt *evt)
{
	MediaTrack *tr = getTrack(evt);
	if(tr) 
	{
		int mute = (getMidiDataValue(evt) == 0);
		CSurf_SetSurfaceMute(tr, CSurf_OnMuteChange(tr, mute), NULL);
	} else
		return false;

	return true;
}

bool LS9::OnInputPanChange(MidiEvt *evt)
{
	MediaTrack *tr = getTrack(evt);
	if(tr)
	{
		CSurf_SetSurfacePan(tr, CSurf_OnPanChange(tr, getPanYamahaToReaper((getMidiDataValue(evt, true))), false), NULL);
	} else
		return false;

	return true;
}

bool LS9::OnInputFaderChange(MidiEvt *evt)
{
	MediaTrack *tr = getTrack(evt);
	if(tr)
	{
		int value = getMidiDataValue(evt);
		if(value < 0) value = 0;
		if(value > 1023) value = 1023;

		CSurf_SetSurfaceVolume(tr, CSurf_OnVolumeChange(tr, getFaderYamahaToReaper(value), false), NULL);
	} else 
		return false;

	return true;
}

bool LS9::OnInputCueChange(MidiEvt *evt)
{
	MediaTrack *tr = getTrack(evt);
	if(tr)
	{
		clearAllSelectedTracks();
		CSurf_OnSelectedChange(tr, 1);
		if(getMidiDataValue(evt) > 0)
			SendMessage(g_hwnd,WM_COMMAND, 40493,0);
		else
			SendMessage(g_hwnd,WM_COMMAND, 40492,0);
	} else
		return false;

	return true;
}

bool LS9::OnChannelSelected(MidiEvt *evt)
{
	MediaTrack *tr = getTrackFromId((getMidiDataValue(evt) + 1));
	if(tr)
	{
		clearAllSelectedTracks();
		CSurf_OnSelectedChange(tr, 1);
	} else
		return false;

	return true;
}

bool LS9::OnChannelSelectPush(MidiEvt *evt)
{
	m_SelPressed = (getMidiDataValue(evt) == 0x01);
	return true;
}

/*******************************************
	Synch
********************************************/

/**
	synchToReaper

	To synch the desk to reaper, we request all the defined callback parameters
	we have and then our plugin will parse them as if they were sent normally by
	someone changing values on the desk.
*/
void LS9::synchToReaper()
{
	OutputDebugString("Synch to Reaper called");

	for(int i = 0; i < nHandlers; i++)
	{
		EventHandler handler = events[i];

		// How many channels we have setup
		int nChannels = CSurf_NumTracks(MCP_MODE);
		char b[100]; 
		sprintf(b, "NumChannels: %d\n", nChannels);
		OutputDebugString(b);
		// Should we be sure the channel exists ?!?!? (TODO)
		for(int c = 0; c < nChannels; c++)
		{
			sendToYamahaRequest(handler.opcodeA, handler.opcodeB, handler.param, c);
		}
	}
}

/*******************************************
	Reaper to Yamaha
********************************************/

/*
	kInputOn
	opcodeA 0x00, opcodeB 0x31
	params:
		kChannelOn: 0x00	min: 0		max: 1		default: 1

	This is a channel on/off in Yamaha thus it's negated into reaper, working as mute,
	channel on = mute off, channel off = mute on
*/

void LS9::SetSurfaceMute(MediaTrack *tr, bool mute)
{
	sendToYamaha(0x00, 0x31, 0x00, (CSurf_TrackToID(tr, MCP_MODE) - 1), (mute) ? 0x00 : 0x01);
}

/*
	kInputPan
	opcodeA 0x00, opcodeB 0x32
	params:
		kPanMode: 0x00		min: 0		max: 3		default: 0
		kChannelPan: 0x01	min: -63	max: 63		default: 0
*/
void LS9::SetSurfacePan(MediaTrack *tr, double pan)
{
	sendToYamaha(0x00, 0x32, 0x01, (CSurf_TrackToID(tr, MCP_MODE) - 1), getPanReaperToYamaha(pan));
}

/*
	This is undocumented on yamaha parameters list

	Since yamaha can't have multiple channels selected, we only send anything when selected is true
	probably we should enforce 1 track only selected programatically on reaper?? (TODO)

	->SND: F0 43 10 3E 12 01 02 39 00 10 00 00 00 00 00 00 02 F7

*/

void LS9::SetSurfaceSelected(MediaTrack *tr, bool selected)
{
	if(selected)
	{
		int track = CSurf_TrackToID(tr, MCP_MODE);
		sendToYamaha(0x02, 0x39, 0x10, 0x00, --track);
	}
}

void LS9::SetSurfaceVolume(MediaTrack *tr, double volume)
{
	sendToYamaha(0x00, 0x33, 0x00, (CSurf_TrackToID(tr, MCP_MODE) - 1), getFaderReaperToYamaha(volume));
}
