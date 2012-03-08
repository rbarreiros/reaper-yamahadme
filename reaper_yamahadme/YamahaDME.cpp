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

int YamahaDME::getPanReaperToYamaha(double pan)
{
	return (int)(pan * 63.0);
}

int YamahaDME::getFaderReaperToYamaha(double volume)
{
	// TODO
	return 0;
}

double YamahaDME::getPanYamahaToReaper(int pan)
{
	return pan / 63.0;
}

int YamahaDME::getMidiDataValue(MidiEvt *evt, bool isSigned)
{
	double value = evt->midi_message[16] | (evt->midi_message[15] << 7) | (evt->midi_message[14] << 14) 
		| (evt->midi_message[13] << 21) | (evt->midi_message[12] << 28);

	if(isSigned)
		if (value >= MAX_SIGNED_INT_VALUE) value -= MAX_UNSIGNED_INT_VALUE;

	return (int)value;
}

int YamahaDME::getMidiTrackId(MidiEvt *evt)
{
	return ((evt->midi_message[10] << 7) | evt->midi_message[11])+1;
}

MediaTrack *YamahaDME::getTrackFromId(int trackid)
{
	return CSurf_TrackFromID(trackid, MCP_MODE);
}

MediaTrack *YamahaDME::getTrack(MidiEvt *evt)
{
	return CSurf_TrackFromID(getMidiTrackId(evt), MCP_MODE);
}

void YamahaDME::clearAllSelectedTracks()
{
	SendMessage(g_hwnd,WM_COMMAND, 40297,0);
}

/*
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
*/

void YamahaDME::sendToYamaha(unsigned int opcodeA, unsigned int opcodeB, int param, int channel, int data)
{
	if(!m_midiOutput)
		return;

	if(desk == YamahaDME::UNDEF)
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

	char buf[512];
	sprintf(buf, "DATA: 0x%02x,0x%02x,0x%02x,%d,%d - 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
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
	OutputDebugString(buf);
}