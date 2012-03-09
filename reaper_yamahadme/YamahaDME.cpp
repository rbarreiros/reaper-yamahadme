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
 * YamahaDME.cpp
 * Copyright (C) 2011 Rui Barreiros <rbarreiros@gmail.com>
 * http://www.audioluz.net
 */

#include "YamahaDME.h"

/*******************************************
	Construction, destruction, finalization
********************************************/

/**
	Class Constructor

	@Param[in]	indev		ID of the input midi device
	@Param[in]	outdev		ID of the output midi device
	@Param[in]	dir			SynchDirection from to where to synch on startup
	@Param[out]	errStats	Errors
*/
YamahaDME::YamahaDME(int inDev, int outDev, SynchDirection dir, int *errStats) : 
		m_SelPressed(false), desk(YamahaDME::UNDEF), m_synchDir(dir), m_initialized(false) 
{ 
	m_midiInDev = inDev;
	m_midiOutDev = outDev;

	m_midiInput = m_midiInDev >= 0 ? CreateMIDIInput(m_midiInDev) : NULL;
	m_midiOutput = m_midiOutDev >= 0 ? CreateThreadedMIDIOutput(CreateMIDIOutput(m_midiOutDev, false, NULL)) : NULL;

	if (errStats)
	{
		if (m_midiInDev >=0  && !m_midiInput) *errStats|=1;
		if (m_midiOutDev >=0  && !m_midiOutput) *errStats|=2;
	}

	if (m_midiInput)
		m_midiInput->start();

#ifdef _DEBUG
	if(m_synchDir == YamahaDME::TOREAPER)
		Debug("Initialized with Synch to Reaper\n");
	else if(m_synchDir == YamahaDME::TOYAMAHA)
		Debug("Initialized with Synch to Yamaha\n");
	else if(m_synchDir == YamahaDME::NONE)
		Debug("Initialized with No synch\n");
#endif

}

/**
	Destructor
*/
YamahaDME::~YamahaDME()
{
	if(m_midiOutput) delete m_midiOutput;
	if(m_midiInput) delete m_midiInput;
}

/**
	CloseNoReset

	Close without sending "reset" messages, prevent "reset" being sent on destructor
*/
void YamahaDME::CloseNoReset()
{
	if(m_midiOutput) delete m_midiOutput;
	if(m_midiInput) delete m_midiInput;
	m_midiInput = 0;
	m_midiOutput = 0;
}

/*******************************************
	IReaperControlSurface Interface methods required to be implemented and common to all desks
********************************************/

const char *YamahaDME::GetTypeString() 
{ 
	return "Yamaha DME-Network"; 
}

/**
	GetDescString

	@Return	Description string that will be presented on reaper list of running control surfaces.
*/
const char *YamahaDME::GetDescString()
{
	char tmp[150], indev[10], outdev[10];
	
	if(!GetMIDIInputName(m_midiInDev, indev, sizeof(indev)))
		sprintf(indev, "Err: Dev not found");

	if(!GetMIDIOutputName(m_midiOutDev, outdev, sizeof(outdev)))
		sprintf(outdev, "Err: Dev not found");

	if(m_midiInput && m_midiOutput)
		sprintf(tmp, "Yamaha DME Network (In: %s, Out: %s)", indev, outdev);
	else
		sprintf(tmp, "Yamaha DME Network (In: Err - Not connected, Out: Err - Not connected)");

	desc.Set(tmp, sizeof(tmp));
	return desc.Get();     
}

/**
	GetConfigString

	@Return	String containing the initial configuration parameters
*/
const char *YamahaDME::GetConfigString()
{
	sprintf(configtmp,"%d %d %d", m_midiInDev, m_midiOutDev, m_synchDir);
	return configtmp;
}

