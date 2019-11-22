#pragma once
#include "../MQ2Plugin.h"



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

	MQ2RezType();
	~MQ2RezType();
	bool GetMember(MQ2VARPTR VarPtr, PCHAR Member, PCHAR Index, MQ2TYPEVAR& Dest);
	bool ToString(MQ2VARPTR VarPtr, PCHAR Destination);
	bool FromData(MQ2VARPTR& VarPtr, MQ2TYPEVAR& Source);
	bool FromString(MQ2VARPTR& VarPtr, PCHAR Source);

};
extern MQ2RezType* pRezType;

BOOL dataRez(PCHAR szIndex, MQ2TYPEVAR& Ret);