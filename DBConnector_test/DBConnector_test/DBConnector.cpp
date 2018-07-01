#include"DBConnector.h"
#include <strsafe.h>

Hitchhiker::CDBConnector::CDBConnector (WCHAR * szDBIP, WCHAR * szUser, WCHAR * szPassword, WCHAR * szDBName, int iDBPort)
{
	/*
	WCHAR		_szDBIP[16];
	WCHAR		_szDBUser[64];
	WCHAR		_szDBPassword[64];
	WCHAR		_szDBName[64];
	int			_iDBPort;
	*/
	StringCchCopy (_szDBIP,sizeof(_szDBIP),szDBIP);
	StringCchCopy (_szDBUser, sizeof (_szDBUser), szUser);
	StringCchCopy (_szDBPassword, sizeof (_szDBPassword), szPassword);
	StringCchCopy (_szDBName, sizeof (_szDBName), szDBName);
	_iDBPort = iDBPort;

}
