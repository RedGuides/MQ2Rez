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
#include "../MQ2Plugin.h"

PreSetup("MQ2Rez");
PLUGIN_VERSION(3.8);

constexpr auto PLUGINMSG = "\aw[\agMQ2Rez\aw]\ao:: ";

//Variables
bool AutoRezAccept = false;
bool CommandPending = false;
bool bDoCommand = false;
bool Initialized = false;
bool SafeMode = false;
bool VoiceNotify = false;
bool Notified = true;
bool ReleaseToBind = false;
bool bQuiet = false;

char RezCommand[MAX_STRING] = { 0 };

int AutoRezPct = 96;

uint64_t AcceptedRez = GetTickCount64();

enum class eINIOptions
{
	WriteOnly,
	ReadAndWrite,
	ReadOnly
};

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
		TypeMember(Version);
		TypeMember(Accept);
		TypeMember(Percent);
		TypeMember(Pct);
		AddMember(xSafeMode, "SafeMode");
		TypeMember(Voice);
		TypeMember(Release);
	};

	~MQ2RezType(){}

	bool GetMember(MQ2VARPTR VarPtr, char* Member, char* Index, MQ2TYPEVAR& Dest)
	{
		PMQ2TYPEMEMBER pMember = MQ2RezType::FindMember(Member);
		if (!pMember)
			return false;
		switch ((RezMembers)pMember->ID)
		{
			case Version:
				Dest.Float = MQ2Version;
				Dest.Type = pFloatType;
				return true;
			case Accept:
				Dest.Int = AutoRezAccept;
				Dest.Type = pBoolType;
				return true;
			case Percent:
			case Pct:
				Dest.Int = AutoRezPct;
				Dest.Type = pIntType;
				return true;
			case xSafeMode:
				Dest.Int = SafeMode;
				Dest.Type = pBoolType;
				return true;
			case Voice:
				Dest.Int = VoiceNotify;
				Dest.Type = pBoolType;
				return true;
			case Release:
				Dest.Int = ReleaseToBind;
				Dest.Type = pBoolType;
				return true;
			default:
				return false;
		}
	}

	bool FromData(MQ2VARPTR& VarPtr, MQ2TYPEVAR& Source) { return false; }
	bool ToString(MQ2VARPTR VarPtr, char* Destination) { return true; }
	bool FromString(MQ2VARPTR& VarPtr, char* Source) { return false; }
};

MQ2RezType* pRezType = nullptr;

int dataRez(char* szIndex, MQ2TYPEVAR& Ret)
{
	Ret.DWord = 1;
	Ret.Type = pRezType;
	return TRUE;
}

bool atob(const char* x)
{
	if (!_stricmp(x, "true") || !_stricmp(x, "on") || atoi(x) != 0)
		return true;
	return false;
}

bool IAmDead()
{
	if (PSPAWNINFO Me = GetCharInfo()->pSpawn) {
		if (Me->RespawnTimer)
			return true;
	}
	return false;
}

