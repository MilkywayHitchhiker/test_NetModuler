#include "stdafx.h"
#include"DBConnector.h"
#include <strsafe.h>
#include <locale.h>
#include"lib\Library.h"

Hitchhiker::CDBConnector::CDBConnector (WCHAR * szDBIP, WCHAR * szUser, WCHAR * szPassword, WCHAR * szDBName, int iDBPort, int ReConnect)
{
	WideCharToMultiByte (CP_UTF8, 0, szDBIP, lstrlenW (szUser), _szDBIP, 16, NULL, NULL);
	WideCharToMultiByte (CP_UTF8, 0, szUser, lstrlenW (szUser), _szDBUser, 64, NULL, NULL);
	WideCharToMultiByte (CP_UTF8, 0, szPassword, lstrlenW (szPassword), _szDBPassword, 64, NULL, NULL);
	WideCharToMultiByte (CP_UTF8, 0, szDBName, lstrlenW (szDBName), _szDBName, 64, NULL, NULL);

	_iDBPort = iDBPort;
	_iReconnect = ReConnect;
	_iLastError = NULL;
	_ReTry = 0;
	_wsetlocale (LC_ALL, L"korean");
	return;
}

Hitchhiker::CDBConnector::~CDBConnector ()
{
	Disconnect ();
}

bool Hitchhiker::CDBConnector::Connect (void)
{

	//초기화
	mysql_init (&_MySQL);

	//MySQL 서버와 connection 연결 작업
	_pMySQL = mysql_real_connect (&_MySQL, _szDBIP, _szDBUser, _szDBPassword, _szDBName, _iDBPort, ( char* )NULL, 0);

	//Connection이 NULL일 경우
	if ( _pMySQL == NULL )
	{
		_iLastError = mysql_errno (_pMySQL);
		char ErrLogBuf[128];
		StringCbCopyA (ErrLogBuf, lstrlenA (ErrLogBuf), mysql_error (&_MySQL));
		MultiByteToWideChar (CP_UTF8, 0, ErrLogBuf, strlen (ErrLogBuf), _szLastErrorMsg, lstrlen(_szLastErrorMsg));

		LOG_LOG (L"DBClass", LOG_ERROR, L"MySQL Connection Errno :%d, %s",_iLastError, ErrLogBuf);

		return false;
	}

	//한글사용을위해추가.
	mysql_set_character_set (_pMySQL, "utf8");
	return true;
}

bool Hitchhiker::CDBConnector::Disconnect (void)
{
	if ( _pMySQL == NULL )
	{
		return false;
	}
	mysql_close (_pMySQL);
	return true;
}

bool Hitchhiker::CDBConnector::Query (WCHAR * szStringFormat, ...)
{
	//쿼리 스트링 작성.
	WCHAR WBufString[eQUERY_MAX_LEN];
	char BufStr[eQUERY_MAX_LEN];
	int Query_stat;
	va_list ap;

	va_start (ap, szStringFormat);
	StringCchVPrintf (WBufString, eQUERY_MAX_LEN, szStringFormat, ap);
	va_end (ap);

	StringCbCopy (_szQuery, eQUERY_MAX_LEN, WBufString);

	WideCharToMultiByte (CP_UTF8, 0, WBufString, eQUERY_MAX_LEN, BufStr, eQUERY_MAX_LEN, NULL, NULL);
	StringCbCopyA (_szQueryUTF8, eQUERY_MAX_LEN, BufStr);

	//DB로 전송.
	Query_stat = mysql_query (_pMySQL, BufStr);

	//0이 아니라면 에러.
	if ( Query_stat != 0 )
	{
		char ErrLogBuf[128];

		//에러는 코드와 메시지 둘다 가지고 있을것.
		_iLastError = mysql_errno (_pMySQL);
		StringCbCopyA (ErrLogBuf, lstrlenA (ErrLogBuf), mysql_error (&_MySQL));
		MultiByteToWideChar (CP_UTF8, 0, ErrLogBuf, strlen (ErrLogBuf), _szLastErrorMsg, lstrlen (_szLastErrorMsg));

		if ( _iLastError == CR_SOCKET_CREATE_ERROR ||
			_iLastError == CR_CONNECTION_ERROR ||
			_iLastError == CR_CONN_HOST_ERROR ||
			_iLastError == CR_SERVER_GONE_ERROR ||
			_iLastError == CR_TCP_CONNECTION ||
			_iLastError == CR_SERVER_HANDSHAKE_ERR ||
			_iLastError == CR_SERVER_LOST ||
			_iLastError == CR_INVALID_CONN_HANDLE )
		{
			while ( !Connect () )
			{
				_ReTry++;
				if ( _ReTry > _iReconnect )
				{
					return false;
				}
			}
		}
	}

	//일반쿼리는 result를 멤버로 보관.
	_pSqlResult = mysql_store_result (_pMySQL);


	return true;
}

bool Hitchhiker::CDBConnector::Query_Save (WCHAR * szStringFormat, ...)
{
	//쿼리 스트링 작성.
	WCHAR WBufString[eQUERY_MAX_LEN];
	char BufStr[eQUERY_MAX_LEN];
	int Query_stat;
	va_list ap;

	StringCchVPrintf (WBufString, eQUERY_MAX_LEN, szStringFormat, ap);
	StringCbCopy (_szQuery, eQUERY_MAX_LEN, WBufString);

	WideCharToMultiByte (CP_UTF8, 0, WBufString, eQUERY_MAX_LEN, BufStr, eQUERY_MAX_LEN, NULL, NULL);
	StringCbCopyA (_szQueryUTF8, eQUERY_MAX_LEN, BufStr);

	//DB로 전송.
	Query_stat = mysql_query (_pMySQL, BufStr);

	//0이 아니라면 에러.
	if ( Query_stat != 0 )
	{
		char ErrLogBuf[128];

		//에러는 코드와 메시지 둘다 가지고 있을것.
		_iLastError = mysql_errno (_pMySQL);
		StringCbCopyA (ErrLogBuf, lstrlenA (ErrLogBuf), mysql_error (&_MySQL));
		MultiByteToWideChar (CP_UTF8, 0, ErrLogBuf, strlen (ErrLogBuf), _szLastErrorMsg, lstrlen (_szLastErrorMsg));

		if ( _iLastError == CR_SOCKET_CREATE_ERROR ||
			_iLastError == CR_CONNECTION_ERROR ||
			_iLastError == CR_CONN_HOST_ERROR ||
			_iLastError == CR_SERVER_GONE_ERROR ||
			_iLastError == CR_TCP_CONNECTION ||
			_iLastError == CR_SERVER_HANDSHAKE_ERR ||
			_iLastError == CR_SERVER_LOST ||
			_iLastError == CR_INVALID_CONN_HANDLE )
		{
			while ( !Connect () )
			{
				_ReTry++;
				if ( _ReTry > _iReconnect )
				{
					return false;
				}
			}
		}
	}

	//Save쿼리는 result를 멤버로 보관하지 않음.
	return true;
}


MYSQL_ROW Hitchhiker::CDBConnector::FetchRow (void)
{
	return mysql_fetch_row (_pSqlResult);
}

void Hitchhiker::CDBConnector::FreeResult (void)
{
	mysql_free_result (_pSqlResult);
	return;
}

void Hitchhiker::CDBConnector::SaveLastError (void)
{
	LOG_LOG (L"DBClass", LOG_ERROR, L"MySQL Connection Errno :%d, %ls", _iLastError, _szLastErrorMsg);
	return;
}
