/*
** Yabber Yabause SCSP Plugin
** Theo Berkau 2015-2016
**
** Based on code from Yabause
**
*/

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "resource.h"
#include "in2.h"
#include "aosdk/ao.h"
#include "aosdk/corlett.h"
#include "yabause/scsp.h"



// avoid CRT. Evil. Big. Bloated. Only uncomment this code if you are using 
// 'ignore default libraries' in VC++. Keeps DLL size way down.
// /*
BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}
// */

// post this to the main window at end of file (after playback as stopped)
#define WM_WA_MPEG_EOF WM_USER+2


// raw configuration.
#define NCH 2
#define SAMPLERATE 44100
#define BPS 16


In_Module mod;			// the output module (filled in near the bottom of this file)

char lastfn[MAX_PATH];	// currently playing file (used for getting info on the current file)

int file_length;		// file length, in bytes
int paused;				// are we paused?
volatile int seek_needed; // if != -1, it is the point that the decode 
						  // thread should seek to, in ms.

HANDLE input_file=INVALID_HANDLE_VALUE; // input file handle

volatile int killDecodeThread=0;			// the kill switch for the decode thread
HANDLE thread_handle=INVALID_HANDLE_VALUE;	// the handle to the decode thread
long long decode_pos;

DWORD WINAPI DecodeThread(LPVOID b); // the decode thread procedure

void config(HWND hwndParent)
{
	// if we had a configuration box we'd want to write it here (using DialogBox, etc)
}

void about(HWND hwndParent)
{
	MessageBoxA(hwndParent,
#ifdef DEBUG
		"Yabber SSF Plugin v1.0D\n\n"
#else
		"Yabber SSF Plugin v1.0\n\n"
#endif
		"by Cyber Warrior X 2015-2016. Based on code from AOSDK and Yabause\n"
		"cwx@cyberwarriorx.com\n\n"
		"Thanks go to:\n"
		"AOSDK Team\n"
		"Yabause Team\n",
#ifdef DEBUG
		"About Yabber SSF input plugin 1.0D",
#else
		"About Yabber SSF input plugin 1.0",
#endif
		MB_OK);
}

void init() { 
	/* any one-time initialization goes here (configuration reading, etc) */ 
}

void quit() { 
	/* one-time deinit, such as memory freeing */ 
}

int isourfile(const char *fn) { 
// used for detecting URL streams.. unused here. 
// return !strncmp(fn,"http://",7); to detect HTTP streams, etc
	return 0; 
} 

// called when winamp wants to play a file
int play(const char *fn) 
{ 
	unsigned long f_size=0;
	int maxlatency;
	unsigned long tmp;

	strcpy_s(lastfn, sizeof(lastfn), fn);

	if (!load_ssf((char *)fn))
		return 1;

	paused=0;

	maxlatency = mod.outMod->Open(SAMPLERATE, NCH, 16, -1, -1);

	if (maxlatency < 0)
	{
		return 1;
	}

	mod.SetInfo(0,(int)(SAMPLERATE / 1000), NCH,1);

	mod.SAVSAInit(maxlatency,SAMPLERATE);
	mod.VSASetInfo(NCH,SAMPLERATE);

	mod.outMod->SetVolume(-666);

	killDecodeThread=0;
	decode_pos = 0;
	thread_handle = (HANDLE) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) DecodeThread,(void *) &killDecodeThread,0,&tmp);
	return 0; 
}

// standard pause implementation
void pause() { paused=1; mod.outMod->Pause(1); }
void unpause() { paused=0; mod.outMod->Pause(0); }
int ispaused() { return paused; }


// stop playing.
void stop() 
{ 
	if (thread_handle != INVALID_HANDLE_VALUE)
	{
		killDecodeThread=1;
		if (WaitForSingleObject(thread_handle,100000) == WAIT_TIMEOUT)
		{
			MessageBox(mod.hMainWindow,"error asking thread to die!\n",
				"error killing decode thread",0);
			TerminateThread(thread_handle,0);
		}
		CloseHandle(thread_handle);
		thread_handle = INVALID_HANDLE_VALUE;
	}

	// close output system
	mod.outMod->Close();

	// deinitialize visualization
	mod.SAVSADeInit();
	
	ScspDeInit();
}

// returns length of playing track
int getlength() 
{
	ao_display_info info;
	if (ssf_fill_info(&info) == AO_SUCCESS)
		return (int)psfTimeToMS(info.info[6])+psfTimeToMS(info.info[7]);
	else
		return 9999000;
}

// returns current output position, in ms.
// you could just use return mod.outMod->GetOutputTime(),
// but the dsp plug-ins that do tempo changing tend to make
// that wrong.
int getoutputtime() 
{ 
	return mod.outMod->GetOutputTime();
}

// called when the user releases the seek scroll bar.
// usually we use it to set seek_needed to the seek
// point (seek_needed is -1 when no seek is needed)
// and the decode thread checks seek_needed.
void setoutputtime(int time_in_ms) { 
	seek_needed=time_in_ms; 
}


// standard volume/pan functions
void setvolume(int volume) { mod.outMod->SetVolume(volume); }
void setpan(int pan) { mod.outMod->SetPan(pan); }


BOOL CALLBACK FileInfoDlg (HWND hwnd, UINT message,	WPARAM wParam, LPARAM lParam)
{
   switch(message)
   {
      case WM_INITDIALOG:
      {
         ao_display_info info;
         SetDlgItemText(hwnd, IDC_FILENAME, (char *)lParam);

         if (ssf_fill_info(&info) == AO_SUCCESS)
         {
				int i;
				for (i = 1; i < 9; i++)
				{
					SetDlgItemText(hwnd, IDC_INFOTEXTLINE+i-1, info.title[i]);
					SetDlgItemText(hwnd, IDC_INFOEDITLINE+i-1, info.info[i]);
				}
			}
			return TRUE;
		}
	case WM_CLOSE:
      EndDialog(hwnd,FALSE);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		   case IDCANCEL:
			{
				SendMessage(hwnd, WM_CLOSE, 0, 0);
				return TRUE;
			}
		   case IDOK:
			{
				SendMessage(hwnd, WM_CLOSE, 0, 0);
			}
		}
		return FALSE;
	}
	return FALSE;
}