/**
	Run

	Called about 30 times a second
*/
void YamahaDME::Run()
{
	if (m_midiInput)
	{
		m_midiInput->SwapBufs(timeGetTime());
		int l=0;
		MIDI_eventlist *list=m_midiInput->GetReadBuf();
		MIDI_event_t *evts;
		while ((evts=list->EnumItems(&l))) onMidiEvent(evts);
	}
}

/*******************************************
	Utility functions	
********************************************/

/**
	getPanReaperToYamaha

	Converts reaper pan value to yamaha pan value

	@Param[in]	pan		Reaper pan value
	@return				Yamaha pan value
*/
int YamahaDME::getPanReaperToYamaha(double pan)
{
	return (int)(pan * 63.0);
}

/**
	getPanYamahaToReaper

	Converts yamaha pan value to reaper pan value

	@Param[in]	pan		Yamaha pan value
	@Return				Reaper pan value
*/
double YamahaDME::getPanYamahaToReaper(int pan)
{
	return pan / 63.0;
}

/**
	getMidiDataValue

	Returns an int properly converted from Yamaha MIDI
	data format (4 * 7bit + 4bit)

	@Param[in]	evt			Midi Event struct
	@Param[in]	isSigned	can the value have negative values ?
	@Return					Returns an int32 with the data value from the midi event
*/

int YamahaDME::getMidiDataValue(MidiEvt *evt, bool isSigned)
{
	double value = evt->midi_message[16] | (evt->midi_message[15] << 7) | (evt->midi_message[14] << 14) 
		| (evt->midi_message[13] << 21) | (evt->midi_message[12] << 28);

	if(isSigned)
		if (value >= MAX_SIGNED_INT_VALUE) value -= MAX_UNSIGNED_INT_VALUE;

	return (int)value;
}

/**
	getMidiTrackId

	Returns an int with the track number presented in the midi event
	from yamaha midi message.
	The message is a 2 * 7bit = 14bit

	@Param[in]	evt		Midi event message
	@Return				Returns an int32 containing the track number in the midi message
*/
int YamahaDME::getMidiTrackId(MidiEvt *evt)
{
	return ((evt->midi_message[10] << 7) | evt->midi_message[11])+1;
}

/**
	getTrackFromId

	Returns a MediaTrack object from a track id

	@Param[in]	trackid		number of track
	@Return					MediaTrack object
*/
MediaTrack *YamahaDME::getTrackFromId(int trackid)
{
	return CSurf_TrackFromID(trackid, MCP_MODE);
}

/**
	getTrack

	Returns a MediaTrack from the Midi event message

	@Param[in]	evt		Midi event message
	@Return				MediaTrack object
*/
MediaTrack *YamahaDME::getTrack(MidiEvt *evt)
{
	return CSurf_TrackFromID(getMidiTrackId(evt), MCP_MODE);
}

/**
	clearAllSelectedTracks

	Unselects all reaper tracks
*/
void YamahaDME::clearAllSelectedTracks()
{
	SendMessage(g_hwnd,WM_COMMAND, 40297,0);
}

/*
	SendToYamaha

	Sends a sysex message to Yamaha desks

	Yamaha Sysex format:
	F0 43 10 3E 12 01 oa ob pp pp cc cc dd dd dd dd dd F7

	F0 = SOX
	43 = YAMAHA
	1n = Prm Chg (n=0-15)
	3E = Digital Mixer
	12 = Mixer (12 = LS9-16/32, 11 = MONACO (M7CL), 0F = PM5D)
	01 = Parameter request
	oa = opcode A  (7bit)
	ob = opcode B  (7bit)
	pp = parameter (14bit)
	cc = channel   (14bit)
	dd = data      (32bit)
	F7 = EOX

	@Param[in]	opcodeA		Check parameter list table for opcodes
	@Param[in]	opcodeB		Check parameter list table for opcodes
	@Param[in]	param		For each function it can have many parameters
	@Param[in]	channel		Channel ID for this message (starts at channel 0)
	@Param[in]	data		The value to send for this parameter
*/

