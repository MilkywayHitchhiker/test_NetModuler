/*---------------------------------------------------------------

	MemoryPool.

	�޸� Ǯ Ŭ����.
	Ư�� ����Ÿ�� ������ �Ҵ� �� ��������.

	- ����.

	CMemoryPool<DATA> MemPool(300);
	DATA *pData = MemPool.Alloc();

	pData ���

	MemPool.Free(pData);

----------------------------------------------------------------*/
#ifndef  __MEMORYPOOL__H__
#define  __MEMORYPOOL__H__
#include <assert.h>
#include "lib\Library.h"
#include <Windows.h>
#include <new.h>

#define TLS_basicChunkSize 1000



template <class DATA>
class CMemoryPool
{
#define SafeLane 0xff77668888
private:

	/*========================================================================
	// �� �� �տ� ���� ��� ����ü.
	========================================================================*/
	struct st_BLOCK_NODE
	{
		DATA Data;
		INT64 Safe;

		st_BLOCK_NODE ()
		{
			stpNextBlock = NULL;
		}

		st_BLOCK_NODE *stpNextBlock;
	};


	void LOCK ()
	{
		AcquireSRWLockExclusive (&_CS);
	}
	void Release ()
	{
		ReleaseSRWLockExclusive (&_CS);
	}

public:

	/*========================================================================
	// ������, �ı���.
	//
	// Parameters:	(int) �ִ� �� ����.
	// Return:		����.
	========================================================================*/
	CMemoryPool (int iBlockNum)
	{
		st_BLOCK_NODE *pNode, *pPreNode;
		InitializeSRWLock (&_CS);
		/*========================================================================
		// TOP ��� �Ҵ�
		========================================================================*/
		_pTop = NULL;

		/*========================================================================
		// �޸� Ǯ ũ�� ����
		========================================================================*/
		m_iBlockCount = iBlockNum;
		if ( iBlockNum < 0 )
		{
			CCrashDump::Crash ();
			return;	// Dump
		}
		else if ( iBlockNum == 0 )
		{
			m_bStoreFlag = true;
			_pTop = NULL;
		}

		/*========================================================================
		// DATA * ũ�⸸ ŭ �޸� �Ҵ� �� BLOCK ����
		========================================================================*/
		else
		{
			m_bStoreFlag = false;

			pNode = ( st_BLOCK_NODE * )malloc (sizeof (st_BLOCK_NODE));
			_pTop = pNode;
			pPreNode = pNode;

			for ( int iCnt = 1; iCnt < iBlockNum; iCnt++ )
			{
				pNode = ( st_BLOCK_NODE * )malloc (sizeof (st_BLOCK_NODE));
				pPreNode->stpNextBlock = pNode;
				pPreNode = pNode;
			}
		}
	}

	virtual	~CMemoryPool ()
	{
		st_BLOCK_NODE *pNode;

		for ( int iCnt = 0; iCnt < m_iBlockCount; iCnt++ )
		{
			pNode = _pTop;
			_pTop = _pTop->stpNextBlock;
			free (pNode);
		}
	}

	/*========================================================================
	// �� �ϳ��� �Ҵ�޴´�.
	//
	// Parameters: PlacementNew����.
	// Return:		(DATA *) ����Ÿ �� ������.
	========================================================================*/
	DATA	*Alloc (bool bPlacementNew = true)
	{
		st_BLOCK_NODE *stpBlock;
		int iBlockCount = m_iBlockCount;


		InterlockedIncrement64 (( LONG64 * )&m_iAllocCount);

		if ( iBlockCount < m_iAllocCount )
		{
			if ( m_bStoreFlag )
			{
				stpBlock = ( st_BLOCK_NODE * )malloc (sizeof (st_BLOCK_NODE));
				InterlockedIncrement64 (( LONG64 * )&m_iBlockCount);
			}

			else
			{
				return nullptr;
			}
		}

		else
		{
			LOCK ();

			stpBlock = _pTop;
			_pTop = _pTop->stpNextBlock;

			Release ();
		}

		if ( bPlacementNew )
		{
			new (( DATA * )&stpBlock->Data) DATA;
		}

		stpBlock->Safe = SafeLane;


		return &stpBlock->Data;
	}

	/*========================================================================
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters:	(DATA *) �� ������.
	// Return:		(BOOL) TRUE, FALSE.
	========================================================================*/
	bool	Free (DATA *pData)
	{
		st_BLOCK_NODE *stpBlock;


		stpBlock = (( st_BLOCK_NODE * )pData);

		if ( stpBlock->Safe != SafeLane )
		{
			return false;
		}

		LOCK ();

		stpBlock->stpNextBlock = _pTop;
		_pTop = stpBlock;

		Release ();
		
		InterlockedDecrement64 (( LONG64 * )&m_iAllocCount);
		
		return true;
	}


