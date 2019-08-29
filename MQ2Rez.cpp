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
//

#define PLUGINMSG "\aw[\agMQ2Rez\aw]\ao:: "
#define PLUGIN_NAME "MQ2Rez"
#include "../../MQ2Plugin.h"
PreSetup(PLUGIN_NAME);
float VERSION = 3.4f;
PLUGIN_VERSION(VERSION);

//Variables
bool AutoRezAccept = false;
bool CommandPending = false;
bool DoCommand = false;
bool Initialized = false;
bool SafeMode = false;
bool VoiceNotify = false;
bool Notified = true;

char RezCommand[MAX_STRING] = "";


int Pulse = 0;
int PulseDelay = 20;
int AutoRezPct = 0;

unsigned __int64 AcceptedRez = GetTickCount64();

//Not using rezdelay stuff. 
ULONGLONG RezDelay = 100;
ULONGLONG RezDelayTimer = 0;

//Prototypes
bool atob(char x[MAX_STRING]);
bool CanRespawn();
bool IAmDead();
bool ShouldTakeRez();
inline bool InGame();

void AcceptRez();
void DisplayHelp();
void DoINIThings();
void LeftClickWnd(PCHAR MyWndName, PCHAR MyButton);
void ShowSettings();
void SpawnAtCorpse();
void TheRezCommand(PSPAWNINFO pCHAR, PCHAR zLine);