bool ShouldTakeRez()
{
	//Doesn't matter if the accept/decline is open, we want to release to bind
	if (ReleaseToBind && IAmDead())
		return true;

	CXWnd* pWnd = FindMQ2Window("ConfirmationDialogBox", true);
	if (pWnd && pWnd->IsVisible()) {
		CStmlWnd* textoutputwnd = (CStmlWnd*)pWnd->GetChildItem("cd_textoutput");
		if (textoutputwnd) {
			char InputCXStr[MAX_STRING] = { 0 };
			GetCXStr(textoutputwnd->STMLText.Ptr, InputCXStr, MAX_STRING);
			bool bReturn = false;
			bool bOktoRez = false;
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
				else {
					bOktoRez = true;
				}

			}

			if (bOktoRez && pct >= AutoRezPct) {
				if (!bReturn) {
					char RezCaster[MAX_STRING] = { 0 };
					GetArg(RezCaster, InputCXStr, 1);
					if (RezCaster[0] == '\0') {
						strcpy_s(RezCaster, "Unknown");
					}
					else {
						if (gAnonymize && !Anonymize(RezCaster, MAX_STRING, 2)) {
							for (int i = 1; i < (int)strlen(RezCaster) - 1; i++) {
								RezCaster[i] = '*';
							}
						}
					}

					if (!bQuiet) {
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

void DisplayHelp()
{
	WriteChatf("%s\awUsage:", PLUGINMSG);
	WriteChatf("%s\aw/rez \ay [help] -> displays this help message", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayaccept on|off -> Toggle auto-accepting rezbox", PLUGINMSG);
	WriteChatf("%s\aw/rez \aypct # -> Autoaccepts rezes only if they are higher than # percent", PLUGINMSG);
	WriteChatf("%s\aw/rez \aysafemode on|off ->Turn on/off safe mode.", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayrelease -> Release to bind On/Off", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayvoice on/off -> Turns on voice macro \"Help\". This is local to you only.", PLUGINMSG);
	WriteChatf("%s\aw/rez \aysilent -> turn On/Off text output when recieving a rez.", PLUGINMSG);
	WriteChatf("%s\aw/rez \aysetcommand mycommand -> Set the command that you want to run after you are rezzed.", PLUGINMSG);
	WriteChatf("%s\aw/rez \aysettings -> Show the current settings.", PLUGINMSG);
}

void ShowSettings() {
	WriteChatf("%s\ayAccept \ar(%s\ar)", PLUGINMSG, AutoRezAccept ? "\agOn" : "\arOff");
	WriteChatf("%s\ayAcceptPct \ar(\ag%i\ar)", PLUGINMSG, AutoRezPct);
	WriteChatf("%s\aySafeMode \ar(%s\ar)", PLUGINMSG, SafeMode ? "\agOn" : "\arOff");
	WriteChatf("%s\ayRelease to Bind \ar(%s\ar)", PLUGINMSG, ReleaseToBind ? "\agOn" : "\arOff");
	WriteChatf("%s\ayVoice \ar(%s\ar)", PLUGINMSG, VoiceNotify ? "\agOn" : "\arOff");
	WriteChatf("%s\aySilent \ar(%s\ar)", PLUGINMSG, bQuiet ? "\agOn" : "\arOff");
	WriteChatf("%s\ayCommand to run after rez: \ag%s\ax", PLUGINMSG, bDoCommand ? RezCommand : "\arDISABLED");
}

void DoINIThings(eINIOptions Operation)
{
	char temp[MAX_STRING] = { 0 };

	if (Operation == eINIOptions::ReadOnly || Operation == eINIOptions::ReadAndWrite)
	{
		GetPrivateProfileString("MQ2Rez", "Accept", AutoRezAccept ? "On" : "Off", temp, MAX_STRING, INIFileName);
		AutoRezAccept = atob(temp);

		errno_t tmpErr = _itoa_s(AutoRezPct, temp, 10);
		if (tmpErr != 0) {
			strcpy_s(temp, "96");
		}
		GetPrivateProfileString("MQ2Rez", "RezPct", temp, temp, MAX_STRING, INIFileName);
		errno = 0;
		char* endptr;
		int i = strtol(temp, &endptr, 10);
		if (errno != ERANGE && endptr != temp && i >= 0 && i <= 100) {
			AutoRezPct = i;
		} else {
			AutoRezPct = 96;
		}

		GetPrivateProfileString("MQ2Rez", "SafeMode", SafeMode ? "On" : "Off", temp, MAX_STRING, INIFileName);
		SafeMode = atob(temp);

		GetPrivateProfileString("MQ2Rez", "VoiceNotify", VoiceNotify ? "On" : "Off", temp, MAX_STRING, INIFileName);
		VoiceNotify = atob(temp);

		GetPrivateProfileString("MQ2Rez", "ReleaseToBind", ReleaseToBind ? "On" : "Off", temp, MAX_STRING, INIFileName);
		ReleaseToBind = atob(temp);

		GetPrivateProfileString("MQ2Rez", "SilentMode", bQuiet ? "On" : "Off", temp, MAX_STRING, INIFileName);
		bQuiet = atob(temp);

		GetPrivateProfileString("MQ2Rez", "Command Line", "DISABLED", RezCommand, MAX_STRING, INIFileName);
		if (RezCommand[0] == '\0' || !_stricmp(RezCommand, "DISABLED"))
		{
			strcpy_s(RezCommand, "DISABLED");
			bDoCommand = false;
		}
		else
		{
			bDoCommand = true;
		}
	}

	if (Operation == eINIOptions::WriteOnly || Operation == eINIOptions::ReadAndWrite)
	{
		WritePrivateProfileString("MQ2Rez", "Accept", AutoRezAccept ? "On" : "Off", INIFileName);

		errno_t tmpErr = _itoa_s(AutoRezPct, temp, 10);
		if (tmpErr != 0) {
			strcpy_s(temp, "96");
		}
		WritePrivateProfileString("MQ2Rez", "RezPct", temp, INIFileName);

		WritePrivateProfileString("MQ2Rez", "SafeMode", SafeMode ? "On" : "Off", INIFileName);
		WritePrivateProfileString("MQ2Rez", "VoiceNotify", VoiceNotify ? "On" : "Off", INIFileName);
		WritePrivateProfileString("MQ2Rez", "ReleaseToBind", ReleaseToBind ? "On" : "Off", INIFileName);
		WritePrivateProfileString("MQ2Rez", "SilentMode", bQuiet ? "On" : "Off", INIFileName);
		WritePrivateProfileString("MQ2Rez", "Command Line", RezCommand[0] == '\0' ? "DISABLED" : RezCommand, INIFileName);
	}

}

void TheRezCommand(PSPAWNINFO pCHAR, char* szLine)
{
	char Arg[MAX_STRING] = { 0 };
	GetArg(Arg, szLine, 1);
	bool bWriteIni = true;

	if (!_stricmp("help", Arg)) {
		DisplayHelp();
		bWriteIni = false;
	}
	else if (!_stricmp("status", Arg) || !_stricmp("settings", Arg)) {
		ShowSettings();
		bWriteIni = false;
	}
	else if (!_stricmp("accept", Arg)) {
		GetArg(Arg, szLine, 2);
		AutoRezAccept = atob(Arg);
		WriteChatf("%s\ayAccept\ar (%s\ar)", PLUGINMSG, (AutoRezAccept ? "\agOn" : "\arOff"));
	}
	else if (!_stricmp("voice", Arg)) {
		GetArg(Arg, szLine, 2);
		VoiceNotify = atob(Arg);
		WriteChatf("%s\ayVoice\ar (%s\ar)", PLUGINMSG, (VoiceNotify ? "\agOn" : "\arOff"));
	}
	else if (!_stricmp("pct", Arg) || !_stricmp("acceptpct", Arg)) {
		GetArg(Arg, szLine, 2);
		errno = 0;
		char* endptr;
		int userInput = strtol(Arg, &endptr, 10);
		if (errno != ERANGE && endptr != Arg && userInput >= 0 && userInput <= 100) {
			AutoRezPct = userInput;
			WriteChatf("%s\ayAcceptPct\ar (\ag%i\ar)", PLUGINMSG, AutoRezPct);
		}
		else {
			bWriteIni = false;
			WriteChatf("%s\ar Accept Percent not a valid percentage (0 through 100): %s", PLUGINMSG, Arg);
		}
	}
	else if (!_stricmp("safemode", Arg)) {
		GetArg(Arg, szLine, 2);
		SafeMode = atob(Arg);
		WriteChatf("%s\aySafeMode\ar (%s\ar)", PLUGINMSG, (SafeMode ? "\agOn" : "\arOff"));
	}
	else if (!_stricmp("setcommand", Arg)) {
		GetArg(Arg, szLine, 2, 1);
		if (Arg[0] == '\0' || !_stricmp(Arg, "DISABLED")) {
			strcpy_s(RezCommand, "DISABLED");
			bDoCommand = false;
		}
		else {
			// If this is a quoted command, just let GetArg handle it.
			if (Arg[0] == '"') {
				GetArg(RezCommand, szLine, 2);
			}
			// Otherwise the command is the rest of the line
			else {
				char* szTemp = GetNextArg(szLine, 1);
				strcpy_s(RezCommand, szTemp);
				
			}
			bDoCommand = true;
		}
		WriteChatf("%s\ayCommand to run after rez set to: \ag%s\ax", PLUGINMSG, bDoCommand ? RezCommand : "\arDISABLED");
	}
	else if (!_stricmp("release", Arg)) {
		GetArg(Arg, szLine, 2);
		ReleaseToBind = atob(Arg);
		WriteChatf("%s\ayReleaseToBind\ar (%s\ar)", PLUGINMSG, (ReleaseToBind ? "\agOn" : "\arOff"));
	}
	else if (!_stricmp("silent", Arg)) {
		GetArg(Arg, szLine, 2);
		bQuiet = atob(Arg);
		WriteChatf("%s\aySilent\ar (%s\ar)", PLUGINMSG, (bQuiet ? "\agOn" : "\arOff"));
	}
	else
	{
		WriteChatf("%s\arInvalid /rez command was used. \ayShowing help!", PLUGINMSG);
		DisplayHelp();
		bWriteIni = false;
	}

	if (bWriteIni)
	{
		DoINIThings(eINIOptions::WriteOnly);
	}

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

void LeftClickWnd(char* MyWndName, char* MyButton) {
	CXWnd* pMyWnd = FindMQ2Window(MyWndName, true);
	if (pMyWnd && pMyWnd->IsVisible() && pMyWnd->IsEnabled()) {
		if (CXWnd* pWnd = pMyWnd->GetChildItem(MyButton)) {
			SendWndClick2(pWnd, "leftmouseup");
		}
	}
}

void AcceptRez()
{
	if (!bQuiet)
		WriteChatf("%s\agAccepting Rez", PLUGINMSG);

	AcceptedRez = GetTickCount64() + 5000;
	LeftClickWnd("ConfirmationDialogBox", "Yes_Button");

	if (bDoCommand)
		CommandPending = true;
}

void SpawnAtCorpse()
{
	if (!bQuiet)
		WriteChatf("%s\ag Respawning", PLUGINMSG);

	AcceptedRez = GetTickCount64();
	LeftClickWnd("RespawnWnd", "RW_SelectButton");
}

PLUGIN_API void InitializePlugin()
{
	AddCommand("/rez", TheRezCommand);
	pRezType = new MQ2RezType;
	AddMQ2Data("Rez", dataRez);
}

PLUGIN_API void ShutdownPlugin()
{
	RemoveCommand("/rez");
	RemoveMQ2Data("Rez");
	delete pRezType;
}

PLUGIN_API void SetGameState(DWORD GameState)
{
	if (GameState == GAMESTATE_CHARSELECT) {
		Initialized = false; //will force it to load the settings from the INI once you are back in game.
	}
}

PLUGIN_API void OnPulse()
{
	static int Pulse = 0;

	if (!Initialized)
	{
		if (GetGameState() == GAMESTATE_INGAME && GetCharInfo())
		{
			//Update the INI name.
			sprintf_s(INIFileName, "%s\\%s_%s.ini", gszINIPath, EQADDR_SERVERNAME, GetCharInfo()->Name);
			WriteChatf("MQ2Rez INI: %s", INIFileName);
			WriteChatf("%s\aoInitialized. Version \ag%.2f", PLUGINMSG, MQ2Version);
			WriteChatf("%s\awType \ay/rez help\aw for list of commands.", PLUGINMSG);
			DoINIThings(eINIOptions::ReadAndWrite);
			Initialized = true;
		}
		else
		{
			return;
		}
	}

	if (GetGameState() != GAMESTATE_INGAME)
		return;

	if (!AutoRezAccept)
		return;

	// Process every 20 pulses
	if (++Pulse < 20)
		return;

	Pulse = 0;

	if (bDoCommand && CommandPending && !IAmDead()) {
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
