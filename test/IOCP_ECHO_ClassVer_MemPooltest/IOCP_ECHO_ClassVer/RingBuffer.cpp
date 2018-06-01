
#include"stdafx.h"
#include"RingBuffer.h"


//������
CRingbuffer::CRingbuffer (void)
{

	pBuffer = new char[dfBuffSize];
	BufferSize = dfBuffSize;
	Front = 0;
	Rear = 0;

	//ũ��Ƽ�� ���� �ʱ�ȭ
	InitializeSRWLock (&cs);

	return;
}
CRingbuffer::CRingbuffer (int iBufferSize)
{

	pBuffer = new char[iBufferSize];
	BufferSize = iBufferSize;
	Front = 0;
	Rear = 0;

	//ũ��Ƽ�� ���� �ʱ�ȭ
	InitializeSRWLock (&cs);

	return;
}

CRingbuffer::~CRingbuffer (void)
{
	delete[]pBuffer;
	return;
}

//�ʱ�ȭ
void CRingbuffer::Initial (int iBufferSize)
{
	delete []pBuffer;

	pBuffer = new char[iBufferSize];
	BufferSize = iBufferSize;
	Front = 0;
	Rear = 0;

	return;
}


//ũ��Ƽ�� ���� ��
void CRingbuffer::Lock (void)
{
	AcquireSRWLockExclusive (&cs);
	return;
}
//ũ��Ƽ�� ���� �� ����
void CRingbuffer::Free (void)
{
	ReleaseSRWLockExclusive (&cs);
	return;
}

//���� ���� ũ��(��� ���ϴ� 1byte����) ��ȯ.
int CRingbuffer::GetBufferSize (void)
{
	return BufferSize - 1;
}


//���� ������� ���� ũ�� ��ȯ
int CRingbuffer::GetUseSize (void)
{
	int Size;
	//���� ��ġ�� �б� ��ġ���� �ڿ� �ִٸ�.
	if ( Rear > Front || Rear == Front)
	{
		Size = Rear - Front;
	}

	//�б� ��ġ�� ���� ��ġ���� �ڿ� �ִٸ�.
	else if ( Front > Rear )
	{
		Size = (BufferSize - Front) + Rear;
	}
	else
	{
		Size = 0;
	}
	return Size;
}


//���� ��밡���� ���� ũ�� ��ȯ
int CRingbuffer::GetFreeSize (void)
{
	int Size;
	//���� ��ġ�� �б� ��ġ���� �ڿ� �ִٸ�.
	if ( Rear > Front || Rear == Front )
	{
		//��ũ�⿡�� ������ġ�� ���� Front������ ����� ���Ѵ�. �����۶� �ѻ����� -1;
		Size = (BufferSize - Rear) + (Front - 1);
	}
	//�б� ��ġ�� ������ġ���� �ڿ� �ִٸ�
	else if ( Front > Rear )
	{
		Size = (Front - Rear) - 1;
	}
	else
	{
		Size = 0;
	}
	return Size;
}

//���� �����ͷ� �ܺο��� �ѹ濡 ������ �ִ� ����
int CRingbuffer::GetNotBrokenGetSize (void)
{
	int Size;

	//���� ��ġ�� �б� ��ġ���� �ڿ� �ִٸ�
	//�б� ��ġ���� ������ġ���� �ѹ濡 �ܾ�� �� �����Ƿ�
	//������ġ - �б���ġ�� Size�� ��ȯ�Ѵ�.
	if ( Rear > Front || Rear == Front)
	{
		Size = Rear - Front;
	}

	//�б� ��ġ�� ���� ��ġ���� �ڿ� �ִٸ�
	//�б� ��ġ���� �������� ������ ���ٰ� �ٽ� ó������ Rear���� �о�� �ǹǷ�
	//�б� ��ġ���� ������ �������� Size�� ��ȯ�Ѵ�.
	else if ( Front > Rear )
	{
		Size =  BufferSize - Front;
	}
	else
	{
		Size = 0;
	}

	return Size;
}


