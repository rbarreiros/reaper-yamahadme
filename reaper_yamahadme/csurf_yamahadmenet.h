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
 * csurf_yamahadmenet.h
 * Copyright (C) 2011 Rui Barreiros <rbarreiros@gmail.com>
 * http://www.audioluz.net
 */

#include "reaper_plugin.h"

#ifndef __CSURF_YAMAHADMENET_H__
#define __CSURF_YAMAHADMENET_H__

#include "YamahaDME.h"

class CSurf_YamahaDMENet : public IReaperControlSurface
{
public:
	CSurf_YamahaDMENet(int indev, int outdev, int *errStats);
	~CSurf_YamahaDMENet();

	const char *GetTypeString() { return "Yamaha DME-Network"; }
	const char *GetDescString();
	const char *GetConfigString();
	void CloseNoReset();
	void Run();

	void SetTrackListChange() { m_Yamaha->SetTrackListChange(); }
	void SetSurfaceVolume(MediaTrack *tr, double volume) { m_Yamaha->SetSurfaceVolume(tr, volume); }
	void SetSurfacePan(MediaTrack *tr, double pan) { m_Yamaha->SetSurfacePan(tr, pan); }
	void SetSurfaceMute(MediaTrack *tr, bool mute) { m_Yamaha->SetSurfaceMute(tr, mute); }
	void SetSurfaceSelected(MediaTrack *tr, bool selected) { m_Yamaha->SetSurfaceSelected(tr, selected); }
	void SetSurfaceSolo(MediaTrack *tr, bool solo) { m_Yamaha->SetSurfaceSolo(tr, solo); }
	void SetSurfaceRecArm(MediaTrack *tr, bool recarm) { m_Yamaha->SetSurfaceRecArm(tr, recarm); }
	void SetPlayState(bool play, bool pause, bool rec) { m_Yamaha->SetPlayState(play, pause, rec); }
	void SetRepeatState(bool rep) { m_Yamaha->SetRepeatState(rep); }
	void SetTrackTitle(MediaTrack *tr, const char *title) { m_Yamaha->SetTrackTitle(tr, title); }
	bool GetTouchState(MediaTrack *tr, int isPan) { return m_Yamaha->GetTouchState(tr, isPan); }
	void SetAutoMode(int mode) { m_Yamaha->SetAutoMode(mode); }
	void ResetCachedVolPanStates() { m_Yamaha->ResetCachedVolPanStates(); }
	void OnTrackSelection(MediaTrack *tr) { m_Yamaha->OnTrackSelection(tr); }
	bool IsKeyDown(int key) { return m_Yamaha->IsKeyDown(key); }
	int Extended(int call, void *parm1, void *parm2, void *parm3) { return m_Yamaha->Extended(call, parm1, parm2, parm3); }

private:
	int m_midi_in_dev,m_midi_out_dev;
	midi_Output *m_midiout;
	midi_Input *m_midiin;

	char configtmp[1024];

	YamahaDME *m_Yamaha;
};

#endif //  __CSURF_YAMAHADMENET_H__
