// LockFree_test.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "lib\Library.h"
#include "LockFreeQueue.h"
#include "MemoryPool.h"


#define NodeMAX 1000
#define Thread_Max 5
#define Data1 0x5555777788889999

#define SleepEnq 0
#define SleepDeq 0

struct test
{
	INT64 Data;
	INT64 Cnt;
};
Queue_LOCK<test *> Queue;
CMemoryPool_LF<test> MemPool(0);

unsigned int WINAPI WorkerThread (LPVOID pParam);


HANDLE Thread[Thread_Max];
bool EndEvent;

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

	for ( int Cnt = 0; Cnt < Thread_Max; Cnt++ )
	{
		Thread[Cnt] = ( HANDLE )_beginthreadex (NULL, 0, WorkerThread, NULL, NULL, NULL);
	}

	while ( 1 )
	{

		wprintf (L"Queue Use Node Cnt = %lld\n", Num);
		wprintf (L"MemPool Alloc Node Cnt = %d\n", Alloc);
		wprintf (L"MemPool Free Node Cnt = %d\n", Free);
		wprintf (L"MemPool Full Node Cnt = %d\n\n", Full);
		Num = Queue.GetUseSize ();
		Alloc = MemPool.GetAllocCount ();
		Free = MemPool.GetFreeCount ();
		Full = MemPool.GetFullCount ();

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
			if ( Queue.Enqueue (p[Cnt]) == false )
			{
				LOG_LOG (L"LF_Queue", LOG_WARNING, L"EnQ Max. OverFlow");
				CCrashDump::Crash ();
			}
		}



		Sleep(SleepEnq);



		test *p2;
		for ( Cnt = 0; Cnt < NodeMAX; Cnt++ )
		{
			if ( Queue.Dequeue (&p2) == false )
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