//���� �����ͷ� �ܺο��� �ѹ濡 ���� �ִ� ����
int CRingbuffer::GetNotBrokenPutSize (void)
{
	int Size;

	//���� ��ġ�� �б� ��ġ���� �ڿ� �ִٸ�
	//���� ��ġ���� �������� ������ �ѹ��� �� �� �����Ƿ�
	//������ġ���� ������ �������� Size�� ��ȯ�Ѵ�.
	if ( Rear > Front || Rear == Front)
	{
		Size = BufferSize - Rear;
	}

	//�б� ��ġ�� ���� ��ġ���� �ڿ� �ִٸ�
	//������ġ���� �б� ��ġ���� �ѹ��� �� �� �����Ƿ�
	//�б� ��ġ - ������ġ �� ���� �����۱⶧���� Front Rear�ߺ������� ���� -1
	else if ( Front > Rear )
	{
		Size = (Front - Rear) - 1;
	}

	return Size;
}


//WritePos�� ����Ÿ ����
int CRingbuffer::Put (char *chpData, int iSize)
{
	int PutSize = iSize;
	int CutSizeOne;
	int CutSizeTwo;
	int _Rear = Rear;	
	int _Front = Front;
	int ReturnSize = 0;


	//�־�ߵ� ����� ���� �� �ִ� ������� ū ���.
	if ( iSize > GetFreeSize () )
	{
		PutSize = GetFreeSize ();
	}

	//������ġ�� �б���ġ���� �ڿ� �������
	if ( _Rear > _Front || _Rear == _Front)
	{
		//������ġ + Size�� ������ ������� �������
		if ( _Rear + PutSize < BufferSize )
		{
			memcpy_s (&pBuffer[_Rear], PutSize, chpData, PutSize);
			_Rear = (_Rear + PutSize) % BufferSize;
			ReturnSize += PutSize;
		}
		//������ġ + Size�� ������ ������� Ŭ���. = Front�� Rear���� Ŀ�� ���.
		else if ( _Rear + PutSize >= BufferSize )
		{
			CutSizeOne = GetNotBrokenPutSize ();
			memcpy_s (&pBuffer[_Rear], CutSizeOne, chpData, CutSizeOne);
			_Rear = (_Rear + CutSizeOne) % BufferSize;
			ReturnSize += CutSizeOne;

			CutSizeTwo = PutSize - CutSizeOne;
			memcpy_s (&pBuffer[_Rear], CutSizeTwo, &chpData[CutSizeOne], CutSizeTwo);
			_Rear = (_Rear + CutSizeTwo) % BufferSize;
			ReturnSize += CutSizeTwo;

		}
	}

	//�б� ��ġ�� ������ġ���� �ڿ� ���� ���
	else if ( _Rear < _Front )
	{
		memcpy_s (&pBuffer[_Rear], PutSize, chpData, PutSize);
		ReturnSize += PutSize;
		_Rear = (_Rear + PutSize) % BufferSize;
	}
	

	Rear = _Rear;
	return ReturnSize;
	
}

//ReadPos���� ������ ������. ReadPos�̵�.
int CRingbuffer::Get (char *chpDest, int iSize)
{
	int _Front = Front;
	int _Rear = Rear;
	int GetSize = iSize;
	int CutSizeOne,CutSizeTwo;
	int ReturnSize = 0;


	//�о�ߵ� ����� ���� �� �ִ� ������� Ŭ ���
	if ( iSize > GetUseSize () )
	{
		GetSize = GetUseSize ();
	}

	//���� ��ġ�� �б� ��ġ���� �ڿ� ���� ���
	if ( _Rear > _Front || _Rear == _Front)
	{
		memcpy_s (chpDest, GetSize, &pBuffer[_Front], GetSize);
		_Front = (_Front + GetSize) % BufferSize;
		ReturnSize += GetSize;
	}

	//�б� ��ġ�� ���� ��ġ���� �ڿ����� ���

	else if ( _Front > _Rear )
	{
		//�б���ġ + GetSize�� BufferSize���� ���� ���
		if ( _Front + GetSize <= BufferSize )
		{
			memcpy_s (chpDest, GetSize, &pBuffer[_Front], GetSize);
			_Front = (_Front + GetSize) % BufferSize;
			ReturnSize += GetSize;
		}
		//�б� ��ġ + GetSize�� BufferSize�� ���ų� Ŭ���
		else if ( _Front + GetSize > BufferSize )
		{
			CutSizeOne = GetNotBrokenGetSize ();
			memcpy_s (chpDest, CutSizeOne, &pBuffer[_Front], CutSizeOne);
			_Front = (_Front + CutSizeOne) % BufferSize;
			ReturnSize += CutSizeOne;

			CutSizeTwo = GetSize - CutSizeOne;
			memcpy_s (&chpDest[CutSizeOne], CutSizeTwo, &pBuffer[_Front], CutSizeTwo);
			_Front = (_Front + CutSizeTwo) % BufferSize;
			ReturnSize += CutSizeTwo;
		}
	}
	
	Front = _Front;
	return ReturnSize;

}


