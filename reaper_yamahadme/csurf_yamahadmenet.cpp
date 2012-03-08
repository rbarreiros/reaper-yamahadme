#include "csurf.h"
#include "../WDL/ptrlist.h"

#include "YamahaDME.h"
#include "LS9.h"

class CSurf_YamahaDMENet : public IReaperControlSurface
{
	int m_midi_in_dev,m_midi_out_dev;
	midi_Output *m_midiout;
	midi_Input *m_midiin;

	WDL_String descspace;
	char configtmp[1024];

	YamahaDME *m_Yamaha;

public:
	CSurf_YamahaDMENet(int indev, int outdev, int *errStats) 
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
		if(m_midiin && m_midiout)
			m_Yamaha = new LS9(m_midiin, m_midiout);

		OutputDebugString("Loaded...");
	}

	~CSurf_YamahaDMENet() 
	{
		delete m_Yamaha;
		delete m_midiout;
		delete m_midiin;
	}

	const char *GetTypeString() { return "Yamaha DME-Network"; }
	const char *GetDescString()
	{
		descspace.Set("Yamaha DME Network");
		char tmp[512];
		sprintf(tmp," (dev %d,%d)",m_midi_in_dev,m_midi_out_dev);
		descspace.Append(tmp);
		return descspace.Get();     
	}

	const char *GetConfigString() // string of configuration data
	{
		sprintf(configtmp,"0 0 %d %d",m_midi_in_dev,m_midi_out_dev);      
		return configtmp;
	}

	void CloseNoReset()
	{
		delete m_Yamaha;
		delete m_midiout;
		delete m_midiin;
		m_midiout = 0;
		m_midiin = 0;
	}

	void Run()
	{
		if (m_midiin)
		{
			m_midiin->SwapBufs(timeGetTime());
			int l=0;
			MIDI_eventlist *list=m_midiin->GetReadBuf();
			MIDI_event_t *evts;
			while ((evts=list->EnumItems(&l))) m_Yamaha->onMidiEvent(evts);
		}
	}

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
};

static void parseParms(const char *str, int parms[4])
{
  parms[0]=0;
  parms[1]=9;
  parms[2]=parms[3]=-1;

  const char *p=str;
  if (p)
  {
    int x=0;
    while (x<4)
    {
      while (*p == ' ') p++;
      if ((*p < '0' || *p > '9') && *p != '-') break;
      parms[x++]=atoi(p);
      while (*p && *p != ' ') p++;
    }
  }  
}

static IReaperControlSurface *createFunc(const char *type_string, const char *configString, int *errStats)
{
  int parms[4];
  parseParms(configString,parms);

  return new CSurf_YamahaDMENet(parms[2],parms[3],errStats);
}


static WDL_DLGRET dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
      {
        int parms[4];
        parseParms((const char *)lParam,parms);

		// esconde as text boxes a mais
		ShowWindow(GetDlgItem(hwndDlg,IDC_EDIT1),SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_EDIT1_LBL),SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_EDIT2),SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_EDIT2_LBL),SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_EDIT2_LBL2),SW_HIDE);

        int n=GetNumMIDIInputs();
        int x=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)"None");
        SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETITEMDATA,x,-1);
        x=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_ADDSTRING,0,(LPARAM)"None");
        SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_SETITEMDATA,x,-1);
        for (x = 0; x < n; x ++)
        {
          char buf[512];
          if (GetMIDIInputName(x,buf,sizeof(buf)))
          {
            int a=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)buf);
            SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETITEMDATA,a,x);
            if (x == parms[2]) SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETCURSEL,a,0);
          }
        }
        n=GetNumMIDIOutputs();
        for (x = 0; x < n; x ++)
        {
          char buf[512];
          if (GetMIDIOutputName(x,buf,sizeof(buf)))
          {
            int a=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_ADDSTRING,0,(LPARAM)buf);
            SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_SETITEMDATA,a,x);
            if (x == parms[3]) SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_SETCURSEL,a,0);
          }
        }
        SetDlgItemInt(hwndDlg,IDC_EDIT1,parms[0],TRUE);
        SetDlgItemInt(hwndDlg,IDC_EDIT2,parms[1],FALSE);
      }
    break;
    case WM_USER+1024:
      if (wParam > 1 && lParam)
      {
        char tmp[512];

        int indev=-1, outdev=-1, offs=0, size=9;
        int r=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETCURSEL,0,0);
        if (r != CB_ERR) indev = SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETITEMDATA,r,0);
        r=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_GETCURSEL,0,0);
        if (r != CB_ERR)  outdev = SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_GETITEMDATA,r,0);

        BOOL t;
        r=GetDlgItemInt(hwndDlg,IDC_EDIT1,&t,TRUE);
        if (t) offs=r;
        r=GetDlgItemInt(hwndDlg,IDC_EDIT2,&t,FALSE);
        if (t) 
        {
          if (r<1)r=1;
          else if(r>256)r=256;
          size=r;
        }

        sprintf(tmp,"%d %d %d %d",offs,size,indev,outdev);
        lstrcpyn((char *)lParam, tmp,wParam);
        
      }
    break;
  }
  return 0;
}

static HWND configFunc(const char *type_string, HWND parent, const char *initConfigString)
{
	return CreateDialogParam(g_hInst,MAKEINTRESOURCE(IDD_SURFACEEDIT_MCU),parent,dlgProc,(LPARAM)initConfigString);
}

reaper_csurf_reg_t csurf_yamahadme_reg = 
{
  "DME Network",
  "YAMAHA DME Network",
  createFunc,
  configFunc,
};
