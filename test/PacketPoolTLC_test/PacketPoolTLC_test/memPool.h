#pragma once
#ifndef  __MEMORYPOOL__H__
#define  __MEMORYPOOL__H__
#include <assert.h>
#include"lib\Library.h"
#include <Windows.h>
#include <new.h>


#define TLS_basicChunkSize 200


template <class DATA>
class CMemoryPool_TLS
{
private:
	/*========================================================================
	// ûũ
	========================================================================*/
	template<class DATA>
	class Chunk
	{
	public:
#define SafeLane 0xff77668888
		struct st_BLOCK_NODE
		{
			DATA BLOCK;
			INT64 Safe;
			Chunk *pChunk_Main;
		};
	public:


		st_BLOCK_NODE *_pArray;
		CMemoryPool_TLS<DATA> *_pMain_Manager;

		int FullCnt;
		int _Top;
		int FreeCnt;
	public:
		////////////////////////////////////////////////////
		//Chunk ������
		////////////////////////////////////////////////////
		Chunk ()
		{
		}
		~Chunk ()
		{
			free (_pArray);
		}

		bool ChunkSetting (int iBlockNum, CMemoryPool_TLS<DATA> *pManager)
		{
			if ( iBlockNum < 0 )
			{
				CCrashDump::Crash ();
			}
			else if ( iBlockNum == 0 )
			{
				iBlockNum = TLS_basicChunkSize;
			}

			FullCnt = iBlockNum;
			_pMain_Manager = pManager;
			_pArray = ( st_BLOCK_NODE * )malloc (sizeof (st_BLOCK_NODE) *iBlockNum);
			_Top = 0;
			FreeCnt = 0;

			for ( int Cnt = 0; Cnt < iBlockNum; Cnt++ )
			{
				_pArray[Cnt].pChunk_Main = this;
				_pArray[Cnt].Safe = SafeLane;
			}
			return true;
		}

		//////////////////////////////////////////////////////
		// �� �ϳ��� �Ҵ�޴´�.
		//
		// Parameters: PlacementNew����.
		// Return:		(DATA *) ����Ÿ �� ������.
		//////////////////////////////////////////////////////
		DATA	*Alloc (bool bPlacementNew = true)
		{
			int iBlockCount = _Top;
			st_BLOCK_NODE *stpBlock = &_pArray[iBlockCount];

			InterlockedIncrement (( volatile long * )&_Top);

			if ( bPlacementNew )
			{
				new (( DATA * )&stpBlock->BLOCK) DATA;
			}

			if ( iBlockCount + 1 == FullCnt )
			{
				//�޸�Ǯ�� �����ϴ� ûũ ��� ����� ���ο� ������� ����.
				_pMain_Manager->Chunk_Alloc ();
			}

			return &stpBlock->BLOCK;

		}

		bool Free (DATA *pData)
		{
			st_BLOCK_NODE *stpBlock;


			stpBlock = (( st_BLOCK_NODE * )pData);

			if ( stpBlock->Safe != SafeLane )
			{
				return false;
			}

			InterlockedIncrement (( volatile long * )&FreeCnt);

			if ( FreeCnt == FullCnt )
			{
				free (this);
			}

			return true;

		}
	};

	struct st_Chunk_NODE
	{
		Chunk<DATA> *pChunk;
		DWORD ThreadID;
		st_Chunk_NODE *pNextNode;
	};

	st_Chunk_NODE *_pTopNode;
	int Chunk_in_BlockCnt;
	DWORD TlsNum;

public:
	/*========================================================================
	// ������
	========================================================================*/
	CMemoryPool_TLS (int iBlockNum)
	{
		if ( iBlockNum == 0 )
		{
			iBlockNum = TLS_basicChunkSize;
		}
		_pTopNode = NULL;
		Chunk_in_BlockCnt = iBlockNum;
		TlsNum = TlsAlloc ();

		//TLS�� ������ �Ұ��� �����̹Ƿ� �ڱ��ڽ��� �ı��ϰ� ����.
		if ( TlsNum == TLS_OUT_OF_INDEXES )
		{
			CCrashDump::Crash ();
			return;//Dump
		}
	}
	~CMemoryPool_TLS ()
	{
		st_Chunk_NODE *pPreTopNode = _pTopNode;
		st_Chunk_NODE *pdeleteTopNode = _pTopNode;
		while ( 1 )
		{
			pPreTopNode = pPreTopNode->pNextNode;

			free(pdeleteTopNode->pChunk);
			
			free (pdeleteTopNode);

			pdeleteTopNode = pPreTopNode;
			if ( pdeleteTopNode == NULL )
			{
				break;
			}
		}
		return;
	}

