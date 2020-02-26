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

#include <mq/Plugin.h>

PreSetup("MQ2Rez");
PLUGIN_VERSION(3.5);

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
bool bWriteINI = true;

char RezCommand[MAX_STRING] = "";

int Pulse = 0;
int PulseDelay = 20;
int AutoRezPct = 96;

uint64_t AcceptedRez = GetTickCount64();

class MQ2RezType* pRezType = nullptr;
class MQ2RezType final : public MQ2Type {
	public:
		enum Members
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

		bool GetMember(MQVarPtr VarPtr, char* Member, char* Index, MQTypeVar& Dest)
		{
			MQTypeMember* pMember = MQ2RezType::FindMember(Member);
			if (!pMember)
				return false;

			switch (static_cast<Members>(pMember->ID))
			{
				case Version:
					Dest.Float = MQ2Version;
					Dest.Type = mq::datatypes::pFloatType;
					return true;
				case Accept:
					Dest.Int = AutoRezAccept;
					Dest.Type = mq::datatypes::pBoolType;
					return true;
				case Percent:
				case Pct:
					Dest.Int = AutoRezPct;
					Dest.Type = mq::datatypes::pIntType;
					return true;
				case xSafeMode:
					Dest.Int = SafeMode;
					Dest.Type = mq::datatypes::pBoolType;
					return true;
				case Voice:
					Dest.Int = VoiceNotify;
					Dest.Type = mq::datatypes::pBoolType;
					return true;
				case Release:
					Dest.Int = ReleaseToBind;
					Dest.Type = mq::datatypes::pBoolType;
					return true;
				default:
					return false;
			}
		}

		bool FromData(MQVarPtr &VarPtr, MQTypeVar &Source) { return false; }
		bool ToString(MQVarPtr &VarPtr, char* Destination) { return true; }
		bool FromString(MQVarPtr &VarPtr, char *Source) { return false; }
};


bool dataRez(const char* szIndex, MQTypeVar& Ret)
{
	Ret.DWord = 1;
	Ret.Type = pRezType;
	return true;
}

bool IAmDead() {
	if (PSPAWNINFO Me = GetCharInfo()->pSpawn) {
		if (Me->RespawnTimer) {
			return true;
		}
	}
	return false;
}

bool ShouldTakeRez()
{
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
							else if (IsRaidMember(name.c_str())) {
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
						char mutableConfirmationText[MAX_STRING] = { 0 };
						strcpy_s(mutableConfirmationText, confirmationText.c_str());
						char RezCaster[MAX_STRING] = { 0 };
						GetArg(RezCaster, mutableConfirmationText, 1);
						if (RezCaster[0] == '\0') {
							strcpy_s(RezCaster, "Unknown");
						}
						WriteChatf("%s\ayReceived a rez from \ap%s \ayfor \ag%i \aypercent. ", PLUGINMSG, RezCaster, pct);
					}
					else {
						WriteChatf("%s\ayReceived a no exp call to my corpse", PLUGINMSG);
					}
					return true;
				}
			}
		}
		else if (ReleaseToBind) {
			if (IAmDead()) {
				return true;
			}
		}
	}
	return false;
}

