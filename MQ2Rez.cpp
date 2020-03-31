//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
// Project: MQ2Rez.cpp
// Author: Complete rewrite by eqmule
// Based on the previous work and ideas of TheZ, dewey and s0rcier
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
// v3.0 - Eqmule 07-22-2016 - Added string safety.
// v3.1 - Eqmule 08-22-2017 - Added a delay to pulse for checking the dialog, it has improved performance.
// v3.2 - s0rCieR 01-28-2019 - Added missing command to be executed when you got rezzed! removed rezz sickness check
// v3.3 - EqMule May 16 2019 -Added /rez delay
// v3.4 - ChatWithThisName - 05-21-2019 - Complete rewrite, loses /rez delay, fixes Safe Mode, Fixes voice notify, now
//        reports who you accepted the rez from and the percentage, adds color to outputs, adds /rez settings -> outputs current settings.
// v3.5 - ChatWithThisName - 10/14/2019 - Add ReleaseToBind toggle using /rez Release. 0/1/on/off as options. Allows immediate release to bind. Accept must be on!
// v3.6 - ChatWithThisName - 10/24/2019 - Add /rez silent on|off to allow the users to remove text output when recieving a rez. Updated help and showsettings
// v3.7 - ChatWithThisName - 11/23/2019 - Add Rez datatype.
#pragma once
#include "../MQ2Plugin.h"

#define PLUGINMSG "\aw[\agMQ2Rez\aw]\ao:: "
#define PLUGIN_NAME "MQ2Rez"
PreSetup(PLUGIN_NAME);
float VERSION = 3.5f;
PLUGIN_VERSION(VERSION);



//Variables
bool AutoRezAccept = false;
bool CommandPending = false;
bool DoCommand = false;
bool Initialized = false;
bool SafeMode = false;
bool VoiceNotify = false;
bool Notified = true;
bool ReleaseToBind = false;
bool bQuiet = false;

char RezCommand[MAX_STRING] = { 0 };


int Pulse = 0;
int PulseDelay = 20;
int AutoRezPct = 0;

uint64_t AcceptedRez = GetTickCount64();

//Not using rezdelay stuff.
uint64_t RezDelay = 100;
uint64_t RezDelayTimer = 0;


//Prototypes
bool atob(char x[MAX_STRING]);
bool CanRespawn();
bool IAmDead();
bool ShouldTakeRez();
inline bool InGame();

void AcceptRez();
void DisplayHelp();
void DoINIThings();
void LeftClickWnd(char* MyWndName, char* MyButton);
void ShowSettings();
void SpawnAtCorpse();
void TheRezCommand(PSPAWNINFO pCHAR, char* szLine);

class MQ2RezType : public MQ2Type
{
public:
	enum RezMembers
	{
		Version = 1,
		Accept,
		Percent,
		Pct,
		xSafeMode,
		Voice,
		Release
	};

	MQ2RezType() : MQ2Type("Rez")
	{
		TypeMember(Version);//1,Float
		TypeMember(Accept);//2,Bool
		TypeMember(Percent);//3,Int
		TypeMember(Pct);//4,Int
		AddMember(xSafeMode, "SafeMode");//5,Bool
		TypeMember(Voice);//6,Bool
		TypeMember(Release);//7,Bool
	};

	~MQ2RezType(){}

	bool GetMember(MQ2VARPTR VarPtr, PCHAR Member, PCHAR Index, MQ2TYPEVAR& Dest)
	{
		PMQ2TYPEMEMBER pMember = MQ2RezType::FindMember(Member);
		if (!pMember)
			return false;
		switch ((RezMembers)pMember->ID)
		{
			case Version:
			{
				Dest.Float = VERSION;
				Dest.Type = pFloatType;
				return true;
			}
			case Accept:
			{
				Dest.Int = AutoRezAccept;
				Dest.Type = pBoolType;
				return true;
			}
			case Percent://intentional fallthrough
			case Pct:
			{
				Dest.Int = AutoRezPct;
				Dest.Type = pIntType;
				return true;
			}
			case xSafeMode:
			{
				Dest.Int = SafeMode;
				Dest.Type = pBoolType;
				return true;
			}
			case Voice:
			{
				Dest.Int = VoiceNotify;
				Dest.Type = pBoolType;
				return true;
			}
			case Release:
			{
				Dest.Int = ReleaseToBind;
				Dest.Type = pBoolType;
				return true;
			}
			default:
				return false;
				break;
		}
	}

	bool ToString(MQ2VARPTR VarPtr, PCHAR Destination)
	{
		return true;
	}

