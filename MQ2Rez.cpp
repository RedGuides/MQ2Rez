//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
// Project: MQ2Rez.cpp
// Author: TheZ, made from an amalgamation of dewey and s0rcier's code
// Updated by Maskoi 09/03/2011
// Removed all loot corpse code & added recognition for Call of the Wild rez.
// Updated by Sym 05/14/2012
// Added delay for accepting rez, removed unused code
// Updated by Sym 01/30/2013
// Added audio notify on hover and command line toggles to control it
// Updated by Sym 03/14/2013
// Fixed typo in audio ini setting
// Updated by Sym 09/02/2013
// Added /rez spawn delay #
// allows custom value for how long to wait before returning to bind if no rez
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

#define    PLUGIN_NAME    "MQ2Rez"                // Plugin Name
#define    PLUGIN_FLAG      0xF9FF                // Plugin Auto-Pause Flags (see InStat)

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

#ifndef PLUGIN_API
	#include "../MQ2Plugin.h"
	PreSetup(PLUGIN_NAME);
    #include <map>
#endif PLUGIN_API
#include "mmsystem.h"
#pragma comment(lib, "winmm.lib")

#define     NOID                -1
DWORD       Initialized         = false;            // Plugin Initialized?
DWORD       Conditions          = false;            // Window Conditions and Character State
long        InStat();
long        SetBOOL(long Cur, PCHAR Val, PCHAR Sec="", PCHAR Key="");
long        SetLONG(long Cur, PCHAR Val, PCHAR Sec="", PCHAR Key="", bool ZeroIsOff=false);
void        WinClick(CXWnd *Wnd, PCHAR ScreenID, PCHAR ClickNotification, DWORD KeyState=0);
bool        WinState(CXWnd *Wnd);
long        AutoRezAccept       = false;            // Take Rez box?
int         AutoRezPct          = 0;                // Take Rez %
long        AutoRezSpawn        = false;            // Let respawn window time out or port to bind.
long        AutoRezSpawnDelay   = 0;                // How long to wait before returning to spawn.
long        AutoRezTimer        = 0;                // How long after zone to take rez (in pulses)
long        PulseCount          = 0;
long        ClickWait           = 0;
long        RezDone             = false;
long        RezClicked          = false;
char        szCommand[MAX_STRING];
char        szAudio[MAX_STRING];
long        RezCommandOn        = false;
char        szTemp[MAX_STRING];
bool        doCommand            = false;
bool        AreWeZoning            = false;
bool        showMessage            = true;
bool        bStartTimer            = false;
long        bSoundAlarm            = false;
char        RezAlertSound[MAX_STRING];
time_t seconds;
time_t StartTime;
time_t RespawnStartTime;

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

bool WinState(CXWnd *Wnd) {
    return (Wnd && ((PCSIDLWND)Wnd)->dShow);
}

void WinClick(CXWnd *Wnd, PCHAR ScreenID, PCHAR ClickNotification, DWORD KeyState) {
    if(Wnd) if(CXWnd *Child=Wnd->GetChildItem(ScreenID)) {
        BOOL KeyboardFlags[4];
        *(DWORD*)&KeyboardFlags=*(DWORD*)&((PCXWNDMGR)pWndMgr)->KeyboardFlags;
        *(DWORD*)&((PCXWNDMGR)pWndMgr)->KeyboardFlags=KeyState;
        SendWndClick2(Child,ClickNotification);
        *(DWORD*)&((PCXWNDMGR)pWndMgr)->KeyboardFlags=*(DWORD*)&KeyboardFlags;
    }
}

long SetLONG(long Cur,PCHAR Val, PCHAR Sec, PCHAR Key, bool ZeroIsOff,long Maxi) {
    char ToStr[16]; long Result=atol(Val);
    if(Result && Result>Maxi) Result=Maxi; _itoa_s(Result,ToStr,10);
    if(Sec[0] && Key[0]) WritePrivateProfileString(Sec,Key,ToStr,INIFileName);
    //sprintf_s(Buffer,"%s :: %s \ag%s\ax",Sec,Key,(ZeroIsOff && !Result)?"\aroff":ToStr);
    //WriteChatColor(Buffer);
    return Result;
}

