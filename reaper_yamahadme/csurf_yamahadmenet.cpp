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
 * csurf_yamahadmenet.cpp
 * Copyright (C) 2011 Rui Barreiros <rbarreiros@gmail.com>
 * http://www.audioluz.net
 */

#include "csurf_yamahadmenet.h"
#include "LS9.h"

/**
	Class Constructor

	@Param[in]	indev		ID of the input midi device
	@Param[in]	outdev		ID of the output midi device
	@Param[out]	errStats	Errors
*/

CSurf_YamahaDMENet::CSurf_YamahaDMENet(int indev, int outdev, SynchDirection dir, int *errStats) : m_synchDir(dir)
{
	m_midi_in_dev=indev;
	m_midi_out_dev=outdev;

	//create midi hardware access
	m_midiin = m_midi_in_dev >= 0 ? CreateMIDIInput(m_midi_in_dev) : NULL;
	m_midiout = m_midi_out_dev >= 0 ? CreateThreadedMIDIOutput(CreateMIDIOutput(m_midi_out_dev,false,NULL)) : NULL;

	if (errStats)
	{
		if (m_midi_in_dev >=0  && !m_midiin) *errStats|=1;
		if (m_midi_out_dev >=0  && !m_midiout) *errStats|=2;
	}

	if (m_midiin)
		m_midiin->start();

	// Detect which desk is connected (LS9/M7CL/PM5D) 
	// and initialize it
	char name[10];
	GetMIDIInputName(m_midi_in_dev, name, sizeof(name));
	if(_strcmpi(name, "ls9") == 0)
		m_Yamaha = new LS9(m_midiin, m_midiout);
	//else if(_strcmpi(name, "m7cl") == 0) // Init M7CL
	//else if(_strcmpi(name, "pm5d") == 0) // Init PM5D
	//else // we should send an error message and bail out, unknown console

	char buf[100];
	if(m_synchDir == CSurf_YamahaDMENet::TOREAPER)
		sprintf(buf, "Initialized with Synch to Reaper\n");
	else if(m_synchDir == CSurf_YamahaDMENet::TOYAMAHA)
		sprintf(buf, "Initialized with Synch to Yamaha\n");
	else if(m_synchDir == CSurf_YamahaDMENet::NONE)
		sprintf(buf, "Initialized with No synch\n");
	OutputDebugString(buf);
}

/**
	Class Destructor
*/
CSurf_YamahaDMENet::~CSurf_YamahaDMENet() 
{
	if(m_Yamaha) delete m_Yamaha;
	if(m_midiout) delete m_midiout;
	if(m_midiin) delete m_midiin;
}

/**
	GetDescString

	@Return	Description string that will be presented on reaper list of running control surfaces.
*/
const char *CSurf_YamahaDMENet::GetDescString()
{
	char tmp[150], indev[10], outdev[10];
	
	if(!GetMIDIInputName(m_midi_in_dev, indev, sizeof(indev)))
		sprintf(indev, "Err: Dev not found");

	if(!GetMIDIOutputName(m_midi_out_dev, outdev, sizeof(outdev)))
		sprintf(outdev, "Err: Dev not found");

	if(m_midiin && m_midiout)
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
const char *CSurf_YamahaDMENet::GetConfigString()
{
	sprintf(configtmp,"%d %d %d", m_midi_in_dev, m_midi_out_dev, m_synchDir);
	return configtmp;
}

/**
	CloseNoReset

	Close without sending "reset" messages, prevent "reset" being sent on destructor
*/
void CSurf_YamahaDMENet::CloseNoReset()
{
	if(m_Yamaha) delete m_Yamaha;
	if(m_midiout) delete m_midiout;
	if(m_midiin) delete m_midiin;

	m_midiout = 0;
	m_midiin = 0;
}

/**
	Run

	Called about 30 times a second
*/
void CSurf_YamahaDMENet::Run()
{
	if (m_midiin && m_Yamaha)
	{
		m_midiin->SwapBufs(timeGetTime());
		int l=0;
		MIDI_eventlist *list=m_midiin->GetReadBuf();
		MIDI_event_t *evts;
		while ((evts=list->EnumItems(&l))) m_Yamaha->onMidiEvent(evts);
	}
}