void YamahaDME::sendToYamaha(unsigned int opcodeA, unsigned int opcodeB, int param, int channel, int data)
{
	if(!m_midiOutput)
		return;

	if(desk == YamahaDME::UNDEF)
		return;

	/* 
		If the synch is anything different than Reaper -> Yamaha
		we won't allow sending data to prevent sending data to yamaha
		and override it's current values at system startup when it is not initialized.
		Initialization should me marked true when onMidiEvent is called
	*/
	if( m_synchDir != YamahaDME::TOYAMAHA && !m_initialized )
		return;

	struct
	{
		MIDI_event_t evt;
		unsigned char data[14];
	} yam;

	yam.evt.frame_offset = 0;
	yam.evt.size = 0;

	yam.evt.midi_message[yam.evt.size++] = 0xf0;
	yam.evt.midi_message[yam.evt.size++] = 0x43;
	yam.evt.midi_message[yam.evt.size++] = 0x10;
	yam.evt.midi_message[yam.evt.size++] = 0x3e;
	yam.evt.midi_message[yam.evt.size++] = desk;
	yam.evt.midi_message[yam.evt.size++] = 0x01;
	yam.evt.midi_message[yam.evt.size++] = opcodeA;
	yam.evt.midi_message[yam.evt.size++] = opcodeB;
	yam.evt.midi_message[yam.evt.size++] = (param >> 7) & 0x7f;
	yam.evt.midi_message[yam.evt.size++] = param & 0x7f;
	yam.evt.midi_message[yam.evt.size++] = (channel >> 7) & 0x7f;
	yam.evt.midi_message[yam.evt.size++] = channel & 0x7f;
	yam.evt.midi_message[yam.evt.size++] = (data >> 28) & 0x7f;
	yam.evt.midi_message[yam.evt.size++] = (data >> 21) & 0x7f;
	yam.evt.midi_message[yam.evt.size++] = (data >> 14) & 0x7f;
	yam.evt.midi_message[yam.evt.size++] = (data >> 7) & 0x7f;
	yam.evt.midi_message[yam.evt.size++] = data & 0x7f;
	yam.evt.midi_message[yam.evt.size++] = 0xf7;

	m_midiOutput->SendMsg(&yam.evt, -1);

#ifdef _DEBUG
	Debug("DATA: 0x%02x,0x%02x,0x%02x,%d,%d - 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
		opcodeA, opcodeB, param, channel, data,
		yam.evt.midi_message[0],
		yam.evt.midi_message[1],
		yam.evt.midi_message[2],
		yam.evt.midi_message[3],
		yam.evt.midi_message[4],
		yam.evt.midi_message[5],
		yam.evt.midi_message[6],
		yam.evt.midi_message[7],
		yam.evt.midi_message[8],
		yam.evt.midi_message[9],
		yam.evt.midi_message[10],
		yam.evt.midi_message[11],
		yam.evt.midi_message[12],
		yam.evt.midi_message[13],
		yam.evt.midi_message[14],
		yam.evt.midi_message[15],
		yam.evt.midi_message[16],
		yam.evt.midi_message[17]);
#endif
}