long SetBOOL(long Cur, PCHAR Val, PCHAR Sec, PCHAR Key) {
    long result=0;
    if(!_strnicmp(Val,"false",5) || !_strnicmp(Val,"off",3) || !_strnicmp(Val,"0",1))    result=0;
    else if(!_strnicmp(Val,"true",4) || !_strnicmp(Val,"on",2) || !_strnicmp(Val,"1",1)) result=1;
    else result=(!Cur)&1;
    if(Sec[0] && Key[0]) WritePrivateProfileString(Sec,Key,result?"1":"0",INIFileName);
    //sprintf_s(Buffer,"%s :: %s %s",Sec,Key,result?"\agon\ax":"\agoff\ax");
    //WriteChatColor(Buffer);
    return result;
}

long InStat() {
    Conditions=0x00000000;
    if(WinState(FindMQ2Window("GuildTributeMasterWnd")))                Conditions|=0x0001;
    if(WinState(FindMQ2Window("TributeMasterWnd")))                     Conditions|=0x0002;
    if(WinState(FindMQ2Window("GuildBankWnd")))                         Conditions|=0x0004;
    if(WinState((CXWnd*)pTradeWnd))                                     Conditions|=0x0008;
    if(WinState((CXWnd*)pMerchantWnd))                                  Conditions|=0x0010;
    if(WinState((CXWnd*)pBankWnd))                                      Conditions|=0x0020;
    if(WinState((CXWnd*)pGiveWnd))                                      Conditions|=0x0040;
    if(WinState((CXWnd*)pSpellBookWnd))                                 Conditions|=0x0080;
    if(WinState((CXWnd*)pLootWnd))                                      Conditions|=0x0200;
    if(WinState((CXWnd*)pInventoryWnd))                                 Conditions|=0x0400;
    if(WinState((CXWnd*)pCastingWnd))                                   Conditions|=0x1000;
    if(GetCharInfo()->standstate==STANDSTATE_CASTING)                   Conditions|=0x2000;
    if(((((PSPAWNINFO)pCharSpawn)->CastingData.SpellSlot)&0xFF)!=0xFF)  Conditions|=0x4000;
    if(GetCharInfo()->Stunned)                                          Conditions|=0x0100;
    if((Conditions&0x0600)!=0x0600 && (Conditions&0x0600))              Conditions|=0x0800;
    return Conditions;
}

BOOL IsWindowOpen(PCHAR WindowName) {
    PCSIDLWND pWnd=(PCSIDLWND)FindMQ2Window(WindowName);
    if (!pWnd) return false;
    return (BOOL)pWnd->dShow;
}

 int ExpRezBox(void)
{

	CXWnd *Child;
	CXWnd *pWnd;
	char InputCXStr[128],*p;
    // text for regular % rez message
    char RezText[128];
    // new var for call of the wild text check
    char CotWText[256],*o;
	int v;

	pWnd=(CXWnd *)FindMQ2Window("ConfirmationDialogBox");
	if(pWnd)
	{
		if (((PCSIDLWND)(pWnd))->dShow==0) return -1;
		Child=pWnd->GetChildItem("cd_textoutput");
		if(Child)
		{
			ZeroMemory(InputCXStr,sizeof(InputCXStr));
            ZeroMemory(CotWText,sizeof(CotWText));
			GetCXStr(((PCSIDLWND)Child)->SidlText,InputCXStr,sizeof(InputCXStr));
            GetCXStr(((PCSIDLWND)Child)->SidlText,CotWText,sizeof(CotWText));
			p = strstr(InputCXStr,"(");
            // check if text is CotW rez
            o = strstr(CotWText,"is attempting to return you to your corpse.");
            // if CotW set rez %, v, to 110 to use normal mq2rez functions 
            if (o) {
                if (showMessage) WriteChatColor("MQ2Rez: This is a Call of the Wild rez.",CONCOLOR_YELLOW);
                v=110;
                return v;
            }
			if (!p) return -1;
			v = atoi(p+1);
			p = strstr(p,"percent");
			if (!p) return -1;
            sprintf_s(RezText, "MQ2Rez: This is a %d pct rez.", v);
            if (p && showMessage) WriteChatColor(RezText,CONCOLOR_YELLOW);
			return v;
		}
	}
	return -1;
}

