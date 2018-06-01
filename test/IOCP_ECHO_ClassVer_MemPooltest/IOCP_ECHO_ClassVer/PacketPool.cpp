#include "stdafx.h"
#include <windows.h>
#include "PacketPool.h"

CMemoryPool<Packet> *Packet::PacketPool;


Packet::Packet() : Buffer (NULL),DataFieldStart (NULL),DataFieldEnd (NULL),ReadPos (NULL),WritePos (NULL)
{
	//���� ����� �Է����� �ʴ´ٸ�, �⺻ġ�� ����.
	Buffer = NULL;
	BufferExpansion = NULL;

	Initial();
	return;
}


Packet::Packet(int iBufferSize) : Buffer (NULL), DataFieldStart (NULL), DataFieldEnd (NULL), ReadPos (NULL), WritePos (NULL)
{
	Buffer = NULL;
	BufferExpansion = NULL;

	Initial(iBufferSize);
	return;
}

Packet::Packet(const Packet &SrcPacket) : Buffer (NULL),_iBufferSize (0), DataFieldStart (NULL), DataFieldEnd (NULL), ReadPos (NULL), WritePos (NULL)
{
	Buffer = NULL;
	BufferExpansion = NULL;
	Initial(SrcPacket._iBufferSize);
	
	/*-----------------------------------------------------------------
	���� ��Ŷ Ŭ�������� �����ؿ´�.
	-------------------------------------------------------------------*/
	PutData(SrcPacket.ReadPos, SrcPacket._iDataSize);
	return;
}


Packet::~Packet()
{
	Release();

	return;
}






// ��Ŷ �ʱ�ȭ.
void Packet::Initial(int iBufferSize)
{
	_iBufferSize = iBufferSize;

	if ( NULL == Buffer )
	{
		if ( BUFFER_DEFAULT < _iBufferSize )
		{
			BufferExpansion = new char[_iBufferSize];
			Buffer = BufferExpansion;
		}
		else
		{
			BufferExpansion = NULL;
			Buffer = BufferDefault;
		}
	}

	DataFieldStart = Buffer+HEADERSIZE_DEFAULT;
	DataFieldEnd = Buffer + (_iBufferSize- HEADERSIZE_DEFAULT);

	ReadPos = WritePos = HeaderStartPos = DataFieldStart;

	_iDataSize = 0;
	HeaderSize = 0;
	iRefCnt = 1;
	
	return;
}

//RefCnt�� 1 ������Ŵ. 
void Packet::Add (void)
{
	InterlockedIncrement (( volatile long * )&iRefCnt);
	return;
}

// RefCnt�� �ϳ� ������Ű�� REfCnt�� 0�� �Ǹ� �ڱ��ڽ� delete�ϰ� ��������.
void Packet::Release (void)
{
	if ( NULL != BufferExpansion )
	{
		delete[] BufferExpansion;
	}
	delete this;
	return;
}


// ��Ŷ �ʱ�ȭ
void Packet::Clear(void)
{

	DataFieldStart = Buffer + HEADERSIZE_DEFAULT;
	DataFieldEnd = Buffer + (_iBufferSize - HEADERSIZE_DEFAULT);

	ReadPos = WritePos = HeaderStartPos = DataFieldStart;

	_iDataSize = 0;
}


// Wirtepos �̵�. (�����̵��� �ȵ�)
int Packet::MoveWritePos(int iSize)
{
	if ( 0 > iSize ) return 0;

	/*-----------------------------------------------------------------
	 �̵��� �ڸ��� �����ϴٸ�.
	-------------------------------------------------------------------*/
	if ( WritePos + iSize > DataFieldEnd )
		return 0;

	WritePos += iSize;
	_iDataSize += iSize;

	return iSize;
}

// ���� ReadPos �̵�.
int Packet::MoveReadPos(int iSize)
{
	if ( 0 > iSize ) return 0;

	// �̵��Ҹ�ŭ ����Ÿ�� ���ٸ�.
	if ( iSize > _iDataSize )
		return 0;

	ReadPos += iSize;
	_iDataSize -= iSize;

	return iSize;
}








