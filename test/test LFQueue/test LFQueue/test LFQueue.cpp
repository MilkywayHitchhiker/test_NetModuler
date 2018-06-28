// test LFQueue.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <Windows.h>
#pragma comment(lib, "winmm.lib")
#include"lib\Library.h"
#include "LockFreeQueue.h"
#include "MemoryPool.h"
#include "PacketPool.h"
CCrashDump Dump;

static unsigned int WINAPI EnQThread (LPVOID pParam);

static unsigned int WINAPI DeQThread (LPVOID pParam);

HANDLE Thread[50];


CQueue_LF<Packet *> UpdateQueue;
//CMemoryPool<int> MemPool(0);
bool flag;

LONG64 EnqSpeed;
LONG64 DeqSpeed;
int EnQ_Max = 40;
int DeQ_Max = 10;
int main()
{
	LOG_DIRECTORY (L"Log");
	LOG_LEVEL (LOG_DEBUG,false);
	timeBeginPeriod (1);
	Packet::Initialize ();

	flag = false;

	for ( int Cnt = 0; Cnt < EnQ_Max; Cnt++ )
	{

		Thread[Cnt] = ( HANDLE )_beginthreadex (NULL, 0, EnQThread, NULL, NULL, NULL);
	}
	for ( int Cnt = EnQ_Max; Cnt < EnQ_Max + DeQ_Max; Cnt++ )
	{

		Thread[Cnt] = ( HANDLE )_beginthreadex (NULL, 0, DeQThread, NULL, NULL, NULL);
	}

	DWORD StartTime = GetTickCount ();
	DWORD EndTime;
	INT64 QueueUse = 0;
	int QMemPoolSize = 0;
	int QMemPoolUse = 0;

	int MemPoolSize = 0;
	int MemPoolUse = 0;
	LOG_LOG (L"StartTime", LOG_SYSTEM, L"Test Start");
	while ( 1 )
	{
		EndTime = GetTickCount ();
		if ( EndTime - StartTime >= 1000 )
		{
			wprintf (L"==========================\n");
			wprintf (L"Queue Use Size = %lld \n",QueueUse);
			wprintf (L"Queue MemPool Full Size = %d \n", QMemPoolSize);
			wprintf (L"Queue MemPool Use Size = %d \n", QMemPoolUse);

			wprintf (L"EnqTPS = %lld \n", EnqSpeed );
			wprintf (L"DeqTPS = %lld \n", DeqSpeed );

			wprintf (L"PacketPool Full Size = %d \n", MemPoolSize);
			wprintf (L"PacketPool Use Size = %d \n", MemPoolUse);
			wprintf (L"==========================\n");


			QueueUse = UpdateQueue.GetUseSize ();
			QMemPoolSize = UpdateQueue._pMemPool->GetFullCount ();
			QMemPoolUse = UpdateQueue._pMemPool->GetAllocCount ();

			/*
			MemPoolSize =MemPool.GetFullCount ();
			MemPoolUse = MemPool.GetAllocCount ();
			*/
			MemPoolSize = Packet::PacketPool_Full ();
			MemPoolUse = Packet::PacketPool_Alloc ();

			InterlockedExchange64 (&EnqSpeed, 0);
			InterlockedExchange64 (&DeqSpeed, 0);

			StartTime = EndTime;

		}

		if ( GetAsyncKeyState ('E') & 0x8001 )
		{
			flag = true;
			Sleep (100);
			break;
		}
	}
	timeEndPeriod (1);
    return 0;
}

static unsigned int WINAPI EnQThread (LPVOID pParam)
{
	while ( 1 )
	{
		
		Packet *p = Packet::Alloc ();
		int a = 10;
		*p << a;
		
		//int *p = MemPool.Alloc ();
		if ( UpdateQueue.Enqueue (p) == false )
		{
			LOG_LOG (L"EnQ", LOG_ERROR, L"QUEUE OverFlow");
			break;
		}
		Sleep (1);

		InterlockedIncrement64 (&EnqSpeed);

		if ( flag )
		{
			break;
		}
	}
	return 0;
}

static unsigned int WINAPI DeQThread (LPVOID pParam)
{
	while ( 1 )
	{
		Packet *p = NULL;
//		int *p;
		if ( UpdateQueue.Dequeue (&p) == false )
		{

			Sleep (5);
			continue;
		}

		int a;
		*p >> a;

		if ( a != 10 )
		{
			LOG_LOG (L"DeQ", LOG_ERROR, L"QUEUE Error");
		}


		/*
		if ( *p != 10 )
		{
			LOG_LOG (L"DeQ", LOG_ERROR, L"QUEUE Error");
		}
		MemPool.Free (p);
		*/

		InterlockedIncrement64 (&DeqSpeed);

		if ( flag )
		{
			break;
		}
		Packet::Free (p);
	}
	return 0;
}