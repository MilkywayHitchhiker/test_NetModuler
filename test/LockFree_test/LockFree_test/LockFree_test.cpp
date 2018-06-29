// LockFree_test.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "lib\Library.h"
#include "LockFreeStack.h"
#include "MemoryPool.h"


#define Thread_Max 100
#define Enq_Max 75
#define Deq_Max 25
#define Data1 0x5555777788889999

#define SleepEnq 2
#define SleepDeq 10

struct test
{
	INT64 Data;
	INT64 Cnt;
};
CStack_LF<test *> Queue;
CMemoryPool_LF<test> MemPool(0);

//unsigned int WINAPI WorkerThread (LPVOID pParam);
unsigned int WINAPI EnQThread (LPVOID pParam);
unsigned int WINAPI DeQThread (LPVOID pParam);

HANDLE Thread[Thread_Max];
bool EndEvent;
UINT EnqTPS;
UINT DeqTPS;

int main()
{
	int Cnt = 0;
	INT64 Num = 0;
	int Alloc = 0;
	int Free = 0;
	int Full = 0;

	LOG_DIRECTORY (L"LOG");
	LOG_LEVEL (LOG_DEBUG,false);
	LOG_LOG (L"LF_Queue", LOG_SYSTEM, L"TEST Start");
	EndEvent = false;

	for ( int Cnt = 0; Cnt < Deq_Max; Cnt++ )
	{
		Thread[Cnt] = ( HANDLE )_beginthreadex (NULL, 0, DeQThread, NULL, NULL, NULL);
	}

	for ( int Cnt = Deq_Max; Cnt < Deq_Max+ Enq_Max; Cnt++ )
	{
		Thread[Cnt] = ( HANDLE )_beginthreadex (NULL, 0, EnQThread, NULL, NULL, NULL);
	}

	while ( 1 )
	{

		wprintf (L"Queue Use Node Cnt = %lld\n", Num);
		wprintf (L"MemPool Alloc Node Cnt = %d\n", Alloc);
		wprintf (L"MemPool Free Node Cnt = %d\n", Free);
		wprintf (L"MemPool Full Node Cnt = %d\n\n", Full);

		wprintf (L"EnqTPS = %d\n\n", EnqTPS);
		wprintf (L"DeqTPS = %d\n\n", DeqTPS);
		Num = Queue.GetUseSize ();
		Alloc = MemPool.GetAllocCount ();
		Free = MemPool.GetFreeCount ();
		Full = MemPool.GetFullCount ();
		EnqTPS = 0;
		DeqTPS = 0;

		Sleep (1000);



		if ( GetAsyncKeyState ('E') & 0x8001 )
		{

			EndEvent = true;

			break;
		}
	}

	WaitForMultipleObjects (Thread_Max, Thread, TRUE, INFINITE);
	LOG_LOG (L"LF_Queue", LOG_SYSTEM, L"TEST End");

    return 0;
}

/*
unsigned int WINAPI WorkerThread (LPVOID pParam)
{
	int Cnt;
	test *p[NodeMAX];
	DWORD retval;

	while ( 1 )
	{
		for ( Cnt = 0; Cnt < NodeMAX; Cnt++ )
		{
			p[Cnt] = MemPool.Alloc ();
			p[Cnt]->Cnt = 0;
			p[Cnt]->Data = Data1;
			if ( Queue.Push (p[Cnt]) == false )
			{
				LOG_LOG (L"LF_Queue", LOG_WARNING, L"EnQ Max. OverFlow");
				CCrashDump::Crash ();
			}
		}



		Sleep(SleepEnq);



		test *p2;
		for ( Cnt = 0; Cnt < NodeMAX; Cnt++ )
		{
			if ( Queue.Pop (&p2) == false )
			{
				break;
			}
			if ( p2->Cnt != 0 )
			{
				LOG_LOG (L"LF_Queue", LOG_WARNING, L"DeQ Error Pointer = 0x%x, Cnt = %lld", p2, p2->Cnt);
				CCrashDump::Crash ();
			}
			if ( p2->Data != Data1 )
			{
				LOG_LOG (L"LF_Queue", LOG_WARNING, L"DeQ Error Pointer = 0x%x, Data = %lld", p2, p2->Cnt, p2->Data);
				CCrashDump::Crash ();
			}
			if ( MemPool.Free (p2) == false )
			{
				LOG_LOG (L"LF_Queue", LOG_WARNING, L"DeQ MemPoolError Pointer = 0x%x, Data = %lld", p2, p2->Cnt, p2->Data);
				CCrashDump::Crash ();
			}
		}

		Sleep (SleepDeq);

		if ( EndEvent == true )
		{
			break;
		}
	}
	return 0;
}
*/

unsigned int WINAPI EnQThread (LPVOID pParam)
{
	test *p;
	while ( 1 )
	{
			p = MemPool.Alloc ();
			p->Cnt = 0;
			p->Data = Data1;
			if ( Queue.Push (p) == false )
			{
				LOG_LOG (L"LF_Queue", LOG_WARNING, L"EnQ Max. OverFlow");
				CCrashDump::Crash ();
			}

			if ( EndEvent == true )
			{
				break;
			}
			InterlockedIncrement (( volatile LONG * )&EnqTPS);

			Sleep (SleepEnq);
		}

	return 0;
}
unsigned int WINAPI DeQThread (LPVOID pParam)
{

	test *p2;
	while ( 1 )
	{
		if ( Queue.Pop (&p2) == false )
		{
			Sleep (SleepDeq);
			continue;
		}
		if ( p2->Cnt != 0 )
		{
			LOG_LOG (L"LF_Queue", LOG_WARNING, L"DeQ Error Pointer = 0x%x, Cnt = %lld", p2, p2->Cnt);
			CCrashDump::Crash ();
		}
		if ( p2->Data != Data1 )
		{
			LOG_LOG (L"LF_Queue", LOG_WARNING, L"DeQ Error Pointer = 0x%x, Data = %lld", p2, p2->Cnt, p2->Data);
			CCrashDump::Crash ();
		}
		if ( MemPool.Free (p2) == false )
		{
			LOG_LOG (L"LF_Queue", LOG_WARNING, L"DeQ MemPoolError Pointer = 0x%x, Data = %lld", p2, p2->Cnt, p2->Data);
			CCrashDump::Crash ();
		}
		InterlockedIncrement (( volatile LONG * )&DeqTPS);

		if ( EndEvent == true )
		{
			break;
		}
	}
	return 0;
}