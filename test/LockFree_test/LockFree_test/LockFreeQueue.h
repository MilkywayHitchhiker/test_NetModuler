#pragma once
#include "MemoryPool.h"
#define MaxCnt 10000


/*
struct DATA
{
	int a;
};
*/


/*======================================================================================
//�̱� ��ũ�� ����ũ ���� ������ Queue.
=======================================================================================*/
template<class DATA>
class Queue_LF
{
private :
	struct NODE
	{
		DATA Data;
		NODE *pNext;
	};
	struct _TOP_NODE
	{
		NODE *pNode;
		INT64 UNIQUEUE;
	};


	_TOP_NODE *_pHead;
	_TOP_NODE *_pTail;

	CMemoryPool_LF<NODE> *_pMemPool;

	volatile __int64 _UniqueueNum;
	volatile __int64 _NodeCnt;
	volatile __int64 _MaxCnt;
public :

	/*//////////////////////////////////////////////////////////////////////
	//������.�ı���.
	//////////////////////////////////////////////////////////////////////*/
	Queue_LF ()
	{
		_TOP_NODE *HNode = ( _TOP_NODE * )_aligned_malloc (sizeof (_TOP_NODE), 16);
		_TOP_NODE *TNode = ( _TOP_NODE * )_aligned_malloc (sizeof (_TOP_NODE), 16);
		HNode->pNode = ( NODE * )malloc (sizeof (NODE));
		HNode->pNode->pNext = NULL;
		HNode->UNIQUEUE = 0;
		
		TNode->pNode = HNode->pNode;
		TNode->UNIQUEUE = 0;
		_MaxCnt = MaxCnt;
		_NodeCnt = 0;

		
		_pHead = HNode;
		_pTail = TNode;

		_pMemPool = new CMemoryPool_LF<NODE> (0);
	}
	~Queue_LF ()
	{
		DATA p;
		while ( 1 )
		{
			
			if ( Dequeue(&p) == false )
			{
				return;
			}
		}
	}


	/*//////////////////////////////////////////////////////////////////////
	//Enqueue
	//���� : DATA
	//return : true,false
	//////////////////////////////////////////////////////////////////////*/

	bool Enqueue (DATA Data)
	{

		//Queue�� ��á���Ƿ� return false.
		if ( _NodeCnt >= _MaxCnt )
		{
			return false;
		}

		_TOP_NODE PreNode;


		NODE *pNode = _pMemPool->Alloc ();
		pNode->Data = Data;
		pNode->pNext = NULL;

		INT64 Uniqueue = InterlockedIncrement64 (&_UniqueueNum);

		

		while ( 1 )
		{
			PreNode.pNode = _pTail->pNode;
			PreNode.UNIQUEUE = _pTail->UNIQUEUE;

			//Tail�� next�� NULL�� �ƴ� ��� �̹� �������� ���� ��带 �־����Ƿ� Tail�� �о���.
			if ( PreNode.pNode->pNext != NULL )
			{
				InterlockedCompareExchangePointer (( volatile PVOID * )_pTail->pNode, PreNode.pNode->pNext, PreNode.pNode);
				continue;
			}

			//Tail�� Next�� NULL�� ��� ���� ��� ����
			if ( InterlockedCompareExchangePointer (( volatile PVOID * )&_pTail->pNode->pNext, pNode, NULL) == NULL )
			{
				InterlockedCompareExchange128 (( volatile LONG64 * )_pTail, Uniqueue, ( LONG64 )pNode, ( LONG64 * )&PreNode);
				break;
			}
		}
		InterlockedIncrement64 (&_NodeCnt);
		return true;
	}


	/*//////////////////////////////////////////////////////////////////////
	//Dequeue
	//���� : ���� �����͸� ���� ����.
	//return : true,false
	//////////////////////////////////////////////////////////////////////*/
	bool Dequeue (DATA *pOut)
	{
		_TOP_NODE PreNode;
		NODE *pNode;
		DATA pOut;

		INT64 Uniqueue = InterlockedIncrement64 (&_UniqueueNum);

		while ( 1 )
		{
			PreNode.pNode = _pHead->pNode;
			PreNode.UNIQUEUE = _pHead->UNIQUEUE;
			
			pNode = PreNode.pNode->pNext;

			//����� Next��带 �̴� ���̹Ƿ� pNode�� NULL�̶�� Dequeue �Ұ���.
			if ( pNode == NULL )
			{
				pOut = NULL;
				return pOut;
			}

			if ( InterlockedCompareExchange128 (( volatile LONG64 * )_pHead, Uniqueue, ( LONG64 )pNode, ( LONG64 * )&PreNode) )
			{
				*pOut = pNode->Data;
				_pMemPool->Free (PreNode.pNode);
				InterlockedDecrement64 (&_NodeCnt);
				return pOut;
			}
		}
	}