	/*========================================================================
	// �� �ϳ��� �Ҵ� �޴´�.
	//
	// Parameters:	PlacementNew ����.
	// Return:		(DATA *) �� ������.
	========================================================================*/
	DATA *Alloc (bool bPlacemenenew = true)
	{
		//�ش� �����忡�� ���� ����ɶ�. �ʱ�ȭ �۾�.
		st_Chunk_NODE *pChunkNode = ( st_Chunk_NODE  *) TlsGetValue (TlsNum);
		if ( pChunkNode == NULL )
		{


			pChunkNode = ( st_Chunk_NODE  * )malloc (sizeof (st_Chunk_NODE));
			pChunkNode->pChunk = (Chunk<DATA> *) malloc (sizeof (Chunk<DATA>));
			pChunkNode->ThreadID = GetCurrentThreadId ();
			pChunkNode->pChunk->ChunkSetting (Chunk_in_BlockCnt, this);

			TlsSetValue (TlsNum, pChunkNode);

			pChunkNode->pNextNode = _pTopNode;
			_pTopNode = pChunkNode;

			InterlockedAdd (( volatile long * )&m_iBlockCount, Chunk_in_BlockCnt);

		}

	//	InterlockedIncrement (( volatile long * )&m_iAllocCount);
		return pChunkNode->pChunk->Alloc ();

	}

	/*========================================================================
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters:	(DATA *) �� ������.
	// Return:		(BOOL) TRUE, FALSE.
	========================================================================*/
	bool Free (DATA *pDATA)
	{
		Chunk<DATA>::st_BLOCK_NODE *pNode =(Chunk<DATA>::st_BLOCK_NODE *) pDATA;

		bool chk = pNode->pChunk_Main->Free (pDATA);
	//	InterlockedDecrement (( volatile long * )&m_iAllocCount);
	//	InterlockedIncrement (( volatile long * )&m_iFreeCount);
		return chk;
	}
	public :


		/*========================================================================
		// Alloc�� �ٵ� Chunk���� ��ü�Ѵ�.
		//
		// Parameters:	����
		// Return:		����
		========================================================================*/
		void Chunk_Alloc ()
		{
			st_Chunk_NODE *pPreTopNode = _pTopNode;
			DWORD ThreadID = GetCurrentThreadId ();

			while ( 1 )
			{
				if ( pPreTopNode->ThreadID == ThreadID )
				{
					pPreTopNode->pChunk = (Chunk<DATA> *) malloc (sizeof (Chunk<DATA>));
					pPreTopNode->pChunk->ChunkSetting (Chunk_in_BlockCnt, this);
					break;
				}
				//ûũ ����� ��ü�ϴµ� �ش� �����尡 �������� �ʴ� �������� ������ �ǹǷ� ũ������ �������� ���� �����.
				else if(pPreTopNode == NULL )
				{
					CCrashDump::Crash ();
					return;
				}
				pPreTopNode = pPreTopNode->pNextNode;
			}
			return;
		}


	/*========================================================================
	// ���� ������� �� ������ ��´�.
	//
	// ! ����
	//	TLC�� ���ɻ� �Ѱ�� ���� ������ ����.
	//
	// Parameters:	����.
	// Return:		(int) ������� �� ����.
	========================================================================*/
	int		GetAllocCount (void)
	{
		//return m_iAllocCount;
		return 0;
	}

	/*========================================================================
	// �޸�Ǯ �� ��ü ������ ��´�.
	//
	// Parameters:	����.
	// Return:		(int) ��ü �� ����.
	========================================================================*/
	int		GetFullCount (void)
	{
		return m_iBlockCount;
	}

	/*========================================================================
	// ���� �������� �� ������ ��´�.
	//
	// ! ����
	//	TLC�� ���ɻ� �Ѱ�� ���� ������ ����.
	//
	// Parameters:	����.
	// Return:		(int) �������� �� ����.
	========================================================================*/
	int		GetFreeCount (void)
	{
		//return m_iFreeCount;
		return 0;
	}

private :

	int m_iBlockCount;
	int m_iAllocCount;
	int m_iFreeCount;
};


#endif