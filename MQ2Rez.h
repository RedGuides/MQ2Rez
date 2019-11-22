#pragma once
#include "../MQ2Plugin.h"

extern float VERSION;

//Variables
extern bool AutoRezAccept;
extern bool CommandPending;
extern bool DoCommand;
extern bool Initialized;
extern bool SafeMode;
extern bool VoiceNotify;
extern bool Notified;
extern bool ReleaseToBind;

extern char RezCommand[MAX_STRING];


extern int Pulse;
extern int PulseDelay;
extern int AutoRezPct;

extern unsigned __int64 AcceptedRez;

//Not using rezdelay stuff. 
extern ULONGLONG RezDelay;
extern ULONGLONG RezDelayTimer;

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