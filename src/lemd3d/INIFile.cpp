#include "stdafx.h"

U32 NemuGetProfileInt(LPCSTR pszSection, LPCSTR pszKey, U32 nDefault, LPCSTR pszProfileName)
{
	CHAR pszDefault[32];
	CHAR pszDest[32];
	U32 nDest;
	BOOL bSucceeded;

	itoa(nDefault, pszDefault, 10);
	bSucceeded = GetPrivateProfileString(pszSection, pszKey, pszDefault, pszDest, 256, pszProfileName);
	nDest = atoi(pszDest);
	return nDest;
}