	bool FromData(MQ2VARPTR& VarPtr, MQ2TYPEVAR& Source)
	{
		return false;
	}
	bool FromString(MQ2VARPTR& VarPtr, PCHAR Source)
	{
		return false;
	}

};

MQ2RezType* pRezType = nullptr;

BOOL dataRez(PCHAR szIndex, MQ2TYPEVAR& Ret)
{
	Ret.DWord = 1;
	Ret.Type = pRezType;
	return true;
}

PLUGIN_API VOID InitializePlugin(VOID)
{
	AddCommand("/rez", TheRezCommand);
	AddMQ2Data("Rez", dataRez);
	pRezType = new MQ2RezType;
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
	RemoveCommand("/rez");
	RemoveMQ2Data("Rez");
	delete pRezType;
}

PLUGIN_API VOID SetGameState(DWORD GameState)
{
	if (gGameState == GAMESTATE_CHARSELECT) {
		Initialized = false;//will force it to load the settings from the INI once you are back in game.
	}

	if (gGameState == GAMESTATE_INGAME && InGame())
	{
		//Update the INI name.
		sprintf_s(INIFileName, "%s\\%s_%s.ini", gszINIPath, EQADDR_SERVERNAME, GetCharInfo()->Name);
	}
}

PLUGIN_API VOID OnPulse(VOID)
{
	if (!InGame())
		return;

	if (!Initialized) {
		Initialized = true;
		WriteChatf("%s\aoInitialized. Version \ag%2.2f", PLUGINMSG, VERSION);
		WriteChatf("%s\awType \ay/rez help\aw for list of commands.", PLUGINMSG);
		DoINIThings();
		ShowSettings();
	}

	if (!AutoRezAccept)
		return;

	if (++Pulse < PulseDelay)
		return;

	Pulse = 0;

	if (DoCommand && CommandPending && !IAmDead()) {
		CommandPending = false;
		EzCommand(RezCommand);
	}

	if (VoiceNotify) {
		if (!Notified && IAmDead()) {
			Notified = true;
			EzCommand("/vt 3 007");
		}
		else if (Notified && !IAmDead()) {
			Notified = false;
		}
	}

	if (ShouldTakeRez())
		AcceptRez();

	if (IAmDead()) {
		if (GetTickCount64() < AcceptedRez) {
			if (CanRespawn())
				SpawnAtCorpse();
		}
	}
}

bool ShouldTakeRez() {

	//Doesn't matter if the accept/decline is open, we want to release to bind
	if (ReleaseToBind && IAmDead())
		return true;

	CXWnd* pWnd = FindMQ2Window("ConfirmationDialogBox");
	if (pWnd && pWnd->IsVisible()) {
		CStmlWnd* textoutputwnd = (CStmlWnd*)pWnd->GetChildItem("cd_textoutput");
		if (textoutputwnd) {
			char InputCXStr[MAX_STRING] = { 0 };
			GetCXStr(textoutputwnd->STMLText, InputCXStr, MAX_STRING);
			bool bReturn = false, bOktoRez = false;
			int pct = 0;

			if (strstr(InputCXStr, " return you to your corpse")) {
				pct = 100;
				bReturn = true;
			}

			if (strstr(InputCXStr, " percent)") || bReturn) {
				if (char* pTemp = strstr(InputCXStr, "("))
					pct = atoi(&pTemp[1]);//set the pct to start on the character after the '('

				if (SafeMode) {
					if (char* pDest = strchr(InputCXStr, ' ')) {
						pDest[0] = '\0';
						if (IsGroupMember(InputCXStr) || IsFellowshipMember(InputCXStr) || IsGuildMember(InputCXStr) || IsRaidMember(InputCXStr) != -1)
							bOktoRez = true;
					}
				}
				else
					bOktoRez = true;

			}

			if (bOktoRez && pct >= AutoRezPct) {
				if (!bReturn) {
					char RezCaster[MAX_STRING] = { 0 };
					GetArg(RezCaster, InputCXStr, 1);
					if (strlen(RezCaster)) {
						if (gAnonymize && !Anonymize(RezCaster, MAX_STRING, 2)) {
							for (int i = 1; i < (int)strlen(RezCaster) - 1; i++) {
								RezCaster[i] = '*';
							}
						}

						if (!bQuiet)
							WriteChatf("%s\ayReceived a rez from \ap%s \ayfor \ag%i \aypercent. ", PLUGINMSG, RezCaster, pct);
					}
				}
				else if (!bQuiet)
					WriteChatf("%s\ayReceived a no exp call to my corpse", PLUGINMSG);

				return true;
			}
		}
	}

	return false;
}

