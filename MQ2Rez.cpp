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
#include <mq/Plugin.h>

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

		virtual bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest) override
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
		virtual bool FromString(MQVarPtr &VarPtr, const char* Source) override { return false; }
};


bool dataRez(const char* szIndex, MQTypeVar& Ret)
{
	Ret.DWord = 1;
	Ret.Type = pRezType;
	return true;
}

bool IAmDead()
{
	if (PSPAWNINFO Me = GetCharInfo()->pSpawn) {
		if (Me->RespawnTimer) {
			return true;
		}
	}
	return false;
}

bool ShouldTakeRez()
{
	//Doesn't matter if the accept/decline is open, we want to release to bind
	if (ReleaseToBind && IAmDead())
		return true;

	CXWnd* pWnd = FindMQ2Window("ConfirmationDialogBox");
	if (pWnd && pWnd->IsVisible()) {
		CStmlWnd* textoutputwnd = (CStmlWnd*)pWnd->GetChildItem("cd_textoutput");
		if (textoutputwnd) {
			std::string confirmationText{ textoutputwnd->STMLText };
			bool bReturn = false;
			bool bOktoRez = false;
			int pct = 0;

			if (confirmationText.find(" return you to your corpse") != std::string::npos) {
				pct = 100;
				bReturn = true;
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
						if (IsGroupMember(name.c_str()) || IsFellowshipMember(name.c_str()) || IsGuildMember(name.c_str()) || IsRaidMember(name.c_str())) {
							bOktoRez = true;
						}
					}
				}
				else {
					bOktoRez = true;
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

					if (!bQuiet) {
						WriteChatf("%s\ayReceived a rez from \ap%s \ayfor \ag%i \aypercent. ", PLUGINMSG, RezCaster, pct);
					}
				}
				else if (!bQuiet) {
					WriteChatf("%s\ayReceived a no exp call to my corpse", PLUGINMSG);
				}

				return true;
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

void ShowSettings()
{
	WriteChatf("%s\ayAccept \ar(%s\ar)", PLUGINMSG, AutoRezAccept ? "\agOn" : "\arOff");
	WriteChatf("%s\ayAcceptPct \ar(\ag%i\ar)", PLUGINMSG, AutoRezPct);
	WriteChatf("%s\aySafeMode \ar(%s\ar)", PLUGINMSG, SafeMode ? "\agOn" : "\arOff");
	WriteChatf("%s\ayRelease to Bind \ar(%s\ar)", PLUGINMSG, ReleaseToBind ? "\agOn" : "\arOff");
	WriteChatf("%s\ayVoice \ar(%s\ar)", PLUGINMSG, VoiceNotify ? "\agOn" : "\arOff");
	WriteChatf("%s\aySilent \ar(%s\ar)", PLUGINMSG, bQuiet ? "\agOn" : "\arOff");
	WriteChatf("%s\ayCommand to run after rez: \ag%s\ax", PLUGINMSG, bDoCommand ? RezCommand : "\arDISABLED");
}

void DoINIThings(const eINIOptions Operation)
{
	if (Operation == eINIOptions::ReadOnly || Operation == eINIOptions::ReadAndWrite)
	{
		AutoRezAccept = GetPrivateProfileBool("MQ2Rez", "Accept", AutoRezAccept, INIFileName);
		AutoRezPct = GetPrivateProfileInt("MQ2Rez", "RezPct", AutoRezPct, INIFileName);
		SafeMode = GetPrivateProfileBool("MQ2Rez", "SafeMode", SafeMode, INIFileName);
		VoiceNotify = GetPrivateProfileBool("MQ2Rez", "VoiceNotify", VoiceNotify, INIFileName);
		ReleaseToBind = GetPrivateProfileBool("MQ2Rez", "ReleaseToBind", ReleaseToBind, INIFileName);
		bQuiet = GetPrivateProfileBool("MQ2Rez", "SilentMode", bQuiet, INIFileName);

		GetPrivateProfileString("MQ2Rez", "Command Line", RezCommand, RezCommand, MAX_STRING, INIFileName);
		if (RezCommand[0] == '\0' || !_stricmp(RezCommand, "DISABLED"))
		{
			bDoCommand = false;
		}
		else
		{
			bDoCommand = true;
		}
	}

	if (Operation == eINIOptions::WriteOnly || Operation == eINIOptions::ReadAndWrite)
	{
		WritePrivateProfileBool("MQ2Rez", "Accept", AutoRezAccept, INIFileName);
		WritePrivateProfileInt("MQ2Rez", "RezPct", AutoRezPct, INIFileName);
		WritePrivateProfileBool("MQ2Rez", "SafeMode", SafeMode, INIFileName);
		WritePrivateProfileBool("MQ2Rez", "VoiceNotify", VoiceNotify, INIFileName);
		WritePrivateProfileBool("MQ2Rez", "ReleaseToBind", ReleaseToBind, INIFileName);
		WritePrivateProfileBool("MQ2Rez", "SilentMode", bQuiet, INIFileName);
		WritePrivateProfileString("MQ2Rez", "Command Line", RezCommand, INIFileName);
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
		AutoRezAccept = GetBoolFromString(Arg, AutoRezAccept);
		WriteChatf("%s\ayAccept\ar (%s\ar)", PLUGINMSG, (AutoRezAccept ? "\agOn" : "\arOff"));
	}
	else if (!_stricmp("voice", Arg)) {
		GetArg(Arg, szLine, 2);
		VoiceNotify = GetBoolFromString(Arg, VoiceNotify);
		WriteChatf("%s\ayVoice\ar (%s\ar)", PLUGINMSG, (VoiceNotify ? "\agOn" : "\arOff"));
	}
	else if (!_stricmp("pct", Arg) || !_stricmp("acceptpct", Arg)) {
		GetArg(Arg, szLine, 2);
		int userInput = GetIntFromString(Arg, AutoRezPct);
		if (userInput >= 0 && userInput <= 100) {
			AutoRezPct = userInput;
			WriteChatf("%s\ayAcceptPct\ar (\ag%i\ar)", PLUGINMSG, AutoRezPct);
		}
		else {
			bWriteIni = false;
			WriteChatf("%s\ar Accept Percent not a valid percentage (0 through 100): %i", PLUGINMSG, userInput);
		}
	}
	else if (!_stricmp("safemode", Arg)) {
		GetArg(Arg, szLine, 2);
		SafeMode = GetBoolFromString(Arg, SafeMode);
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
		ReleaseToBind = GetBoolFromString(Arg, ReleaseToBind);
		WriteChatf("%s\ayReleaseToBind\ar (%s\ar)", PLUGINMSG, (ReleaseToBind ? "\agOn" : "\arOff"));
	}
	else if (!_stricmp("silent", Arg)) {
		GetArg(Arg, szLine, 2);
		bQuiet = GetBoolFromString(Arg, ReleaseToBind);
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
	if (CSidlScreenWnd *pWnd = (CSidlScreenWnd *)pRespawnWnd) {
		if (pWnd->IsVisible()) {
			if (CListWnd* clist = (CListWnd*)pWnd->GetChildItem("OptionsList")) {
				if (CButtonWnd* cButton = (CButtonWnd*)pWnd->GetChildItem("SelectButton")) {
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

void LeftClickWnd(char* MyWndName, char* MyButton) {
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
	if (!bQuiet) {
		WriteChatf("%s\agAccepting Rez", PLUGINMSG);
	}

	AcceptedRez = GetTickCount64() + 5000;
	LeftClickWnd("ConfirmationDialogBox", "Yes_Button");

	if (bDoCommand) {
		CommandPending = true;
	}
}

void SpawnAtCorpse()
{
	if (!bQuiet) {
		WriteChatf("%s\ag Respawning", PLUGINMSG);
	}

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

PLUGIN_API void SetGameState(int GameState)
{
	if (GameState == GAMESTATE_CHARSELECT) {
		Initialized = false; //will force it to load the settings from the INI once you are back in game.
	}
}

PLUGIN_API void OnPulse()
{
	static int Pulse = 0;

	if (GetGameState() != GAMESTATE_INGAME || !pCharData)
		return;

	if (!Initialized)
	{
		//Update the INI name.
		sprintf_s(INIFileName, "%s\\%s_%s.ini", gPathConfig, EQADDR_SERVERNAME, pCharData->Name);
		WriteChatf("%s\aoInitialized. Version \ag%.2f", PLUGINMSG, MQ2Version);
		WriteChatf("%s\awType \ay/rez help\aw for list of commands.", PLUGINMSG);
		DoINIThings(eINIOptions::ReadAndWrite);
		Initialized = true;
	}

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