void DisplayHelp()
{
	WriteChatf("%s\awUsage:", PLUGINMSG);
	WriteChatf("%s\aw/rez \ay -> displays settings", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayaccept on|off -> Toggle auto-accepting rezbox", PLUGINMSG);
	WriteChatf("%s\aw/rez \aypct # -> Autoaccepts rezes only if they are higher than # percent", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayrelease -> Release to bind On/Off", PLUGINMSG);
	//WriteChatf("%s\aw/rez \aydelay #### -> sets time in milliseconds to wait before accepting rez.", PLUGINMSG);
	WriteChatf("%s\aw/rez \aysetcommand mycommand -> Set the command that you want to run after you are rezzed.", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayvoice on/off -> Turns on voice macro \"Help\" when in group", PLUGINMSG);
	WriteChatf("%s\aw/rez \ayhelp", PLUGINMSG);
	WriteChatf("%s\aw/rez \aysettings -> Show the current settings.", PLUGINMSG);
}

void ShowSettings()
{
	WriteChatf("%s\ayAccept\ar(%s\ar)", PLUGINMSG, (AutoRezAccept ? "\agOn" : "\arOff"));
	WriteChatf("%s\ayAcceptPct\ar(\ag%i\ar)", PLUGINMSG, AutoRezPct);
	WriteChatf("%s\aySafeMode\ar(%s\ar)", PLUGINMSG, (SafeMode ? "\agOn" : "\arOff"));
	WriteChatf("%s\ayVoice\ar(%s\ar)", PLUGINMSG, (VoiceNotify ? "\agOn" : "\arOff"));
	WriteChatf("%s\ayRelease to Bind\ar(%s\ar)", PLUGINMSG, (ReleaseToBind ? "\agOn" : "\arOff"));
	if (RezCommand[0] == '\0' || !_stricmp(RezCommand, "DISABLED")) {
		WriteChatf("%s\arRez Command to run after rez: \agNot Set\ax.", PLUGINMSG);
	}
	else {
		WriteChatf("%s\ayCommand line set to: \ag%s\ax", PLUGINMSG, RezCommand);
	}
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
			AutoRezAccept = true;
		}
		else if (!_stricmp("off", Arg) || !_stricmp("0", Arg)) {
			AutoRezAccept = false;
		}
		WritePrivateProfileString("MQ2Rez", "accept", std::to_string(AutoRezAccept), INIFileName);
		WriteChatf("%s\ayAccept\ar(%s\ar)", PLUGINMSG, (AutoRezAccept ? "\agOn" : "\arOff"));
		return;
	}

	//voice
	if (!_stricmp("voice", Arg)) {
		GetArg(Arg, szLine, 2);
		if (!_stricmp("on", Arg) || !_stricmp("1", Arg)) {
			VoiceNotify = true;
		}
		else if (!_stricmp("off", Arg) || !_stricmp("0", Arg)) {
			VoiceNotify = false;
		}
		WritePrivateProfileString("MQ2Rez", "VoiceNotify", std::to_string(VoiceNotify), INIFileName);
		WriteChatf("%s\ayVoice\ar(%s\ar)", PLUGINMSG, (VoiceNotify ? "\agOn" : "\arOff"));
		return;

	}

	//pct
	if (!_stricmp("pct", Arg) || !_stricmp("acceptpct", Arg)) {
		GetArg(Arg, szLine, 2);
		int argTmp = GetIntFromString(Arg, AutoRezPct);
		if (argTmp >= 0 && argTmp <= 100) {
			AutoRezPct = argTmp;
			WritePrivateProfileString("MQ2Rez", "RezPct", std::to_string(AutoRezPct), INIFileName);
			WriteChatf("%s\ayAcceptPct\ar(\ag%i\ar)", PLUGINMSG, AutoRezPct);
		}
		else {
			WriteChatf("%s\ar Accept Percent not a valid percentage: %d", PLUGINMSG, argTmp);
		}
		return;
	}

	//safemode
	if (!_stricmp("safemode", Arg)) {
		GetArg(Arg, szLine, 2);
		if (!_stricmp("on", Arg) || !_stricmp("1", Arg)) {
			SafeMode = true;
		}
		if (!_stricmp("off", Arg) || !_stricmp("0", Arg)) {
			SafeMode = false;
		}
		WritePrivateProfileString("MQ2Rez", "SafeMode", std::to_string(SafeMode), INIFileName);
		WriteChatf("%s\aySafeMode\ar(%s\ar)", PLUGINMSG, (SafeMode ? "\agOn" : "\arOff"));
		return;
	}

	//setcommand
	if (!_stricmp("setcommand", Arg)) {
		GetArg(Arg, szLine, 2);
		WritePrivateProfileString("MQ2Rez", "Command Line", Arg, INIFileName);
		WriteChatf("%s\ayCommand set to: \ag%s\ax", PLUGINMSG, Arg);
		GetPrivateProfileString("MQ2Rez", "Command Line", RezCommand, RezCommand, MAX_STRING, INIFileName);
		strcpy_s(RezCommand, Arg);
		if (RezCommand[0] == '\0' || !_stricmp(RezCommand, "DISABLED")) {
			bDoCommand = false;
		}
		else {
			bDoCommand = true;
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
			ReleaseToBind = true;
		}
		if (!_stricmp("off", Arg) || !_stricmp("0", Arg)) {
			ReleaseToBind = false;
		}
		WritePrivateProfileString("MQ2Rez", "ReleaseToBind", std::to_string(ReleaseToBind), INIFileName);
		WriteChatf("%s\ayReleaseToBind\ar(%s\ar)", PLUGINMSG, (ReleaseToBind ? "\agOn" : "\arOff"));
		return;
	}

	WriteChatf("%s\arInvalid /rez command was used. \ayShowing help!", PLUGINMSG);
	DisplayHelp();
}

inline bool InGame()
{
	return(GetGameState() == GAMESTATE_INGAME && GetCharInfo() && GetCharInfo()->pSpawn && GetPcProfile());
}

