#pragma once
#include "RezType.h"
#include "MQ2Rez.h"
#include "../MQ2Plugin.h"

MQ2RezType* pRezType = nullptr;

BOOL dataRez(PCHAR szIndex, MQ2TYPEVAR& Ret)
{
	Ret.DWord = 1;
	Ret.Type = pRezType;
	return true;
}

MQ2RezType::MQ2RezType() : MQ2Type("Rez")
{
	TypeMember(Version);//1,Float
	TypeMember(Accept);//2,Bool
	TypeMember(Percent);//3,Int
	TypeMember(Pct);//4,Int
	AddMember(xSafeMode, "SafeMode");//5,Bool
	TypeMember(Voice);//6,Bool
	TypeMember(Release);//7,Bool
};

MQ2RezType::~MQ2RezType()
{
}

bool MQ2RezType::GetMember(MQ2VARPTR VarPtr, PCHAR Member, PCHAR Index, MQ2TYPEVAR& Dest)
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
	break;
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

bool MQ2RezType::ToString(MQ2VARPTR VarPtr, PCHAR Destination)
{
	return true;
}

bool MQ2RezType::FromData(MQ2VARPTR& VarPtr, MQ2TYPEVAR& Source)
{
	return false;
}
bool MQ2RezType::FromString(MQ2VARPTR& VarPtr, PCHAR Source)
{
	return false;
}