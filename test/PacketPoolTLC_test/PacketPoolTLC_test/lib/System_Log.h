#pragma once


#include <wchar.h>
#include <stdarg.h>
#include <time.h>
#include <locale.h>
#include <Strsafe.h>
#include <Windows.h>

enum en_LOG_LEVEL
{
	LOG_DEBUG = 0,
	LOG_WARNING,
	LOG_ERROR,
	LOG_SYSTEM,
};


namespace hiker
{

	class CSystemLog
	{
#define FileNameLength 128
#define HeaderLength 128
#define LogstrLength 1024
#define HEXLength 1024
#define SessionLength 32
	private:

		CSystemLog (en_LOG_LEVEL Level)
		{
			InitializeSRWLock (&_srwLock);
			setlocale (LC_ALL, "");
			_SaveLogLevel = Level;
			_LogNo = 0;
			_windowprint = false;
			return;
		}
		~CSystemLog (void)
		{

		}

		static CSystemLog *pLog;
	public:

		//------------------------------------------------------
		// �̱��� Ŭ����, 
		//------------------------------------------------------
		static CSystemLog *GetInstance (en_LOG_LEVEL Level)
		{
			if ( pLog == NULL )
			{
				pLog = new CSystemLog(Level);
			}
			return pLog;
		}

		//------------------------------------------------------
		// �ܺο��� �α׷��� ����
		//------------------------------------------------------
		void SetLogLevel (en_LOG_LEVEL LogLevel, BOOL CONSOLE = false)
		{
			_SaveLogLevel = LogLevel;
			_windowprint = CONSOLE;
			return;
		}

		//------------------------------------------------------
		// �α� ��� ����.
		//------------------------------------------------------
		void SetLogDirectory (WCHAR *szDirectory)
		{
			_wmkdir (szDirectory);
			wsprintf (_SaveDirectory, L"%s\\", szDirectory);
		}