bool IAmDead() {
	if (PSPAWNINFO Me = GetCharInfo()->pSpawn) {
		if (Me->RespawnTimer)
			return true;
	}
	return false;
}

void TheRezCommand(PSPAWNINFO pCHAR, char *szLine)
{
	char Arg[MAX_STRING];
	GetArg(Arg, szLine, 1);
	//help
	if (!_stricmp("help", Arg)) {
		DisplayHelp();
		return;
	}

	//accept
	if (!_stricmp("accept", Arg)) {
		GetArg(Arg, szLine, 2);
		if (!_stricmp("on", Arg) || !_stricmp("1", Arg)) {
			WritePrivateProfileString("MQ2Rez", "accept", "on", INIFileName);
			AutoRezAccept = true;
		}
		if (!_stricmp("off", Arg) || !_stricmp("0", Arg)) {
			WritePrivateProfileString("MQ2Rez", "accept", "off", INIFileName);
			AutoRezAccept = false;
		}
		WriteChatf("%s\ayAccept\ar(%s\ar)", PLUGINMSG, (AutoRezAccept ? "\agOn" : "\arOff"));
		return;
	}

	//voice
	if (!_stricmp("voice", Arg)) {
		GetArg(Arg, szLine, 2);
		if (!_stricmp("on", Arg) || !_stricmp("1", Arg)) {
			WritePrivateProfileString("MQ2Rez", "VoiceNotify", "1", INIFileName);
			VoiceNotify = true;
		}
		else if (!_stricmp("off", Arg) || !_stricmp("0", Arg)) {
			WritePrivateProfileString("MQ2Rez", "VoiceNotify", "0", INIFileName);
			VoiceNotify = false;
		}
		WriteChatf("%s\ayVoice\ar(%s\ar)", PLUGINMSG, (VoiceNotify ? "\agOn" : "\arOff"));
		return;
	}

	//pct
	if (!_stricmp("pct", Arg) || !_stricmp("acceptpct", Arg)) {
		GetArg(Arg, szLine, 2);
		bool valid = true;
		if (IsNumber(Arg)) {
			if (atoi(Arg) <= 100 && atoi(Arg) >= 0) {
				WritePrivateProfileString("MQ2Rez", "RezPct", Arg, INIFileName);
				AutoRezPct = atoi(Arg);
				WriteChatf("%s\ayAcceptPct\ar(\ag%i\ar)", PLUGINMSG, AutoRezPct);
				return;
			}
			else {
				valid = false;
			}
		}
		else {
			valid = false;
		}

		if (!valid) {
			WriteChatf("%s\ar That was not valid. Please select a number between 0 and 100", PLUGINMSG);
		}
		return;
	}

	//safemode
	if (!_stricmp("safemode", Arg)) {
		GetArg(Arg, szLine, 2);
		if (!_stricmp("on", Arg) || !_stricmp("1", Arg)) {
			WritePrivateProfileString("MQ2Rez", "SafeMode", "On", INIFileName);
			SafeMode = true;
		}
		if (!_stricmp("off", Arg) || !_stricmp("0", Arg)) {
			WritePrivateProfileString("MQ2Rez", "SafeMode", "Off", INIFileName);
			SafeMode = false;
		}
		WriteChatf("%s\aySafeMode\ar(%s\ar)", PLUGINMSG, (SafeMode ? "\agOn" : "\arOff"));
		return;
	}

	//setcommand
	if (!_stricmp("setcommand", Arg)) {
		GetArg(Arg, szLine, 2);
		if (!strlen(Arg)) {
			strcpy_s(Arg, "DISABLED");
		}
		WritePrivateProfileString("MQ2Rez", "Command Line", Arg, INIFileName);
		WriteChatf("%s\ayCommand set to: \ag%s\ax", PLUGINMSG, Arg);
		GetPrivateProfileString("MQ2Rez", "Command Line", 0, RezCommand, MAX_STRING, INIFileName);
		strcpy_s(RezCommand, Arg);
		if (RezCommand[0] == '\0' || !_stricmp(RezCommand, "DISABLED")) {
			DoCommand = false;
		}
		else {
			DoCommand = true;
		}
		return;
	}

	//status
	if (!_stricmp("status", Arg) || !_stricmp("settings", Arg)) {
		GetArg(Arg, szLine, 2);
		ShowSettings();
		return;
	}

	if (!_stricmp("release", Arg)) {
		GetArg(Arg, szLine, 2);
		if (!_stricmp("on", Arg) || !_stricmp("1", Arg)) {
			WritePrivateProfileString("MQ2Rez", "ReleaseToBind", "On", INIFileName);
			ReleaseToBind = true;
		}
		if (!_stricmp("off", Arg) || !_stricmp("0", Arg)) {
			WritePrivateProfileString("MQ2Rez", "ReleaseToBind", "Off", INIFileName);
			ReleaseToBind = false;
		}
		WriteChatf("%s\ayReleaseToBind\ar(%s\ar)", PLUGINMSG, (ReleaseToBind ? "\agOn" : "\arOff"));
		return;
	}

	if (!_stricmp("silent", Arg)) {
		GetArg(Arg, szLine, 2);
		if (!_stricmp("on", Arg) || !_stricmp("1", Arg)) {
			WritePrivateProfileString("MQ2Rez", "bQuiet", "On", INIFileName);
			bQuiet = true;
		}
		if (!_stricmp("off", Arg) || !_stricmp("0", Arg)) {
			WritePrivateProfileString("MQ2Rez", "bQuiet", "Off", INIFileName);
			bQuiet = false;
		}
		WriteChatf("%s\aySilent\ar(%s\ar)", PLUGINMSG, (bQuiet ? "\agOn" : "\arOff"));
		return;
	}

	WriteChatf("%s\arInvalid /rez command was used. \ayShowing help!", PLUGINMSG);
	DisplayHelp();
}

