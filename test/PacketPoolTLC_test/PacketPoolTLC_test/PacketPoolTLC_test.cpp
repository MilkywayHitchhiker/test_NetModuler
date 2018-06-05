// PacketPoolTLC_test.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "RingBuffer.h"
#include "MemoryPool.h"
#include"lib\Library.h"
#define testMax 100000


struct test
{
	unsigned int a;
};


CMemoryPool<test> LockPool (0);
CMemoryPool_LF<test> LFPool (0);
CMemoryPool_TLS<test> TLSPool (0);

int main()
{
	int Cnt=0;
	test *p[testMax];

	for ( int Num = 0; Num < 10; Num++ )
	{
		for ( Cnt = 0; Cnt < testMax; Cnt++ )
		{
			p[Cnt] = NULL;
		}


		//new delete
		PROFILE_BEGIN (L"new");
		for ( Cnt = 0; Cnt < testMax; Cnt++ )
		{

			p[Cnt] = new test;

		}
		PROFILE_END (L"new");

		PROFILE_BEGIN (L"delete");
		for ( Cnt = 0; Cnt < testMax; Cnt++ )
		{

			delete p[Cnt];

		}
		PROFILE_END (L"delete");

		//malloc free
		PROFILE_BEGIN (L"malloc");
		for ( Cnt = 0; Cnt < testMax; Cnt++ )
		{

			p[Cnt] = ( test * )malloc (sizeof (test));

		}
		PROFILE_END (L"malloc");

		PROFILE_BEGIN (L"free");
		for ( Cnt = 0; Cnt < testMax; Cnt++ )
		{
			free (p[Cnt]);

		}
		PROFILE_END (L"free");

		//LockPool Alloc Free
		PROFILE_BEGIN (L"LOCKPool Alloc ");
		for ( Cnt = 0; Cnt < testMax; Cnt++ )
		{

			p[Cnt] = LockPool.Alloc ();

		}
		PROFILE_END (L"LOCKPool Alloc ");

		PROFILE_BEGIN (L"LOCKPool Free ");
		for ( Cnt = 0; Cnt < testMax; Cnt++ )
		{

			LockPool.Free (p[Cnt]);

		}
		PROFILE_END (L"LOCKPool Free ");

		//LFPool Alloc Free
		for ( Cnt = 0; Cnt < testMax; Cnt++ )
		{
			PROFILE_BEGIN (L"LFPool Alloc ");
			p[Cnt] = LFPool.Alloc ();
			PROFILE_END (L"LFPool Alloc ");
		}

		for ( Cnt = 0; Cnt < testMax; Cnt++ )
		{
			PROFILE_BEGIN (L"LFPool Free ");
			LFPool.Free (p[Cnt]);
			PROFILE_END (L"LFPool Free ");
		}

		//TLSPool Alloc Free
		for ( Cnt = 0; Cnt < testMax; Cnt++ )
		{
			PROFILE_BEGIN (L"TLSPool Alloc ");
			p[Cnt] = TLSPool.Alloc ();
			PROFILE_END (L"TLSPool Alloc ");
		}

		for ( Cnt = 0; Cnt < testMax; Cnt++ )
		{
			PROFILE_BEGIN (L"TLSPool Free ");
			TLSPool.Free (p[Cnt]);
		PROFILE_END (L"TLSPool Free ");
		}


	}
	PROFILE_KEYPROC ();

    return 0;
}