/*
** reaper_csurf
** Copyright (C) 2006-2008 Cockos Incorporated
** License: LGPL.
*/

#include "csurf.h"
#include "LS9.h"
#include "M7CL.h"
#include "PM5D.h"

#include <WDL/ptrlist.h>

/**
	Parameter parsing function
*/
static void parseParms(const char *str, int parms[4])
{
  parms[0]=-1;
  parms[1]=-1;
  parms[2]=1;
  parms[3]=0;

  const char *p = str;

  if (p)
  {
    int x = 0;
    while (x < sizeof(parms))
    {
      while (*p == ' ') p++;
      if ((*p < '0' || *p > '9') && *p != '-') break;
      parms[x++]=atoi(p);
      while (*p && *p != ' ') p++;
    }
  }  
}

/**
	Entry point function of our Plugin
	Parses the parameters from the dialog, passes them to our class and instantiates the class
*/
static IReaperControlSurface *createFunc(const char *type_string, const char *configString, int *errStats)
{
  int parms[4];
  parseParms(configString,parms);

  // Detect which desk were dealing with
  char name[10];
  GetMIDIInputName(parms[0], name, sizeof(name));

  if(_strcmpi(name, "ls9") == 0)
  {
#ifdef _DEBUG
	  YamahaDME::Debug("Attempting to initialize LS9 control surface");
#endif
	  return new LS9(parms[0], parms[1], (YamahaDME::SynchDirection)parms[2], (parms[3] == 0) ? false : true, errStats);
  }
  else if(_strcmpi(name, "m7cl") == 0)
  {
#ifdef _DEBUG
	  YamahaDME::Debug("Attempting to initialize M7CL control surface");
#endif
	  return new M7CL(parms[0], parms[1], (YamahaDME::SynchDirection)parms[2], (parms[3] == 0) ? false : true, errStats);
  }
  else if(_strcmpi(name, "pm5d") == 0)
  {
#ifdef _DEBUG
	  YamahaDME::Debug("Attempting to initialize PM5D control surface");
#endif
	  return new PM5D(parms[0], parms[1], (YamahaDME::SynchDirection)parms[2], (parms[3] == 0) ? false : true, errStats);
  }

#ifdef _DEBUG
  YamahaDME::Debug("The midi device we tried to initialize is not LS9 or M7CL or PM5D");
#endif
  return NULL; // Cancels adding the device, would be nice to generate a dialog msg with a warning.
}

/**
	Function called when our dialog is created or edited.

*/
static WDL_DLGRET dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
      {
        int parms[4];
        parseParms((const char *)lParam,parms);
#ifdef _DEBUG
		YamahaDME::Debug((const char*)lParam);
#endif
		
		// Default synch method is to yamaha
		switch(parms[2])
		{
		case YamahaDME::TOYAMAHA:
			CheckDlgButton(hwndDlg, IDC_RADIO1, BST_CHECKED);
			break;
		case YamahaDME::TOREAPER:
			CheckDlgButton(hwndDlg, IDC_RADIO2, BST_CHECKED);
			break;
		case YamahaDME::NONE:
			CheckDlgButton(hwndDlg, IDC_RADIO3, BST_CHECKED);
			break;
		default:
			CheckDlgButton(hwndDlg, IDC_RADIO1, BST_CHECKED);
		};

		if(parms[3] > 0)
			CheckDlgButton(hwndDlg, IDC_CHKB1, BST_CHECKED);
		else
			CheckDlgButton(hwndDlg, IDC_CHKB1, BST_UNCHECKED);

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
            if (x == parms[0]) SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETCURSEL,a,0);
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
            if (x == parms[1]) SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_SETCURSEL,a,0);
          }
        }
      }
    break;

	case WM_USER+1024:
      if (wParam > 1 && lParam)
      {
        char tmp[10];

        int indev=-1, outdev=-1;
        int r=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETCURSEL,0,0);
        if (r != CB_ERR) indev = SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETITEMDATA,r,0);
        r=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_GETCURSEL,0,0);
        if (r != CB_ERR)  outdev = SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_GETITEMDATA,r,0);

		YamahaDME::SynchDirection dir = YamahaDME::NONE;
		if(IsDlgButtonChecked(hwndDlg, IDC_RADIO1))
			dir = YamahaDME::TOYAMAHA;
		else if(IsDlgButtonChecked(hwndDlg, IDC_RADIO2))
			dir = YamahaDME::TOREAPER;
		else if(IsDlgButtonChecked(hwndDlg, IDC_RADIO3))
			dir = YamahaDME::NONE;

		int live = IsDlgButtonChecked(hwndDlg, IDC_CHKB1);

        sprintf(tmp,"%d %d %d %d", indev, outdev, dir, live);
        lstrcpyn((char *)lParam, tmp,wParam);        
      }
    break;
  }
  return 0;
}

/**
	Creates the dialog
*/
static HWND configFunc(const char *type_string, HWND parent, const char *initConfigString)
{
	return CreateDialogParam(g_hInst,MAKEINTRESOURCE(IDD_SURFACEEDIT),parent,dlgProc,(LPARAM)initConfigString);
}

/**
	Registration array containing the info of our plugin
*/
reaper_csurf_reg_t csurf_yamahadme_reg = 
{
  "yamaha_dme_network", // unique string, no spaces A-Z, 0-9
  "Yamaha DME Network", // description
  createFunc,			// Plugin creation function
  configFunc,			// Dialog creation function
};
