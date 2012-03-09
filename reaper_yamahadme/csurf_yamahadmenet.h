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

	CSurf_YamahaDMENet(int indev, int outdev, YamahaDME::SynchDirection sDir, int *errStats);
	~CSurf_YamahaDMENet();

	const char *GetTypeString() { return "Yamaha DME-Network"; }
	const char *GetDescString();
	const char *GetConfigString();
	void CloseNoReset();
	void Run();

	void SetTrackListChange() { if(m_Yamaha) m_Yamaha->SetTrackListChange(); }
	void SetSurfaceVolume(MediaTrack *tr, double volume) { if(m_Yamaha) m_Yamaha->SetSurfaceVolume(tr, volume); }
	void SetSurfacePan(MediaTrack *tr, double pan) { if(m_Yamaha) m_Yamaha->SetSurfacePan(tr, pan); }
	void SetSurfaceMute(MediaTrack *tr, bool mute) { if(m_Yamaha) m_Yamaha->SetSurfaceMute(tr, mute); }
	void SetSurfaceSelected(MediaTrack *tr, bool selected) { if(m_Yamaha) m_Yamaha->SetSurfaceSelected(tr, selected); }
	void SetSurfaceSolo(MediaTrack *tr, bool solo) { if(m_Yamaha) m_Yamaha->SetSurfaceSolo(tr, solo); }
	void SetSurfaceRecArm(MediaTrack *tr, bool recarm) { if(m_Yamaha) m_Yamaha->SetSurfaceRecArm(tr, recarm); }
	void SetPlayState(bool play, bool pause, bool rec) { if(m_Yamaha) m_Yamaha->SetPlayState(play, pause, rec); }
	void SetRepeatState(bool rep) { if(m_Yamaha) m_Yamaha->SetRepeatState(rep); }
	void SetTrackTitle(MediaTrack *tr, const char *title) { if(m_Yamaha) m_Yamaha->SetTrackTitle(tr, title); }
	bool GetTouchState(MediaTrack *tr, int isPan) { if(m_Yamaha) return m_Yamaha->GetTouchState(tr, isPan); return false; }
	void SetAutoMode(int mode) { if(m_Yamaha) m_Yamaha->SetAutoMode(mode); }
	void ResetCachedVolPanStates() { if(m_Yamaha) m_Yamaha->ResetCachedVolPanStates(); }
	void OnTrackSelection(MediaTrack *tr) { if(m_Yamaha) m_Yamaha->OnTrackSelection(tr); }
	bool IsKeyDown(int key) { if(m_Yamaha) return m_Yamaha->IsKeyDown(key); return false; }
	int Extended(int call, void *parm1, void *parm2, void *parm3) { if(m_Yamaha) return m_Yamaha->Extended(call, parm1, parm2, parm3); return 0; }

private:
	int m_midi_in_dev,m_midi_out_dev;
	YamahaDME::SynchDirection m_synchDir;

	midi_Output *m_midiout;
	midi_Input *m_midiin;

	char configtmp[1024];
	WDL_String desc;

	YamahaDME *m_Yamaha;
};

#endif //  __CSURF_YAMAHADMENET_H__
