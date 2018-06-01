#pragma once
#include <Windows.h>
#include <locale.h>

__declspec (thread) static void *pThread = NULL;

class ProfileStructher
{
#define Max 50		//���⿡���� �� �뵵 �̹Ƿ� class���ο��� ������ �ش�.
#define ThreadMax 10

private:
	struct Profile
	{
		bool flag;
		WCHAR Name[64];
		
		bool Beginflag;		//�������� ���ۿ���
		__int64 Start_Time;	//���۽ð�
		__int64 TotalTime;	//��� �ð��� �հ�
		__int64 Min_Time[2];	//0���� �ּ� �ɸ��ð�. �ʹ� ������. 1���� �� ������
		__int64 Max_Time[2];	//0���� �ִ� �ɸ� �ð�. �ʹ� ū ��. 1���� �� ���� ū��.
		unsigned __int64 CallCNT;	//call�� Ƚ��.
	};
	struct ProfileThread
	{
		bool flag;
			
		DWORD ThreadID;
		Profile profile_Array[Max];
	};
	ProfileThread Thread[ThreadMax];

	LARGE_INTEGER SecondFrequency;
	double NanoSecond;

public:
	ProfileStructher(void)
	{
		int cnt;
		for ( int TCnt = 0; TCnt < ThreadMax; TCnt++ )
		{
			Thread[TCnt].ThreadID = 0;
			Thread[TCnt].flag = false;
			for ( cnt = 0; cnt < Max; cnt++ )
			{
				Thread[TCnt].profile_Array[cnt].flag = false;
				Thread[TCnt].profile_Array[cnt].Name[0] = NULL;
				Thread[TCnt].profile_Array[cnt].TotalTime = 0;
				Thread[TCnt].profile_Array[cnt].Min_Time[0] = 0x7fffffffffffffff;		//�ּ� �ɸ� �ð��̹Ƿ� �ִ밪���� �о��ش�.
				Thread[TCnt].profile_Array[cnt].Min_Time[1] = 0x7fffffffffffffff;
				Thread[TCnt].profile_Array[cnt].Max_Time[0] = 0;		//�ִ� �ɸ� �ð��̹Ƿ� �ּҰ����� �о��ش�.
				Thread[TCnt].profile_Array[cnt].Max_Time[1] = 0;
				Thread[TCnt].profile_Array[cnt].CallCNT = 0;		//ȣ��Ƚ�� �ʱ�ȭ
			}
		}

		QueryPerformanceFrequency (&SecondFrequency);
		NanoSecond = (double) SecondFrequency.QuadPart / 1000000000;
		setlocale (LC_ALL, "");

		_CStartTime = GetTickCount64 ();
	}
	~ProfileStructher (void)
	{

	}
	bool Set_Profile (WCHAR *name, __int64 SetTime);	//����ȵ� �������� �� ��� false ����
	bool End_Profile (WCHAR *name, __int64 EndTime);	//���ų� ���۵��� ���� �������� �ϰ�� false ����.
	void Print_Profile (void);		//����� �������ϵ��� ���Ϸ� �����.
	void ClearProfile (void);		//��� ���������� �ʱ�ȭ ���ѹ�����.




	INT64 _CStartTime;
};

void Profile_Begin (WCHAR *name);
void Profile_End (WCHAR *name);
void PROFILE_KeyProc (void);


//�����
//#define PROFILE_CHECK

#ifdef PROFILE_CHECK
#define PROFILE_BEGIN(X)	Profile_Begin(X)
#define PROFILE_END(X)		Profile_End(X)
#define PROFILE_KEYPROC()		PROFILE_KeyProc()

#else
#define PROFILE_BEGIN(X)
#define PROFILE_END(X)
#define PROFILE_KEYPROC()

#endif