bool CanRespawn()
{
	if (CSidlScreenWnd *pWnd = (CSidlScreenWnd *)pRespawnWnd) {
		if (pWnd->IsVisible()) {
			if (CListWnd* clist = (CListWnd*)pWnd->GetChildItem("OptionsList")) {
				if (CButtonWnd* cButton = (CButtonWnd*)pWnd->GetChildItem("SelectButton")) {
					CXStr Str;
					for (int index = 0; index < clist->ItemsArray.Count; index++) {
						if (!ReleaseToBind) {
							if (!clist->GetItemText(index, 1).CompareN("Resurrect", 9)) {
								if (clist->GetCurSel() != index)
									clist->SetCurSel(index);
								break;
							}
						}
						else {
							if (!clist->GetItemText(index, 1).CompareN("Bind Location", 13)) {
								if (clist->GetCurSel() != index)
									clist->SetCurSel(index);
								break;
							}
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

void LeftClickWnd(PCHAR MyWndName, PCHAR MyButton) {
	if (CSidlScreenWnd* pMyWnd = (CSidlScreenWnd*)FindMQ2Window(MyWndName)) {
		if (pMyWnd->IsVisible() && pMyWnd->IsEnabled()) {
			if (CXWnd* pWnd = pMyWnd->GetChildItem(MyButton)) {
				SendWndClick2(pWnd, "leftmouseup");
			}
		}
	}
}

void AcceptRez()
{
	WriteChatf("%s\agAccepting Rez", PLUGINMSG);
	AcceptedRez = GetTickCount64() + 5000;
	LeftClickWnd("ConfirmationDialogBox", "Yes_Button");
	if (bDoCommand) CommandPending = true;
}

void SpawnAtCorpse()
{
	WriteChatf("%s\ag Respawning", PLUGINMSG);
	AcceptedRez = GetTickCount64();
	LeftClickWnd("RespawnWnd", "RW_SelectButton");
}

// FIXME: No reason to use MAX_STRING here.  Or char for that matter.
void VerifyINI(char Section[MAX_STRING], char Key[MAX_STRING], char Default[MAX_STRING])
{
	char temp[MAX_STRING] = { 0 };
	if (GetPrivateProfileString(Section, Key, 0, temp, MAX_STRING, INIFileName) == 0)
	{
		WritePrivateProfileString(Section, Key, Default, INIFileName);
	}
}

void DoINIThings() {
	AutoRezAccept = GetPrivateProfileBool("MQ2Rez", "Accept", AutoRezAccept, INIFileName);
	AutoRezPct = GetPrivateProfileInt("MQ2Rez", "RezPct", AutoRezPct, INIFileName);
	SafeMode = GetPrivateProfileBool("MQ2Rez", "SafeMode", SafeMode, INIFileName);
	VoiceNotify = GetPrivateProfileBool("MQ2Rez", "VoiceNotify", VoiceNotify, INIFileName);
	ReleaseToBind = GetPrivateProfileBool("MQ2Rez", "ReleaseToBind", ReleaseToBind, INIFileName);

	GetPrivateProfileString("MQ2Rez", "Command Line", RezCommand, RezCommand, MAX_STRING, INIFileName);
	if (RezCommand[0] == '\0' || !_stricmp(RezCommand, "DISABLED"))
	{
		bDoCommand = false;
	}
	else
	{
		bDoCommand = true;
	}

	if (bWriteINI)
	{
		WritePrivateProfileString("MQ2Rez", "Accept", std::to_string(AutoRezAccept), INIFileName);
		WritePrivateProfileString("MQ2Rez", "RezPct", std::to_string(AutoRezPct), INIFileName);
		WritePrivateProfileString("MQ2Rez", "SafeMode", std::to_string(SafeMode), INIFileName);
		WritePrivateProfileString("MQ2Rez", "VoiceNotify", std::to_string(VoiceNotify), INIFileName);
		WritePrivateProfileString("MQ2Rez", "ReleaseToBind", std::to_string(ReleaseToBind), INIFileName);
		WritePrivateProfileString("MQ2Rez", "Command Line", RezCommand, INIFileName);
		bWriteINI = false;
	}

}

PLUGIN_API VOID InitializePlugin(VOID)
{
	AddCommand("/rez", TheRezCommand);
	pRezType = new MQ2RezType;
	AddMQ2Data("Rez", dataRez);
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
	RemoveCommand("/rez");
	RemoveMQ2Data("Rez");
	delete pRezType;
}

PLUGIN_API VOID SetGameState(DWORD GameState)
{
	if (!InGame()) return;
	if (gGameState == GAMESTATE_INGAME)
	{
		//Update the INI name.
		sprintf_s(INIFileName, "%s\\%s_%s.ini", gPathConfig, EQADDR_SERVERNAME, GetCharInfo()->Name);
	}
}

PLUGIN_API VOID OnPulse(VOID)
{
	if (!InGame()) return;
	if (!Initialized) {
		Initialized = true;
		WriteChatf("%s\aoInitialized. Version \ag%2.2f", PLUGINMSG, MQ2Version);
		WriteChatf("%s\awType \ay/rez help\aw for list of commands.", PLUGINMSG);
		DoINIThings();
		ShowSettings();
	}
	if (!AutoRezAccept) return;
	if (++Pulse < PulseDelay) return;
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