	/*========================================================================
	// ���� ������� �� ������ ��´�.
	//
	// Parameters:	����.
	// Return:		(int) ������� �� ����.
	========================================================================*/
	int		GetAllocCount (void)
	{
		return m_iAllocCount;
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
	// Parameters:	����.
	// Return:		(int) �������� �� ����.
	========================================================================*/
	int		GetFreeCount (void)
	{
		return m_iBlockCount - m_iAllocCount;
	}

private:
	/*========================================================================
	// ��� ������ ž
	========================================================================*/
	st_BLOCK_NODE *_pTop;

	/*========================================================================
	// �޸� ���� �÷���, true�� ������ �����Ҵ� ��
	========================================================================*/
	bool m_bStoreFlag;

	/*========================================================================
	// ���� ������� �� ����
	========================================================================*/
	int m_iAllocCount;

	/*========================================================================
	// ��ü �� ����
	========================================================================*/
	int m_iBlockCount;

	SRWLOCK _CS;

};

template <class DATA>
class CMemoryPool_LF
{
private:

	/*========================================================================
	// �� �� �տ� ���� ��� ����ü.
	========================================================================*/
	struct st_BLOCK_NODE
	{
		DATA Data;
		st_BLOCK_NODE ()
		{
			stpNextBlock = NULL;
		}
		st_BLOCK_NODE *stpNextBlock;
	};

	/*========================================================================
	// ������ �޸� Ǯ�� ž ���
	========================================================================*/
	struct st_TOP_NODE
	{
		st_BLOCK_NODE *pTopNode;
		__int64 iUniqueNum;
	};

public:

	/*========================================================================
	// ������, �ı���.
	//
	// Parameters:	(int) �ִ� �� ����.
	// Return:		����.
	========================================================================*/
	CMemoryPool_LF (int iBlockNum)
	{
		st_BLOCK_NODE *pNode, *pPreNode;

		/*========================================================================
		// TOP ��� �Ҵ�
		========================================================================*/
		_pTop = ( st_TOP_NODE * )_aligned_malloc (sizeof (st_TOP_NODE), 16);
		_pTop->pTopNode = NULL;
		_pTop->iUniqueNum = 0;

		_iUniqueNum = 0;

		/*========================================================================
		// �޸� Ǯ ũ�� ����
		========================================================================*/
		m_iBlockCount = iBlockNum;
		if ( iBlockNum < 0 )
		{
			CCrashDump::Crash ();
			return;	// Dump
		}
		else if ( iBlockNum == 0 )
		{
			m_bStoreFlag = true;
			_pTop->pTopNode = NULL;
		}

		/*========================================================================
		// DATA * ũ�⸸ ŭ �޸� �Ҵ� �� BLOCK ����
		========================================================================*/
		else
		{
			m_bStoreFlag = false;

			pNode = ( st_BLOCK_NODE * )malloc (sizeof (st_BLOCK_NODE));
			_pTop->pTopNode = pNode;
			pPreNode = pNode;

			for ( int iCnt = 1; iCnt < iBlockNum; iCnt++ )
			{
				pNode = ( st_BLOCK_NODE * )malloc (sizeof (st_BLOCK_NODE));
				pPreNode->stpNextBlock = pNode;
				pPreNode = pNode;
			}
		}
	}

	virtual	~CMemoryPool_LF ()
	{
		st_BLOCK_NODE *pNode;

		for ( int iCnt = 0; iCnt < m_iBlockCount; iCnt++ )
		{
			pNode = _pTop->pTopNode;
			_pTop->pTopNode = _pTop->pTopNode->stpNextBlock;
			free (pNode);
		}
	}

	/*========================================================================
	// �� �ϳ��� �Ҵ�޴´�.
	//
	// Parameters: PlacementNew����.
	// Return:		(DATA *) ����Ÿ �� ������.
	========================================================================*/
	DATA	*Alloc (bool bPlacementNew = true)
	{
		st_BLOCK_NODE *stpBlock;
		st_TOP_NODE pPreTopNode;
		int iBlockCount = m_iBlockCount;
		InterlockedIncrement64 (( LONG64 * )&m_iAllocCount);

		if ( iBlockCount < m_iAllocCount )
		{
			if ( m_bStoreFlag )
			{
				stpBlock = ( st_BLOCK_NODE * )malloc (sizeof (st_BLOCK_NODE));
				InterlockedIncrement64 (( LONG64 * )&m_iBlockCount);
			}

			else
				return nullptr;
		}

		else
		{
			__int64 iUniqueNum = InterlockedIncrement64 (&_iUniqueNum);

			do
			{
				pPreTopNode.iUniqueNum = _pTop->iUniqueNum;
				pPreTopNode.pTopNode = _pTop->pTopNode;

			} while ( !InterlockedCompareExchange128 (( volatile LONG64 * )_pTop,
				iUniqueNum,
				( LONG64 )_pTop->pTopNode->stpNextBlock,
				( LONG64 * )&pPreTopNode) );

			stpBlock = pPreTopNode.pTopNode;
		}

		if ( bPlacementNew )
		{
			new (( DATA * )&stpBlock->Data) DATA;
		}

		return &stpBlock->Data;
	}