PLUGIN_API VOID InitializePlugin(VOID)
{
	AddCommand("/rez", TheRezCommand);
}
PLUGIN_API VOID ShutdownPlugin(VOID)
{
	RemoveCommand("/rez");
}
PLUGIN_API VOID SetGameState(DWORD GameState)
{
	if (!InGame()) return;
	if (gGameState == GAMESTATE_INGAME)
	{
		//Update the INI name.
		sprintf_s(INIFileName, "%s\\%s_%s.ini", gszINIPath, EQADDR_SERVERNAME, GetCharInfo()->Name);
	}
}
PLUGIN_API VOID OnPulse(VOID)
{
	if (!InGame()) return;
	if (!Initialized) {
		Initialized = true;
		WriteChatf("%s\aoInitialized. Version \ag%2.2f", PLUGINMSG, VERSION);
		WriteChatf("%s\awType \ay/rez help\aw for list of commands.", PLUGINMSG);
		DoINIThings();
		ShowSettings();
	}
	if (!AutoRezAccept) return;
	if (++Pulse < PulseDelay) return;
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
	if (CSidlScreenWnd *pWnd = (CSidlScreenWnd *)FindMQ2Window("ConfirmationDialogBox")) {
		if (pWnd->IsVisible()) {
			if (CStmlWnd *Child = (CStmlWnd*)pWnd->GetChildItem("cd_textoutput")) {
				std::string confirmationText{ Child->STMLText };
				BOOL bReturn = FALSE;
				int pct = 0;
				BOOL bOktoRez = FALSE;
				if (confirmationText.find(" return you to your corpse") != std::string::npos) {
					pct = 100;
					bReturn = TRUE;
				}
				if (confirmationText.find(" percent)") != std::string::npos || bReturn) {
					auto iBracket = confirmationText.find("(");
					if (iBracket != std::string::npos) {
						pct = std::stoi(confirmationText.substr(iBracket + 1));
					}
					if (SafeMode) {
						auto iSpace = confirmationText.find(' ');
						if (iSpace != std::string::npos) {
							auto name = confirmationText.substr(0, iSpace);
							if (IsGroupMember(name.c_str())) {
								bOktoRez = TRUE;
							}
							else if (IsFellowshipMember(name.c_str())) {
								bOktoRez = TRUE;
							}
							else if (IsGuildMember(name.c_str())) {
								bOktoRez = TRUE;
							}
							else if (IsRaidMember(name.c_str()) != -1) {
								bOktoRez = TRUE;
							}
						}
					}
					else {
						bOktoRez = TRUE;
					}
				}
				if (bOktoRez && pct >= AutoRezPct) {
					if (!bReturn) {
						char RezCaster[MAX_STRING] = "";
						GetArg(RezCaster, confirmationText.c_str(), 1);
						if (strlen(RezCaster)) {
							if (gAnonymize) {
								int len = strlen(RezCaster);
								for (int i = 1; i < len - 1; i++) {
									RezCaster[i] = '*';
								}
								WriteChatf("%s\ayReceived a rez from \ap%s \ayfor \ag%i \aypercent. ", PLUGINMSG, RezCaster, pct);
							}
							else
								WriteChatf("%s\ayReceived a rez from \ap%s \ayfor \ag%i \aypercent. ", PLUGINMSG, RezCaster, pct);

						}

					}
					else {
						WriteChatf("%s\ayReceived a no exp call to my corpse", PLUGINMSG);
					}
					return true;
				}
			}
		}
	}
	return false;
}
bool IAmDead() {
	if (PSPAWNINFO Me = GetCharInfo()->pSpawn) {
		if (Me->RespawnTimer) return true;
	}
	return false;
}
void TheRezCommand(PSPAWNINFO pCHAR, PCHAR szLine)
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
			WriteChatf("%s\ar That was not a valid percentage", PLUGINMSG);
		}
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

	WriteChatf("%s\arInvalid /rez command was used. \ayShowing help!", PLUGINMSG);
	DisplayHelp();
}
void DisplayHelp() {
	WriteChatf("%s\awUsage:", PLUGINMSG);
	WriteChatf("%s\aw/rez \ay -> displays settings", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayaccept on|off -> Toggle auto-accepting rezbox", PLUGINMSG);
	WriteChatf("%s\aw/rez \aypct # -> Autoaccepts rezes only if they are higher than # percent", PLUGINMSG);
	//WriteChatf("%s\aw/rez \aydelay #### -> sets time in milliseconds to wait before accepting rez.", PLUGINMSG);
	WriteChatf("%s\aw/rez \aysetcommand mycommand -> Set the command that you want to run after you are rezzed.", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayvoice on/off -> Turns on voice macro \"Help\" when in group", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayhelp", PLUGINMSG);
	WriteChatf("%s\aw/rez \aysettings -> Show the current settings.", PLUGINMSG);
}
inline bool InGame()
{
	return(GetGameState() == GAMESTATE_INGAME && GetCharInfo() && GetCharInfo()->pSpawn && GetCharInfo2());
}
void AcceptRez()
{
	WriteChatf("%s\agAccepting Rez", PLUGINMSG);
	AcceptedRez = GetTickCount64() + 5000;
	LeftClickWnd("ConfirmationDialogBox", "Yes_Button");
	if (DoCommand) CommandPending = true;
}
bool CanRespawn()
{
	if (CSidlScreenWnd *pWnd = (CSidlScreenWnd *)pRespawnWnd) {
		if (pWnd->IsVisible()) {
			if (CListWnd* clist = (CListWnd*)pWnd->GetChildItem("OptionsList")) {
				if (CButtonWnd* cButton = (CButtonWnd*)pWnd->GetChildItem("SelectButton")) {
					CXStr Str;
					CHAR szOut[MAX_STRING] = { 0 };
					for (int index = 0; index < clist->ItemsArray.Count; index++) {
						if (!clist->GetItemText(index, 1).CompareN("Resurrect", 9)) {
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
		}
	}
	return false;
}
void SpawnAtCorpse()
{
	WriteChatf("%s\ag Respawning", PLUGINMSG);
	AcceptedRez = GetTickCount64();
	LeftClickWnd("RespawnWnd", "RW_SelectButton");
}
void LeftClickWnd(PCHAR MyWndName, PCHAR MyButton) {
	if (CSidlScreenWnd* pMyWnd = (CSidlScreenWnd*)FindMQ2Window(MyWndName)) {
		if (pMyWnd->IsVisible() && pMyWnd->IsEnabled()) {
			if (CXWnd* pWnd = pMyWnd->GetChildItem(MyButton)) {
				SendWndClick2(pWnd, "leftmouseup");
			}
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
	for (int i = 0; i < 4; i++)
		x[i] = tolower(x[i]);
	if (!_stricmp(x, "true") || atoi(x) != 0 || !_stricmp(x, "on"))
		return true;
	return false;
}

void ShowSettings() {
	WriteChatf("%s\ayAccept\ar(%s\ar)", PLUGINMSG, (AutoRezAccept ? "\agOn" : "\arOff"));
	WriteChatf("%s\ayAcceptPct\ar(\ag%i\ar)", PLUGINMSG, AutoRezPct);
	WriteChatf("%s\aySafeMode\ar(%s\ar)", PLUGINMSG, (SafeMode ? "\agOn" : "\arOff"));
	WriteChatf("%s\ayVoice\ar(%s\ar)", PLUGINMSG, (VoiceNotify ? "\agOn" : "\arOff"));
	if (RezCommand[0] == '\0' || !_stricmp(RezCommand, "DISABLED")) {
		WriteChatf("%s\arRez Command to run after rez: \agNot Set\ax.", PLUGINMSG);
	}
	else {
		WriteChatf("%s\ayCommand line set to: \ag%s\ax", PLUGINMSG, RezCommand);
	}
	return;
}