#include "LS9.h"

typedef bool (LS9::*MidiHandlerFunc)(MIDI_event_t*);

struct EventHandler
{
	unsigned int opcodeA;
	unsigned int opcodeB;
	MidiHandlerFunc func;
};

// Our event tables
static const int nHandlers = 5;
static const EventHandler events[nHandlers] = {
	{ 0x00, 0x31, &LS9::OnInputOnChange },
	{ 0x00, 0x32, &LS9::OnInputPanChange },
	{ 0x00, 0x33, &LS9::OnInputFaderChange },
	{ 0x01, 0x5e, &LS9::OnInputCueChange },
	{ 0x02, 0x39, &LS9::OnChannelSelected },
	//{ 0x02, 0x41, &LS9::OnChannelSelectPush }, receives 2 events, on sel key press and on sel key release
};

double LS9::getFaderYamahaToReaper(int volume)
{
	return DB2VAL(LS9_val2db[volume]);
}

/*******************************************
	Yamaha to Reaper
********************************************/

void LS9::onMidiEvent(MidiEvt *evt)
{
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
	//m_SelPressed = (getMidiDataValue(evt) == 1);
	MediaTrack *tr = getTrackFromId((getMidiDataValue(evt) + 1));
	if(tr)
	{
		clearAllSelectedTracks();
		CSurf_OnSelectedChange(tr, 1);
	} else
		return false;

	return true;
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