void DisplayHelp() {
	WriteChatf("%s\awUsage:", PLUGINMSG);
	WriteChatf("%s\aw/rez \ay -> displays settings", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayaccept on|off -> Toggle auto-accepting rezbox", PLUGINMSG);
	WriteChatf("%s\aw/rez \aypct # -> Autoaccepts rezes only if they are higher than # percent", PLUGINMSG);
	WriteChatf("%s\aw/rez \aysafemode on|off ->Turn on/off safe mode.", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayrelease -> Release to bind On/Off", PLUGINMSG);
	//WriteChatf("%s\aw/rez \aydelay #### -> sets time in milliseconds to wait before accepting rez.", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayvoice on/off -> Turns on voice macro \"Help\". This is local to you only.", PLUGINMSG);
	WriteChatf("%s\aw/rez \aysilent -> turn On/Off text output when recieving a rez.", PLUGINMSG);
	WriteChatf("%s\aw/rez \aysetcommand mycommand -> Set the command that you want to run after you are rezzed.", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayhelp", PLUGINMSG);
	WriteChatf("%s\aw/rez \aysettings -> Show the current settings.", PLUGINMSG);

}

inline bool InGame()
{
	return(GetGameState() == GAMESTATE_INGAME && GetCharInfo() && GetCharInfo()->pSpawn && GetCharInfo2());
}

void AcceptRez()
{
	if (!bQuiet)
		WriteChatf("%s\agAccepting Rez", PLUGINMSG);

	AcceptedRez = GetTickCount64() + 5000;
	LeftClickWnd("ConfirmationDialogBox", "Yes_Button");

	if (DoCommand)
		CommandPending = true;
}

bool CanRespawn()
{
	CXWnd* pWnd = (CXWnd*)pRespawnWnd;
	if (pWnd && pWnd->IsVisible()) {
		CListWnd* clist = (CListWnd*)pWnd->GetChildItem("OptionsList");
		CButtonWnd* cButton = (CButtonWnd*)pWnd->GetChildItem("SelectButton");
		if (clist && cButton) {
			CXStr Str;
			char szOut[MAX_STRING] = { 0 };

			for (int index = 0; index < clist->ItemsArray.Count; index++) {
				clist->GetItemText(&Str, index, 1);
				GetCXStr(Str.Ptr, szOut, MAX_STRING);

				if (!ReleaseToBind) {
					if (!_strnicmp(szOut, "Resurrect", 9)) {
						if (clist->GetCurSel() != index)
							clist->SetCurSel(index);

						break;
					}
				}
				else if (!_strnicmp(szOut, "Bind Location", 13)) {
					if (clist->GetCurSel() != index)
						clist->SetCurSel(index);

					break;
				}
			}

			if (cButton->IsEnabled()) {
				return true;
			}
		}
	}
	return false;
}

void SpawnAtCorpse()
{
	if (!bQuiet)
		WriteChatf("%s\ag Respawning", PLUGINMSG);

	AcceptedRez = GetTickCount64();
	LeftClickWnd("RespawnWnd", "RW_SelectButton");
}
void LeftClickWnd(char* MyWndName, char *MyButton) {
	CXWnd* pMyWnd = FindMQ2Window(MyWndName);
	if (pMyWnd && pMyWnd->IsVisible() && pMyWnd->IsEnabled()) {
		if (CXWnd* pWnd = pMyWnd->GetChildItem(MyButton)) {
			SendWndClick2(pWnd, "leftmouseup");
		}
	}
}

void VerifyINI(char Section[MAX_STRING], char Key[MAX_STRING], char Default[MAX_STRING])
{
	char temp[MAX_STRING] = { 0 };
	if (GetPrivateProfileString(Section, Key, 0, temp, MAX_STRING, INIFileName) == 0)
	{
		WritePrivateProfileString(Section, Key, Default, INIFileName);
	}
}

void DoINIThings() {
	char temp[MAX_STRING] = { 0 };

	VerifyINI("MQ2Rez", "Accept", "0");
	GetPrivateProfileString("MQ2Rez", "Accept", "false", temp, MAX_STRING, INIFileName);
	AutoRezAccept = atob(temp);

	VerifyINI("MQ2Rez", "RezPct", "0");
	GetPrivateProfileString("MQ2Rez", "RezPct", "96", temp, MAX_STRING, INIFileName);
	AutoRezPct = atoi(temp);

	VerifyINI("MQ2Rez", "RezDelay", "0");
	GetPrivateProfileString("MQ2Rez", "RezPct", "100", temp, MAX_STRING, INIFileName);
	RezDelay = atoi(temp);

	VerifyINI("MQ2Rez", "SafeMode", "0");
	GetPrivateProfileString("MQ2Rez", "SafeMode", "false", temp, MAX_STRING, INIFileName);
	SafeMode = atob(temp);

	VerifyINI("MQ2Rez", "VoiceNotify", "0");
	GetPrivateProfileString("MQ2Rez", "VoiceNotify", "false", temp, MAX_STRING, INIFileName);
	VoiceNotify = atob(temp);

	VerifyINI("MQ2Rez", "ReleaseToBind", "0");
	GetPrivateProfileString("MQ2Rez", "ReleaseToBind", "false", temp, MAX_STRING, INIFileName);
	ReleaseToBind = atob(temp);

	//bQuiet
	VerifyINI("MQ2Rez", "bQuiet", "0");
	GetPrivateProfileString("MQ2Rez", "bQuiet", "false", temp, MAX_STRING, INIFileName);
	bQuiet = atob(temp);

	GetPrivateProfileString("MQ2Rez", "Command Line", 0, RezCommand, MAX_STRING, INIFileName);
	if (RezCommand[0] == '\0' || !_stricmp(RezCommand, "DISABLED"))
	{
		if (_stricmp(RezCommand, "DISABLED"))
			WritePrivateProfileString("MQ2Rez", "Command Line", "DISABLED", INIFileName);
		DoCommand = false;
	}
	else
		DoCommand = true;
}

bool atob(char x[MAX_STRING])
{
	if (!_stricmp(x, "true") || atoi(x) != 0 || !_stricmp(x, "on"))
		return true;
	return false;
}

void ShowSettings() {
	WriteChatf("%s\ayAccept\ar(%s\ar)", PLUGINMSG, (AutoRezAccept ? "\agOn" : "\arOff"));
	WriteChatf("%s\ayAcceptPct\ar(\ag%i\ar)", PLUGINMSG, AutoRezPct);
	WriteChatf("%s\aySafeMode\ar(%s\ar)", PLUGINMSG, (SafeMode ? "\agOn" : "\arOff"));
	WriteChatf("%s\ayRelease to Bind\ar(%s\ar)", PLUGINMSG, (ReleaseToBind ? "\agOn" : "\arOff"));
	WriteChatf("%s\ayVoice\ar(%s\ar)", PLUGINMSG, (VoiceNotify ? "\agOn" : "\arOff"));
	WriteChatf("%s\aySilent\ar(%s\ar)", PLUGINMSG, (bQuiet ? "\agOn" : "\arOff"));

	if (RezCommand[0] == '\0' || !_stricmp(RezCommand, "DISABLED")) {
		WriteChatf("%s\arRez Command to run after rez: \agNot Set\ax.", PLUGINMSG);
	}
	else {
		WriteChatf("%s\ayCommand line set to: \ag%s\ax", PLUGINMSG, RezCommand);
	}
	return;
}
