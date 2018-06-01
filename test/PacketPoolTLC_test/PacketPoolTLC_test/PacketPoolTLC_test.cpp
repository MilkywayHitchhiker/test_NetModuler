// PacketPoolTLC_test.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "RingBuffer.h"
#include "PacketPool.h"
#define ThreadMAX 6
#define InqueueTime 1
#define QueueThread 3

unsigned int WINAPI InQueueThread (LPVOID pParam);

unsigned int WINAPI WorkerThread (LPVOID pParam);

HANDLE Thread[ThreadMAX];
HANDLE Event;

CCrashDump Dump;

struct test
{
	unsigned int a;
};


CRingbuffer Queue;

int main()
{
	Packet::Initialize ();
	int Cnt=0;

	Event = CreateEvent (NULL, FALSE, FALSE, NULL);


	for ( int Cnt = 0; Cnt < ThreadMAX- QueueThread; Cnt++ )
	{
		Thread[Cnt] = ( HANDLE )_beginthreadex (NULL, 0, WorkerThread,NULL, NULL, NULL);
	}
	for ( int Cnt = ThreadMAX - QueueThread; Cnt < ThreadMAX; Cnt++ )
	{
		Thread[Cnt] = ( HANDLE )_beginthreadex (NULL, 0, InQueueThread, NULL, NULL, NULL);
	}


	while (1)
	{


		wprintf (L"Full Cnt = %d \n", Packet::PacketPool->GetFullCount());
		wprintf (L"Alloc Cnt = %d \n", Packet::PacketPool->GetAllocCount());
		wprintf (L"Free Cnt = %d \n\n", Packet::PacketPool->GetFreeCount ());

		Sleep (1000);

	}


    return 0;
}



unsigned int WINAPI InQueueThread (LPVOID pParam)
{
	while ( 1 )
	{
		
		Packet *p = Packet::Alloc ();
		if ( p == NULL )
		{
			wprintf (L"===============Alloc ERROR==============\n");
			CCrashDump::Crash;
		}

		*p << 7799;

		Queue.Lock ();
		Queue.Put((char *)&p,sizeof(p));
		Queue.Free ();
		SetEvent (Event);

		Sleep (InqueueTime);
	}

}


unsigned int WINAPI WorkerThread (LPVOID pParam)
{
	Packet *p;
	int a;
	while ( 1 )
	{
		WaitForSingleObject (Event, INFINITE);

		Queue.Lock ();
		Queue.Get (( char * )&p, 8);
		Queue.Free ();
		*p >> a;

		if ( a != 7799 )
		{
			wprintf (L"===============FREE ERROR==============\n");
			CCrashDump::Crash;
		}
		Packet::Free (p);
	}
}