	/*//////////////////////////////////////////////////////////////////////
	//GetUseSize
	//���� : ����
	//return : ���� Node����.
	//////////////////////////////////////////////////////////////////////*/
	INT64 GetUseSize (void)
	{
		return _NodeCnt;
	}
};






/*======================================================================================
//�̱� ��ũ�� ����ũ ���� Thread Safe SRW Lock���� Queue
=======================================================================================*/
template<class DATA>
class Queue_LOCK
{
private:
	struct NODE
	{
		DATA Data;
		NODE *pNext;
	};
	struct _TOP_NODE
	{
		NODE *pNode;
	};


	_TOP_NODE *_pHead;
	_TOP_NODE *_pTail;

	CMemoryPool_LF<NODE> *_pMemPool;

	volatile __int64 _NodeCnt;
	volatile __int64 _MaxCnt;

	SRWLOCK _CS;
public:

	/*//////////////////////////////////////////////////////////////////////
	//������.�ı���.
	//////////////////////////////////////////////////////////////////////*/
	Queue_LOCK ()
	{
		InitializeSRWLock (&_CS);

		_TOP_NODE *HNode = ( _TOP_NODE * )_aligned_malloc (sizeof (_TOP_NODE), 16);
		_TOP_NODE *TNode = ( _TOP_NODE * )_aligned_malloc (sizeof (_TOP_NODE), 16);
		HNode->pNode = ( NODE * )malloc (sizeof (NODE));
		HNode->pNode->pNext = NULL;

		TNode->pNode = HNode->pNode;
		_MaxCnt = MaxCnt;
		_NodeCnt = 0;


		_pHead = HNode;
		_pTail = TNode;

		_pMemPool = new CMemoryPool_LF<NODE> (0);
	}
	~Queue_LOCK ()
	{
		DATA p;
		while ( 1 )
		{
			
			if ( Dequeue (&p) == false )
			{
				return;
			}
		}
	}


	/*//////////////////////////////////////////////////////////////////////
	//Enqueue
	//���� : DATA
	//return : true,false
	//////////////////////////////////////////////////////////////////////*/

	bool Enqueue (DATA Data)
	{

		//Queue�� ��á���Ƿ� return false.
		if ( _NodeCnt >= _MaxCnt )
		{
			return false;
		}

		_TOP_NODE PreNode;


		NODE *pNode = _pMemPool->Alloc ();
		pNode->Data = Data;
		pNode->pNext = NULL;

		LOCK ();

		PreNode.pNode = _pTail->pNode;

		_pTail->pNode->pNext = pNode;
		_pTail->pNode = pNode;
		
		InterlockedIncrement64 (&_NodeCnt);

		Free ();
		return true;
	}


	/*//////////////////////////////////////////////////////////////////////
	//Dequeue
	//���� : ���� �����͸� ���� ����.
	//return : true,false
	//////////////////////////////////////////////////////////////////////*/
	bool Dequeue (DATA *pOut)
	{
		_TOP_NODE PreNode;
		NODE *pNode;

		LOCK ();

		PreNode.pNode = _pHead->pNode;

		pNode = PreNode.pNode->pNext;

		//����� Next��带 �̴� ���̹Ƿ� pNode�� NULL�̶�� Dequeue �Ұ���.
		if ( pNode == NULL )
		{
			pOut = NULL;
			Free ();
			return false;
		}

		_pHead->pNode = pNode;

		*pOut = pNode->Data;
		_pMemPool->Free (PreNode.pNode);
		InterlockedDecrement64 (&_NodeCnt);

		Free ();


		return true;
	}



	/*//////////////////////////////////////////////////////////////////////
	//GetUseSize
	//���� : ����
	//return : ���� Node����.
	//////////////////////////////////////////////////////////////////////*/
	INT64 GetUseSize (void)
	{
		return _NodeCnt;
	}


	void LOCK (void)
	{
		AcquireSRWLockExclusive (&_CS);
	}
	void Free (void)
	{
		ReleaseSRWLockExclusive (&_CS);
	}
};