	/*========================================================================
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters:	(DATA *) �� ������.
	// Return:		(BOOL) TRUE, FALSE.
	========================================================================*/
	bool	Free (DATA *pData)
	{
		st_BLOCK_NODE *stpBlock;
		st_TOP_NODE pPreTopNode;

		__int64 iUniqueNum = InterlockedIncrement64 (&_iUniqueNum);

		do
		{
			pPreTopNode.iUniqueNum = _pTop->iUniqueNum;
			pPreTopNode.pTopNode = _pTop->pTopNode;

			stpBlock = (( st_BLOCK_NODE * )pData);
			stpBlock->stpNextBlock = _pTop->pTopNode;
		} while ( !InterlockedCompareExchange128 (( volatile LONG64 * )_pTop, iUniqueNum, ( LONG64 )stpBlock, ( LONG64 * )&pPreTopNode) );

		InterlockedDecrement64 (( LONG64 * )&m_iAllocCount);
		return true;
	}


	/*========================================================================
	// ���� ������� �� ������ ��´�.
	//
	// Parameters:	����.
	// Return:		(int) ������� �� ����.
	========================================================================*/
	int		GetAllocCount (void)
	{
		return m_iAllocCount;
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
	// Parameters:	����.
	// Return:		(int) �������� �� ����.
	========================================================================*/
	int		GetFreeCount (void)
	{
		return m_iBlockCount - m_iAllocCount;
	}

private:
	/*========================================================================
	// ��� ������ ž
	========================================================================*/
	st_TOP_NODE *_pTop;

	/*========================================================================
	// ž�� Unique Number
	========================================================================*/
	__int64 _iUniqueNum;

	/*========================================================================
	// �޸� ���� �÷���, true�� ������ �����Ҵ� ��
	========================================================================*/
	bool m_bStoreFlag;

	/*========================================================================
	// ���� ������� �� ����
	========================================================================*/
	int m_iAllocCount;

	/*========================================================================
	// ��ü �� ����
	========================================================================*/
	int m_iBlockCount;


};

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
	private :


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

			_Top = 0;
			FreeCnt = 0;
			FullCnt = iBlockNum;
			_pMain_Manager = pManager;
			_pArray = ( st_BLOCK_NODE * )malloc (sizeof (st_BLOCK_NODE) * iBlockNum);


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
			int iBlockCount = InterlockedIncrement (( volatile long * )&_Top);
			st_BLOCK_NODE *stpBlock = &_pArray[iBlockCount - 1];

			if ( bPlacementNew )
			{
				new (( DATA * )&stpBlock->BLOCK) DATA;
			}

			if ( iBlockCount == FullCnt )
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

			int Cnt = InterlockedIncrement (( volatile long * )&FreeCnt);

			if ( Cnt == FullCnt )
			{
				free (_pArray);
				free(this);
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
	SRWLOCK _CS;
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

			free (pdeleteTopNode->pChunk);

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
		st_Chunk_NODE *pChunkNode = ( st_Chunk_NODE  * )TlsGetValue (TlsNum);



		if ( pChunkNode == NULL )
		{


			pChunkNode = ( st_Chunk_NODE  * )malloc (sizeof (st_Chunk_NODE));
			pChunkNode->pChunk = (Chunk<DATA> *)malloc (sizeof (Chunk<DATA>));

			pChunkNode->ThreadID = GetCurrentThreadId ();
			pChunkNode->pChunk->ChunkSetting (Chunk_in_BlockCnt, this);

			TlsSetValue (TlsNum, pChunkNode);

			pChunkNode->pNextNode = _pTopNode;
			_pTopNode = pChunkNode;

			InterlockedAdd (( volatile long * )&m_iBlockCount, Chunk_in_BlockCnt);

		}


		DATA *pData = pChunkNode->pChunk->Alloc ();


		//InterlockedIncrement (( volatile long * )&m_iAllocCount);

		return pData;

	}
	
	/*========================================================================
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters:	(DATA *) �� ������.
	// Return:		(BOOL) TRUE, FALSE.
	========================================================================*/
	bool Free (DATA *pDATA)
	{
		Chunk<DATA>::st_BLOCK_NODE *pNode = (Chunk<DATA>::st_BLOCK_NODE *) pDATA;

		bool chk = pNode->pChunk_Main->Free (pDATA);
	//		InterlockedDecrement (( volatile long * )&m_iAllocCount);
	//		InterlockedIncrement (( volatile long * )&m_iFreeCount);
		return chk;
	}
public:


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
				pPreTopNode->pChunk = (Chunk<DATA> *)malloc (sizeof (Chunk<DATA>));
				pPreTopNode->pChunk->ChunkSetting (Chunk_in_BlockCnt, this);
				break;
			}
			//ûũ ����� ��ü�ϴµ� �ش� �����尡 �������� �ʴ� �������� ������ �ǹǷ� ũ������ �������� ���� �����.
			else if ( pPreTopNode == NULL )
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
	//	return m_iAllocCount;
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
	//	return m_iFreeCount;
		return 0;
	}

private:

	int m_iBlockCount;
	int m_iAllocCount;
	int m_iFreeCount;
};


#endif