/*
	sendToYamahaRequest

	Sends a sysex to yamaha desks requesting a parameter value

	Yamaha Sysex request format:
	F0 43 30 3E 12 01 oa ob pp pp cc cc F7

	F0 = SOX
	43 = YAMAHA
	3n = Prm Chg (n=0-15)
	3E = Digital Mixer
	12 = Mixer (12 = LS9-16/32, 11 = MONACO (M7CL), 0F = PM5D)
	01 = Parameter request
	oa = opcode A  (7bit)
	ob = opcode B  (7bit)
	pp = parameter (14bit)
	cc = channel   (14bit)
	F7 = EOX

	@Param[in]	opcodeA		Check parameter list table for opcodes
	@Param[in]	opcodeB		Check parameter list table for opcodes
	@Param[in]	param		For each function it can have many parameters
	@Param[in]	channel		Channel ID for this message (starts at channel 0)
*/
void YamahaDME::sendToYamahaRequest(unsigned int opcodeA, unsigned int opcodeB, int param, int channel)
{
	if(!m_midiOutput)
		return;

	if(desk == YamahaDME::UNDEF)
		return;

	struct
	{
		MIDI_event_t evt;
		unsigned char data[9];
	} yam;

	yam.evt.frame_offset = 0;
	yam.evt.size = 0;

	yam.evt.midi_message[yam.evt.size++] = 0xf0;
	yam.evt.midi_message[yam.evt.size++] = 0x43;
	yam.evt.midi_message[yam.evt.size++] = 0x10;
	yam.evt.midi_message[yam.evt.size++] = 0x3e;
	yam.evt.midi_message[yam.evt.size++] = desk;
	yam.evt.midi_message[yam.evt.size++] = 0x01;
	yam.evt.midi_message[yam.evt.size++] = opcodeA;
	yam.evt.midi_message[yam.evt.size++] = opcodeB;
	yam.evt.midi_message[yam.evt.size++] = (param >> 7) & 0x7f;
	yam.evt.midi_message[yam.evt.size++] = param & 0x7f;
	yam.evt.midi_message[yam.evt.size++] = (channel >> 7) & 0x7f;
	yam.evt.midi_message[yam.evt.size++] = channel & 0x7f;
	yam.evt.midi_message[yam.evt.size++] = 0xf7;

	m_midiOutput->SendMsg(&yam.evt, -1);

#ifdef _DEBUG
	Debug("REQUEST: 0x%02x,0x%02x,0x%02x,%d - 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
		opcodeA, opcodeB, param, channel,
		yam.evt.midi_message[0],
		yam.evt.midi_message[1],
		yam.evt.midi_message[2],
		yam.evt.midi_message[3],
		yam.evt.midi_message[4],
		yam.evt.midi_message[5],
		yam.evt.midi_message[6],
		yam.evt.midi_message[7],
		yam.evt.midi_message[8],
		yam.evt.midi_message[9],
		yam.evt.midi_message[10],
		yam.evt.midi_message[11],
		yam.evt.midi_message[12]);
#endif
}

/**
	Debug

	Ref: http://www.unixwiz.net/techtips/outputdebugstring.html
*/
#ifdef _DEBUG
void __cdecl YamahaDME::Debug(const char *format, ...)
{
	char    buf[4096], *p = buf;
	va_list args;
	int     n;

	va_start(args, format);
	n = _vsnprintf(p, sizeof buf - 3, format, args); // buf-3 is room for CR/LF/NUL
	va_end(args);

	p += (n < 0) ? sizeof buf - 3 : n;

	while ( p > buf  &&  isspace(p[-1]) )
		*--p = '\0';

	*p++ = '\r';
	*p++ = '\n';
	*p   = '\0';

	OutputDebugString(buf);
}
#endif

/**
	sendCurrentArmRecordSet

	Sends all currently armed tracks as CUE on message to the desk
	only if the [SEL] key is pressed for more than ~3 seconds

	We have the current cue list saved to be returned to the desk when
	the [SEL] key is 'unpressed'

	TODO - NOT IMPLEMETED
*/
void YamahaDME::sendCurrentArmRecordSet()
{
}

/**
	sendClearCurrentArmRecordSet

	Clears the cues we sent to the desk and return the 
	cues we had before we pressed [SEL]

	TODO - NOT IMPLEMETED
*/
void YamahaDME::sendClearCurrentArmRecordSet()
{
	// we need to find the clear cue opcode and send that instead
	// no idea if all the desks have that, certain LS9 has
}

/**
	onArmRecord

	When [SEL] + [CUE] is pressed we arm the track record.

	TODO - NOT IMPLEMETED
*/
void YamahaDME::onArmRecord(MidiEvt *evt)
{
	if(!m_SelPressed)
		return;

}