		//------------------------------------------------------
		// ���� �α� ����� �Լ�.
		//------------------------------------------------------
		void Log (WCHAR *szType, en_LOG_LEVEL LogLevel, WCHAR *szStringFormat, ...)
		{

			WCHAR FileName[FileNameLength];
			WCHAR Header[HeaderLength];
			WCHAR Logstr[LogstrLength] = { 0, };
			va_list va;
			FILE *fp;

			if ( LogLevel < _SaveLogLevel )
			{
				return;
			}


			HeaderSetting (FileName, Header, szType, LogLevel);

			va_start (va, szStringFormat);
			int hResult = StringCchVPrintf (Logstr, LogstrLength, szStringFormat, va);
			va_end (va);


			// �α� ���� ������ ���� ���� ���н� ���зα� ����ó��.
			if ( FAILED (hResult) )
			{
				WCHAR buff[LogstrLength];
				wsprintf (buff, L"LOG_BUFFER_OVERFLOW Data = ");
				StringCchCat (buff, LogstrLength - 31, Logstr);

				wsprintf (Logstr, buff);
			}


			AcquireSRWLockExclusive (&_srwLock);

			_wfopen_s (&fp, FileName, L"a+t,ccs=UNICODE");

			fwprintf_s (fp, Header);
			fwprintf_s (fp, Logstr);

			fclose (fp);

			ReleaseSRWLockExclusive (&_srwLock);

			if ( _windowprint )
			{
				Time = time (NULL);
				localtime_s (&t, &Time);

				wprintf (L"[%04d-%02d-%02d %02d:%02d:%02d] %s \n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, Logstr);
			}
			

		}
		
		//------------------------------------------------------
		// BYTE ���̳ʸ��� ���� �α� ���
		//------------------------------------------------------
		void LogHex (WCHAR *szType, en_LOG_LEVEL LogLevel, WCHAR *szLog, BYTE *pByte, int iByteLen)
		{


			WCHAR FileName[FileNameLength];
			WCHAR Header[HeaderLength];
			WCHAR Logstr[LogstrLength] = { 0, };
			WCHAR HEX[HEXLength] = { 0, };
			FILE *fp;

			if ( LogLevel < _SaveLogLevel )
			{
				return;
			}

			HeaderSetting (FileName, Header, szType, LogLevel);


			//���⼭ ���̳ʸ� �α� ��Ʈ�� ����.
			WCHAR buff[48];
			int retval;
			int cnt;

			retval = StringCchPrintf (Logstr,LogstrLength,L"%s ", szLog);
			// �α� ���� ������ ���� ���� ���н� ���зα� ����ó��.
			if ( FAILED (retval) )
			{
				WCHAR Logbuff[LogstrLength];
				wsprintf (Logbuff, L"[ LOG_BUFFER_OVERFLOW ] [ Data ] = ");
				StringCchCat (Logbuff, LogstrLength - 31, Logstr);

				wsprintf (Logstr, Logbuff);
			}



			//HEX �α� ��Ʈ�� ����.
			for ( cnt = 0; cnt < iByteLen; cnt++ )
			{
				retval = 0;
				if ( iByteLen - 1 == cnt )
				{
					wsprintf (buff,L"%02X", pByte[cnt]);
				}
				else
				{
					wsprintf (buff, L"%02X ", pByte[cnt]);
				}

				retval = StringCchCat (HEX, HEXLength, buff);

				if ( FAILED (retval) )
				{
					WCHAR failbuff[HEXLength];
					wsprintf (failbuff, L"[ HEX_BUFFER_OVERFLOW ]  [ Data ] = ");
					StringCchCat (failbuff, HEXLength - 31, HEX);

					wsprintf (HEX, failbuff);
					break;
				}
			}

			AcquireSRWLockExclusive (&_srwLock);

			_wfopen_s (&fp, FileName, L"a+t,ccs=UNICODE");

			fwprintf_s (fp, Header);
			fwprintf_s (fp, Logstr);
			fwprintf_s (fp, HEX);

			fclose (fp);

			ReleaseSRWLockExclusive (&_srwLock);

			if ( _windowprint )
			{
				Time = time (NULL);
				localtime_s (&t, &Time);
				wprintf (L"[%04d-%02d-%02d %02d:%02d:%02d] %s %s \n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, Logstr, HEX);
			}
		}




		//------------------------------------------------------
		// SessionKey 64bit ��� ����.
		//------------------------------------------------------
		void LogSessionKey (WCHAR *szType, en_LOG_LEVEL LogLevel, WCHAR *szLog, BYTE *pSessionKey)
		{


			WCHAR FileName[FileNameLength];
			WCHAR Header[HeaderLength];
			WCHAR Logstr[LogstrLength] = { 0, };
			WCHAR SessionKey[SessionLength] = { 0, };
			FILE *fp;

			if ( LogLevel < _SaveLogLevel )
			{
				return;
			}

			HeaderSetting (FileName, Header, szType, LogLevel);


			//���⼭ ���̳ʸ� �α� ��Ʈ�� ����.
			int retval;

			retval = StringCchPrintf (Logstr, LogstrLength, L"%s ", szLog);
			// �α� ���� ������ ���� ���� ���н� ���зα� ����ó��.
			if ( FAILED (retval) )
			{
				WCHAR Logbuff[LogstrLength];
				wsprintf (Logbuff, L"[ LOG_BUFFER_OVERFLOW ] [ Data ] = ");
				StringCchCat (Logbuff, LogstrLength - 31, Logstr);

				wsprintf (Logstr, Logbuff);
			}


			//HEX �α� ��Ʈ�� ����.
			unsigned long long *pKey = (unsigned long long *)pSessionKey;
			wsprintf (SessionKey, L"%X ", *pKey);


			AcquireSRWLockExclusive (&_srwLock);

			_wfopen_s (&fp, FileName, L"a+t,ccs=UNICODE");

			fwprintf_s (fp, Header);
			fwprintf_s (fp, Logstr);
			fwprintf_s (fp, SessionKey);

			fclose (fp);

			ReleaseSRWLockExclusive (&_srwLock);

			if ( _windowprint )
			{
				Time = time (NULL);
				localtime_s (&t, &Time);
				wprintf (L"[%04d-%02d-%02d %02d:%02d:%02d] %s %s \n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, Logstr, SessionKey);
			}
		}
		

	private:

		__int64	_LogNo;
		SRWLOCK			_srwLock;

		en_LOG_LEVEL	_SaveLogLevel;
		WCHAR			_SaveDirectory[128];
		BOOL _windowprint;

		time_t Time;
		struct tm t;

		

		/*=================================
		//�α� ������ ���ϸ�� �� ���� ��� ����� �Լ�.
		=================================*/
		void HeaderSetting (WCHAR *FileName, WCHAR *pHeader, WCHAR *szType, en_LOG_LEVEL LogLevel)
		{
			Time = time (NULL);
			localtime_s (&t, &Time);

			memset (FileName, 0, FileNameLength * sizeof(WCHAR));
			memset (pHeader, 0, HeaderLength * sizeof (WCHAR));

			wsprintf (FileName, L"%s\\%-4d_%-1d_%ls.txt", _SaveDirectory, t.tm_year + 1900, t.tm_mon + 1, szType);

			__int64 No = InterlockedIncrement64 (&_LogNo);


			wsprintf (pHeader, L"\n[%08I64d] [ %-5s ] [%04d-%02d-%02d %02d:%02d:%02d", No, szType, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

			switch ( LogLevel )
			{
			case LOG_DEBUG:
				StringCchCat (pHeader, HeaderLength, L" / DEBUG ]  ");
				break;

			case LOG_WARNING:
				StringCchCat (pHeader, HeaderLength, L" / WARNING ]  ");
				break;
			case LOG_ERROR:
				StringCchCat (pHeader, HeaderLength, L" / ERROR ]  ");
				break;
			case LOG_SYSTEM:
				StringCchCat (pHeader, HeaderLength, L" / SYSTEM ]  ");
				break;
			}
			return;
		}
	};
	//CSystemLog *CSystemLog::pLog = nullptr;
}



/*====================================================================
//�����ϰ� ����ϱ� ���� ��ũ�η� ����
====================================================================*/
#define SYSLOG_ON

#ifdef SYSLOG_ON 
extern hiker::CSystemLog *Log;
#define LOG_DIRECTORY(dir)	Log->SetLogDirectory (dir);
#define LOG_LEVEL(level,console)	Log->SetLogLevel(level,console);
#define LOG_LOG(type,level,fmt,...) Log->Log(type,level,fmt,__VA_ARGS__);
#define LOG_LOGHEX(type,level,strLog,pByte,ByteLength)	Log->LogHex(type,level,strLog,pByte,ByteLength);
#define LOG_LOGSession(type, level, strLog, pSessionKey)	Log->LogSessionKey (type, level, strLog, pSessionKey);

#else

#define LOG_DIRECTORY(dir)
#define LOG_LEVEL(level,console)
#define LOG_LOG(type,level,fmt,...)
#define LOG_LOGHEX(type,level,strLog,pByte,ByteLength)
#define LOG_LOGSession(type, level, strLog, pSessionKey)
#endif