//ReadPos���� ������ ������. ReadPos����.
int CRingbuffer::Peek (char *chpDest, int iSize)
{
	int _Front = Front;
	int _Rear = Rear;
	int GetSize = iSize;
	int CutSizeOne, CutSizeTwo;
	int ReturnSize = 0;


	//�о�ߵ� ����� ���� �� �ִ� ������� Ŭ ���
	if ( iSize > GetUseSize () )
	{
		GetSize = GetUseSize ();
	}

	//���� ��ġ�� �б� ��ġ���� �ڿ� ���� ���
	if ( _Rear > _Front || _Rear == _Front )
	{
		memcpy_s (chpDest, GetSize, &pBuffer[_Front], GetSize);
		_Front = (_Front + GetSize) % BufferSize;
		ReturnSize += GetSize;
	}

	//�б� ��ġ�� ���� ��ġ���� �ڿ����� ���

	else if ( _Front > _Rear )
	{
		//�б���ġ + GetSize�� BufferSize���� ���� ���
		if ( _Front + GetSize <= BufferSize )
		{
			memcpy_s (chpDest, GetSize, &pBuffer[_Front], GetSize);
			_Front = (_Front + GetSize) % BufferSize;
			ReturnSize += GetSize;
		}
		//�б� ��ġ + GetSize�� BufferSize�� ���ų� Ŭ���
		else if ( _Front + GetSize > BufferSize )
		{
			CutSizeOne = GetNotBrokenGetSize ();
			memcpy_s (chpDest, CutSizeOne, &pBuffer[_Front], CutSizeOne);
			_Front = (_Front + CutSizeOne) % BufferSize;
			ReturnSize += CutSizeOne;

			CutSizeTwo = GetSize - CutSizeOne;
			memcpy_s (&chpDest[CutSizeOne], CutSizeTwo, &pBuffer[_Front], CutSizeTwo);
			_Front = (_Front + CutSizeTwo) % BufferSize;
			ReturnSize += CutSizeTwo;
		}
	}

	return ReturnSize;
}


//���ϴ� ���̸�ŭ �б� ��ġ���� ����/���� ��ġ �̵�

void CRingbuffer::RemoveData (int iSize)
{
	if ( iSize > GetUseSize () )
	{
		iSize = GetUseSize ();
	}
	Front = (Front + iSize) % BufferSize;
	return;
}
int CRingbuffer::MoveWritePos (int iSize)
{
	if ( iSize > GetFreeSize () )
	{
		iSize = GetFreeSize ();
	}
	Rear = (Rear + iSize) % BufferSize;
	return iSize;
}

//������ ��� ������ ����
void CRingbuffer::ClearBuffer (void)
{
	Front = 0;
	Rear = 0;
	return;
}

//������ ������ ����.
char *CRingbuffer::GetBufferPtr (void)
{
	return pBuffer;
}

//������ ReadPos ������ ����
char *CRingbuffer::GetReadBufferPtr (void)
{
	return &pBuffer[Front];
}

//������ WritePos ������ ����
char *CRingbuffer::GetWriteBufferPtr (void)
{
	return &pBuffer[Rear];
}