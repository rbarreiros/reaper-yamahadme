#ifndef __YAMAHA_DME_H__
#define __YAMAHA_DME_H__

#include "csurf.h"

#define MAX_UNSIGNED_INT_VALUE 4294967296
#define MAX_SIGNED_INT_VALUE 2147483648

typedef MIDI_event_t MidiEvt; // Shorter typing

/*
	Base class for all desks
*/
class YamahaDME
{
public:
	enum DeskType {
		UNDEF = 0x00,
		PM5D = 0x0F,
		M7CL = 0x11,
		LS9 = 0x12
	}; 

	YamahaDME(midi_Input *in, midi_Output *out) : m_SelPressed(false), desk(YamahaDME::UNDEF) { m_midiInput = in; m_midiOutput = out; }
	virtual void onMidiEvent(MidiEvt *evt) = 0;
	virtual void synchToYamaha() = 0;
	virtual void synchToReaper() = 0;

	virtual double getFaderYamahaToReaper(int volume) = 0;

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

	void sendToYamaha(unsigned int opcodeA, unsigned int opcodeB, int param, int channel, int data);

protected:
	int getPanReaperToYamaha(double pan);
	int getFaderReaperToYamaha(double volume);

	double getPanYamahaToReaper(int pan);

	int getMidiDataValue(MidiEvt *evt, bool isSigned = false);
	int getMidiTrackId(MidiEvt *evt);

	MediaTrack *getTrackFromId(int trackid);
	MediaTrack *getTrack(MidiEvt *evt);

	void clearAllSelectedTracks();

	bool m_SelPressed;
	midi_Input *m_midiInput;
	midi_Output *m_midiOutput;
	DeskType desk;
};

#endif