int SelectResurrect(void)
{
	CListWnd *Child;
	CXWnd *pWnd;
	CXStr Text;
	char buf[2048];
	int x=0;

	pWnd=(CXWnd *)FindMQ2Window("RespawnWnd");
	if(!pWnd) return -1;
	if(((PCSIDLWND)(pWnd))->dShow==0) return -1;
	Child=(CListWnd *)pWnd->GetChildItem("rw_optionslist");
	if(!Child) return -1;
	for(x=0; x<Child->ItemsArray.Count; x++)
	{
		memset(buf,0,2048);
		Child->GetItemText(&Text,x,1);
		GetCXStr(Text.Ptr, buf, 2048);
		
		if(_strnicmp(buf,"Resurrect",9)==0)
		{
			SendListSelect("RespawnWnd", "RW_OptionsList", x);
			WriteChatf("MQ2Rez::\aySelectResurrect Found: \ax <\ag %s at %i \ax>.", buf, x);
			return x; // we found what we needed, so return
		}
	}
	return -1; // This "should" never happen, unless Daybreak changes the resurrect text, or the list box... Or some other crazy shit happens...
}


void AutoRezCommand(PSPAWNINFO pCHAR, PCHAR zLine) {
    bool ShowInfo=false;
    bool NeedHelp=false;
	char Parm1[MAX_STRING];
	char Parm2[MAX_STRING];
    char Parm3[MAX_STRING];
	GetArg(Parm1,zLine,1);
	GetArg(Parm2,zLine,2);
    GetArg(Parm3,zLine,3);

    if(!_stricmp("help",Parm1)) NeedHelp=true;

    //Accept rez?
    else if(!_stricmp("accept",Parm1)) {
        if (!_stricmp("on",Parm2)) AutoRezAccept=SetBOOL(AutoRezAccept ,Parm2 ,"MQ2Rez","Accept");
        if (!_stricmp("off",Parm2)) AutoRezAccept=SetBOOL(AutoRezAccept ,Parm2,"MQ2Rez","Accept");
        WriteChatf("MQ2Rez :: Accept %s",AutoRezAccept ? "\agON\ax" : "\arOFF\ax");
	}
    //What percent?
    else if(!_stricmp("pct",Parm1)) {
		WritePrivateProfileString("MQ2Rez","RezPct",Parm2,INIFileName);
		AutoRezPct=atoi(Parm2);
        WriteChatf("MQ2Rez :: Accept Percent \ag%d\ax",AutoRezPct);
	}
    //Should I spawn first?
    else if(!_stricmp("spawn",Parm1)) {
        if (!_stricmp("on",Parm2)) AutoRezSpawn=SetBOOL(AutoRezSpawn ,Parm2 ,"MQ2Rez","Spawn");
        if (!_stricmp("off",Parm2)) AutoRezSpawn=SetBOOL(AutoRezSpawn ,Parm2,"MQ2Rez","Spawn");
        WriteChatf("MQ2Rez :: Spawn %s",AutoRezSpawn ? "\agON\ax" : "\arOFF\ax");
        if (!_stricmp("delay",Parm2)) {
            WritePrivateProfileString("MQ2Rez","SpawnDelay",Parm3,INIFileName);
            AutoRezSpawnDelay=atoi(Parm3);
            WriteChatf("MQ2Rez :: Spawn Delay \ag%d\ax seconds", AutoRezSpawnDelay);
        }
    }
    //execute command on rez?
    else if(!_stricmp("command",Parm1)) {
        if (!_stricmp("on",Parm2)) RezCommandOn=SetBOOL(RezCommandOn,Parm2 ,"MQ2Rez","RezCommandOn");
        if (!_stricmp("off",Parm2)) RezCommandOn=SetBOOL(RezCommandOn,Parm2,"MQ2Rez","RezCommandOn");
        WriteChatf("MQ2Rez :: Execute Command %s", RezCommandOn ? "\agON\ax" : "\arOFF\ax");
    }
    //What command should I execute?
    else if(!_stricmp("setcommand",Parm1)) {
		WritePrivateProfileString("MQ2Rez","Command Line",Parm2,INIFileName);
        strcpy_s(szCommand,Parm2);
        WriteChatf("MQ2Rez :: Execute command set to: \ag%s\ax",szCommand);
		}
    else if(!_stricmp("setaudio",Parm1)) {
        WritePrivateProfileString("MQ2Rez","SoundFile",Parm2,INIFileName);
        strcpy_s(szAudio,Parm2);
        WriteChatf("MQ2Rez :: Sound file set to: \ag%s\ax",szAudio);
    }
    else if(!_stricmp("testaudio",Parm1)) {
        WriteChatf("MQ2Rez :: \agTesting \atAudio \ayAlert\ax using %s", szAudio);
        PlaySound(szAudio,0,SND_ASYNC);
    }
    else if(!_stricmp("hoveralert",Parm1)) {
        if (!_stricmp("on",Parm2)) bSoundAlarm=SetBOOL(bSoundAlarm,Parm2 ,"MQ2Rez","HoverAlert");
        if (!_stricmp("off",Parm2)) bSoundAlarm=SetBOOL(bSoundAlarm,Parm2,"MQ2Rez","HoverAlert");
        WriteChatf("MQ2Rez :: Audio notify on hover %s", bSoundAlarm ? "\agON\ax" : "\arOFF\ax");
    }
    //Help??
    else if(!_stricmp("",Parm1))  {
        ShowInfo=TRUE;
        NeedHelp=TRUE;
	}
    if(NeedHelp) {
        WriteChatColor("Usage:");
        WriteChatColor("/rez -> displays settings");
        WriteChatColor("/rez accept on|off -> Toggle auto-accepting rezbox");
        WriteChatColor("/rez spawn  on|off -> Toggles going to bind point after death");
        WriteChatColor("/rez spawn delay # -> How long to wait before going to bind point after death, in seconds");
        WriteChatColor("/rez pct # -> Autoaccepts rezes only if they are higher than # percent");
        WriteChatColor("/rez command on|off -> Toggle use of a command after looting out corpse");
        WriteChatColor("/rez hoveralert on|off -> Toggle audio notification on hover");
        WriteChatColor("/rez setcommand mycommand -> Set the command that you want.");
        WriteChatColor("/rez testaudio -> Test plays the audio alert.");
        WriteChatColor("/rez setaudio filename -> Sets the audio file to use.");
        WriteChatColor("/rez help");
    }
    if (ShowInfo) {
        WriteChatf("MQ2Rez :: Accept %s",AutoRezAccept ? "\agON\ax" : "\arOFF\ax");
        WriteChatf("MQ2Rez :: Spawn %s",AutoRezSpawn ? "\agON\ax" : "\arOFF\ax");
        WriteChatf("MQ2Rez :: Spawn Delay \ag%d\ax seconds", AutoRezSpawnDelay);
        WriteChatf("MQ2Rez :: Accept Percent \ag%d\ax",AutoRezPct);
        WriteChatf("MQ2Rez :: Execute Command %s", RezCommandOn ? "\agON\ax" : "\arOFF\ax");
        WriteChatf("MQ2Rez :: Execute command set to: \ag%s\ax",szCommand);
        WriteChatf("MQ2Rez :: Audio notify on hover %s", bSoundAlarm ? "\agON\ax" : "\arOFF\ax");
	}
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

PLUGIN_API VOID SetGameState(DWORD GameState) {
    if(GameState==GAMESTATE_INGAME) {
        if(!Initialized) {
            Initialized=true;
            sprintf_s(INIFileName,"%s\\%s_%s.ini",gszINIPath,EQADDR_SERVERNAME,GetCharInfo()->Name);
            AutoRezAccept=GetPrivateProfileInt("MQ2Rez","Accept" ,0,INIFileName);
            AutoRezSpawn =GetPrivateProfileInt("MQ2Rez","Spawn"  ,0,INIFileName);
            AutoRezSpawnDelay =GetPrivateProfileInt("MQ2Rez","SpawnDelay" ,0,INIFileName);
            AutoRezPct   =GetPrivateProfileInt("MQ2Rez","RezPct" ,0,INIFileName);
            RezCommandOn =GetPrivateProfileInt("MQ2Rez","RezCommandOn" ,0,INIFileName);
            bSoundAlarm  =GetPrivateProfileInt("MQ2Rez","bSoundAlarm" ,0,INIFileName);
            GetPrivateProfileString("MQ2Rez","Command Line","DISABLED",szTemp,MAX_STRING,INIFileName);
            strcpy_s(szCommand,szTemp);
            if(!strcmp(szCommand,"DISABLED")) {
                RezCommandOn = false;                
            }
            sprintf_s(szAudio,"%s\\mq2rez.wav",gszINIPath);
            GetPrivateProfileString("MQ2Rez","Soundfile",szAudio,RezAlertSound,MAX_STRING,INIFileName);
            WritePrivateProfileString("MQ2Rez","Soundfile",szAudio,INIFileName);
        }
    } else if(GameState!=GAMESTATE_LOGGINGIN) {
            if(Initialized) {
                Initialized=0;
		}
	}
}

PLUGIN_API VOID InitializePlugin() {
	AddCommand("/rez",AutoRezCommand);
}

PLUGIN_API VOID ShutdownPlugin() {
	RemoveCommand("/rez");
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//

PLUGIN_API VOID OnPulse() {
    
    if (RezClicked && ClickWait>35)  {
       //DoCommand(GetCharInfo()->pSpawn,"/squelch /notify RespawnWnd RW_OptionsList listselect 2");
        SelectResurrect();
        if(ClickWait>75) return;
        DoCommand(GetCharInfo()->pSpawn,"/squelch /notify RespawnWnd RW_SelectButton leftmouseup");
        ClickWait=0;
        RezClicked=false;
        RezDone = true;
        return;
			}
    else if(RezClicked) {
        ClickWait++;
        return;
		}
    static int RespawnWndCnt = 0;
    static int RezBoxCnt = 0;

    if (IsWindowOpen("RespawnWnd")) {
        RespawnWndCnt++;
        if (!bStartTimer) {
            bStartTimer = true;
            StartTime = time(NULL);
            RespawnStartTime = time(NULL);
        }
        if (bSoundAlarm && (time(NULL) > StartTime + 15)) {
            PlaySound(RezAlertSound,0,SND_ASYNC);
            StartTime = time(NULL);
							}
						}
						else {
        RespawnWndCnt=0;
        bStartTimer = false;
					}
    if (AutoRezAccept && (ExpRezBox()>=AutoRezPct) ) {
        RezBoxCnt++;
        PulseCount++;
        showMessage = false;
    } else {
        RezBoxCnt = 0;
        PulseCount = 0;
        showMessage = true;
    }
    if (AutoRezSpawn && RespawnWndCnt) {
        if ((time(NULL) > RespawnStartTime + AutoRezSpawnDelay)) {
            WinClick(FindMQ2Window("RespawnWnd"),"RW_SelectButton","leftmouseup",1);
        return;
				}
			}
    if (AutoRezAccept && RezBoxCnt > 0 && PulseCount > AutoRezTimer) {
        
        WriteChatColor("Accepting Rez now");
        DoCommand(GetCharInfo()->pSpawn,"/notify ConfirmationDialogBox CD_Yes_Button leftmouseup");
        RezClicked = true;
        doCommand = true;
        seconds = time (NULL);
		}

    if ( !AreWeZoning && doCommand && RezCommandOn && time(NULL) > seconds + 4) {
        doCommand = false;
        WriteChatf("Rez accepted. Executing %s",szCommand);
        sprintf_s(szTemp,"/docommand %s", szCommand);
        DoCommand(GetCharInfo()->pSpawn,szTemp);
	}

}

PLUGIN_API VOID OnBeginZone(VOID) 
{
    AreWeZoning = true; 
}

PLUGIN_API VOID OnEndZone(VOID) {
    AreWeZoning = false;
    seconds = time (NULL);
}