//////////////////////////////////////////////////////////////////////////
// ������ ���۷����� ����
//////////////////////////////////////////////////////////////////////////
Packet &Packet::operator = (Packet &SrcPacket)
{
	ReadPos = Buffer;
	WritePos = Buffer;

	//���� ��Ŷ Ŭ�������� �����ؿ´�.
	PutData(SrcPacket.ReadPos, SrcPacket._iDataSize);

	return *this;
}

// �ֱ�.	�� ���� Ÿ�Ը��� ��� ����.
Packet &Packet::operator << (BYTE byValue)
{
	PutData(( char * )&byValue, sizeof(BYTE)); //ĳ���� �⺻(char *)&value
	return *this;
}

Packet &Packet::operator << (char chValue)
{
	PutData(&chValue, sizeof(char));
	return *this;
}

Packet &Packet::operator << (short shValue)
{
	PutData(( char * )&shValue, sizeof(short));
	return *this;
}

Packet &Packet::operator << (WORD wValue)
{
	PutData(( char * )&wValue, sizeof(WORD));
	return *this;
}

Packet &Packet::operator << (int iValue)
{
	PutData(reinterpret_cast<char *>(&iValue), sizeof(int));

	return *this;
}

Packet &Packet::operator << (DWORD dwValue)
{
	PutData(( char * )&dwValue, sizeof(DWORD));
	return *this;
}

Packet &Packet::operator << (float fValue)
{
	PutData(( char * )&fValue, sizeof(float));
	return *this;
}

Packet &Packet::operator << (__int64 iValue)
{
	PutData(( char * )&iValue, sizeof(__int64));
	return *this;
}

Packet &Packet::operator << (double dValue)
{
	PutData((char *)&dValue, sizeof(double));
	return *this;
}

//////////////////////////////////////////////////////////////////////////
// ����.	�� ���� Ÿ�Ը��� ��� ����.
//////////////////////////////////////////////////////////////////////////
Packet &Packet::operator >> (BYTE &byValue)
{
	GetData((char *)&byValue, sizeof(BYTE));
	return *this;
}

Packet &Packet::operator >> (char &chValue)
{
	GetData(&chValue, sizeof(char));
	return *this;
}

Packet &Packet::operator >> (short &shValue)
{
	GetData((char *)&shValue, sizeof(short));
	return *this;
}

Packet &Packet::operator >> (WORD &wValue)
{
	GetData((char *)&wValue, sizeof(WORD));
	return *this;
}

Packet &Packet::operator >> (int &iValue)
{
	GetData((char *)&iValue, sizeof(int));
	return *this;
}

Packet &Packet::operator >> (DWORD &dwValue)
{
	GetData((char *)&dwValue, sizeof(DWORD));
	return *this;
}

Packet &Packet::operator >> (float &fValue)
{
	GetData((char *)&fValue, sizeof(float));
	return *this;
}

Packet &Packet::operator >> (__int64 &iValue)
{
	GetData((char *)&iValue, sizeof(__int64));
	return *this;
}

Packet &Packet::operator >> (double &dValue)
{
	GetData((char *)&dValue, sizeof(double));
	return *this;
}







// ����Ÿ ���.
int Packet::GetData(char *chpDest, int iSize)
{
	//����� �ϴ� ��ŭ�� ����Ÿ�� ���ٸ�.
	if ( iSize > _iDataSize )
		return 0;

	memcpy(chpDest, ReadPos, iSize);
	ReadPos += iSize;

	_iDataSize -= iSize;

	return iSize;

}


// ����Ÿ ����.
int Packet::PutData(char *chpSrc, int iSrcSize)
{
	//���� �ڸ��� ���ٸ�.
	if ( WritePos + iSrcSize > DataFieldEnd )
		return 0;

	memcpy(WritePos, chpSrc, iSrcSize);
	WritePos += iSrcSize;

	_iDataSize += iSrcSize;

	return iSrcSize;
}

// ��� ����.
int	Packet::PutHeader (char *chpSrc, int iSrcSize)
{
	if ( HeaderStartPos - iSrcSize < Buffer )
		return 0;

	char *PrePos = HeaderStartPos - iSrcSize;
	memcpy (PrePos, chpSrc, iSrcSize);
	HeaderStartPos = PrePos;

	_iDataSize += iSrcSize;

	return  iSrcSize;
}