// this gets called when the use hits Alt+3 to get the file info.
// if you need more info, ask me :)

int infoDlg(const char *fn, HWND hwnd)
{
	DialogBoxParam(mod.hDllInstance, MAKEINTRESOURCE(IDD_FILEINFO), hwnd, FileInfoDlg, (LPARAM)fn);
	return 0;
}


// this is an odd function. it is used to get the title and/or
// length of a track.
// if filename is either NULL or of length 0, it means you should
// return the info of lastfn. Otherwise, return the information
// for the file in filename.
// if title is NULL, no title is copied into it.
// if length_in_ms is NULL, no length is copied into it.
void getfileinfo(const char *filename, char *title, int *length_in_ms)
{
	if (!filename || !*filename)  // currently playing file
	{
		if (length_in_ms) *length_in_ms=getlength();

		if (title) // get non-path portion.of filename
		{
			char *p=lastfn+strlen(lastfn);
			while (*p != '\\' && p >= lastfn) p--;
			strcpy(title,++p);
		}
	}
	else // some other file
	{
		FILE *fp = fopen(filename, "rb");
		unsigned int size;
		unsigned char *buf;
		corlett_t *extra;
		unsigned char *out;
		uint64 out_size;

		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buf = (unsigned char *)malloc(size);
		fread((void *)buf, 1, size, fp);
		fclose(fp);

		if (corlett_decode(buf, size, &out, &size, &extra) == AO_SUCCESS)
		{
			free (out);
			free (buf);

         if (length_in_ms)
             *length_in_ms = psfTimeToMS(extra->inf_length);
			if (title)
				strcpy(title, extra->inf_title);

			free (extra);
		}
		else
		{
			free (buf);

			if (length_in_ms) // calculate length
				*length_in_ms=-1000; // the default is unknown file length (-1000).
			if (title) // get non path portion of filename
			{
				const char *p=filename+strlen(filename);
				while (*p != '\\' && p >= filename) p--;
				strcpy(title,++p);
			}
		}
	}
}

void eq_set(int on, char data[10], int preamp) 
{ 
	// most plug-ins can't even do an EQ anyhow.. I'm working on writing
	// a generic PCM EQ, but it looks like it'll be a little too CPU 
	// consuming to be useful :)
	// if you _CAN_ do EQ with your format, each data byte is 0-63 (+20db <-> -20db)
	// and preamp is the same. 
}


DWORD WINAPI DecodeThread(LPVOID b)
{
	int buf_size=44100/30 * 4;
	char *buf=malloc(buf_size);
	int t;
	while (!killDecodeThread)
	{
		int len;
		char text[512];
		int to_write_pos;

		if (seek_needed != -1) // seek is needed.
		{
			mod.outMod->Flush(seek_needed*10/441); // flush output device and set 
			seek_needed=-1;
			// output position to the seek position
			decode_pos = seek_needed;
		}

		while (mod.outMod->CanWrite() < 4) Sleep(0);
		len = mod.outMod->CanWrite();

		if (len > buf_size)
			len = buf_size;

		if (ssf_gen((short *)buf, len / 4) == AO_FAIL)
		{
			PostMessage(mod.hMainWindow,WM_WA_MPEG_EOF,0,0); // tell WA it's EOF
		}

		t=mod.outMod->GetWrittenTime();
		mod.SAAddPCMData(buf,NCH,16,t);
		mod.VSAAddPCMData(buf,NCH,16,t);
		// if we have a DSP plug-in, then call it on our samples
		//if (mod.dsp_isactive()) 
		//	l=mod.dsp_dosamples(
		//	(short *)buf,l/NCH/(BPS/8),BPS,NCH,SAMPLERATE
		//	) // dsp_dosamples
		//	*(NCH*(BPS/8));

		mod.outMod->Write(buf, len);

		while (mod.outMod->IsPlaying() && mod.outMod->CanWrite() < 4) Sleep(10);

	}
	if (buf)
		free(buf);
	return 0;
}


// module definition.

In_Module mod = 
{
	IN_VER,	// defined in IN2.H
#ifdef DEBUG
	"Yabber SSF Plugin v1.0D"
#else
	"Yabber SSF Plugin v1.0"
#endif
	// winamp runs on both alpha systems and x86 ones. :)
#ifdef __alpha
	"(AXP)"
#else
	"(x86)"
#endif
	,
	0,	// hMainWindow (filled in by winamp)
	0,  // hDllInstance (filled in by winamp)
	"SSF\0SSF File (*.ssf)\0MINISSF\0Mini SSF file(*.minissf)\0\0"
	// this is a double-null limited list. "EXT\0Description\0EXT\0Description\0" etc.
	,
	1,	// is_seekable
	1,	// uses output plug-in system
	config,
	about,
	init,
	quit,
	getfileinfo,
	infoDlg,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,
	
	getlength,
	getoutputtime,
	setoutputtime,

	setvolume,
	setpan,

	0,0,0,0,0,0,0,0,0, // visualization calls filled in by winamp

	0,0, // dsp calls filled in by winamp

	eq_set,

	NULL,		// setinfo call filled in by winamp

	0 // out_mod filled in by winamp

};

// exported symbol. Returns output module.

__declspec( dllexport ) In_Module * winampGetInModule2()
{
	return &mod;
}
