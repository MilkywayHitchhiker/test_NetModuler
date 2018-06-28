#include "stdafx.h"
#include <windows.h>
#include "PacketPool.h"

//#include"ServerConfig.h"

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

Packet::Packet (unsigned char PacketCode, char XOR_Code1, char XOR_Code2, int iBufferSize = 0)
{

	Buffer = NULL;
	BufferExpansion = NULL;

	_PacketCode = PacketCode;
	_XORCode1 = XOR_Code1;
	_XORCode2 = XOR_Code2;
	srand (time (NULL));

	//���� ����� �Է����� �ʴ´ٸ�, �⺻������� ����.
	if ( iBufferSize == 0 )
	{
		Initial ();
	}
	else
	{
		Initial (iBufferSize);
	}

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
	InitializeSRWLock (&_CS);

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



	/*
	_PacketCode = _PACKET_CODE;
	_XORCode1 = _PACKET_KEY1;
	_XORCode2 = _PACKET_KEY2;
	*/
	srand (time (NULL));


	_iDataSize = 0;
	HeaderSize = 0;
	iRefCnt = 1;
	DeQCount = 0;
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

Packet &Packet::operator << (WCHAR &chValue)
{
	PutData (( char * )&chValue, sizeof (WCHAR));
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

Packet &Packet::operator >> (WCHAR &chValue)
{
	GetData ((char *)&chValue, sizeof (WCHAR));
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
	InterlockedIncrement (( volatile LONG * )&DeQCount);

	//����� �ϴ� ��ŭ�� ����Ÿ�� ���ٸ�.
	if ( iSize > _iDataSize )
	{
		ErrorAlloc err;
		err.PutSize = 0;
		err.UseHeaderSize = 0;
		err.GetSize = iSize;
		err.UseDataSize = _iDataSize;
		err.Get = true;
		throw err;
	}

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
	{
			ErrorAlloc err;
			err.GetSize = 0;
			err.UseHeaderSize = 0;
			err.PutSize = iSrcSize;
			err.UseDataSize = _iDataSize;

			err.Get = false;
			throw err;
	}

	memcpy(WritePos, chpSrc, iSrcSize);
	WritePos += iSrcSize;
	if ( iSrcSize < 4 )
	{
		throw;
	}
	_iDataSize += iSrcSize;

	return iSrcSize;
}

// ��� ����.
int	Packet::PutHeader (char *chpSrc, int iSrcSize)
{
	if ( iSrcSize > HEADERSIZE_DEFAULT - HeaderSize )
	{
		ErrorAlloc err;
		err.GetSize = 0;
		err.PutSize = 0;
		err.UseDataSize = _iDataSize;
		err.UseHeaderSize = HeaderSize;
		err.Get = false;
		throw err;
	}
		
	char *PrePos = HeaderStartPos - iSrcSize;
	memcpy (PrePos, chpSrc, iSrcSize);
	HeaderStartPos = PrePos;

	_iDataSize += iSrcSize;
	HeaderSize += iSrcSize;
	return  HeaderSize;
}



//===============================================================================================
//= ������ ��ȣȭ ����
//1. Rand XOR Code ����
//2. Payload �� checksum ���
//3. Rand XOR Code ��[CheckSum, Payload] ����Ʈ ���� xor
//4. ���� XOR Code 1 ��[Rand XOR Code, CheckSum, Payload] �� XOR
//5. ���� XOR Code 2 ��[Rand XOR Code, CheckSum, Payload] �� XOR
//===============================================================================================
bool Packet::EnCode (void)
{
	char *ReadPosBuff;
	
	AcquireLOCK ();
	if ( EnCodeFlag )
	{
		ReleaseLOCK ();
		return true;
	}
	EnCodeFlag = true;

	int DataSize = _iDataSize;
	unsigned char XORCode1 = _XORCode1;
	unsigned char XORCode2 = _XORCode2;

	HEADER Header;

//	Header.Code = _PACKET_CODE;
//	Header.Len = DataSize;

	//1. Rand XOR Code �� ������ �̰� �����ϰ� 1byte �ڵ带 ����
	Header.RandXOR = rand () % 255;

	//2. CheckSum Payload �κ��� 1byte �� ��� ���ؼ� % 256 �� unsigned char ��
	ReadPosBuff = ReadPos;
	int CheckSum = 0;
	for ( int Cnt = 0; Cnt < DataSize; Cnt++ )
	{
		CheckSum += ReadPosBuff[Cnt];
	}
	Header.CheckSum = CheckSum % 256;

	//3. Rand XOR Code�� ChecSum, Payload ����Ʈ ���� xor
	ReadPosBuff = ReadPos;
	Header.CheckSum = Header.CheckSum ^ Header.RandXOR;
	for ( int Cnt = 0; Cnt < DataSize; Cnt++ )
	{
		ReadPosBuff[Cnt] = ReadPosBuff[Cnt] ^ Header.RandXOR;
	}

	//4. ���� XOR Code 1 ��[Rand XOR Code, CheckSum, Payload] �� XOR
	ReadPosBuff = ReadPos;
	Header.RandXOR = Header.RandXOR ^ XORCode1;
	Header.CheckSum = Header.CheckSum ^ XORCode1;
	for ( int Cnt = 0; Cnt < DataSize; Cnt++ )
	{
		ReadPosBuff[Cnt] = ReadPosBuff[Cnt] ^ XORCode1;
	}

	//5. ���� XOR Code 2 ��[Rand XOR Code, CheckSum, Payload] �� XOR
	ReadPosBuff = ReadPos;
	Header.RandXOR = Header.RandXOR ^ XORCode2;
	Header.CheckSum = Header.CheckSum ^ XORCode2;
	for ( int Cnt = 0; Cnt < DataSize; Cnt++ )
	{
		ReadPosBuff[Cnt] = ReadPosBuff[Cnt] ^ XORCode2;
	}

	//�������� Buff�� Data�� HeaderPos�� �ű�.
	PutHeader ((char *)&Header.CheckSum, 1);
	PutHeader (( char * )&Header.RandXOR, 1);
	PutHeader (( char * )&Header.Len, 2);
	PutHeader (( char * )&Header.Code, 1);


	ReleaseLOCK ();

	return true;
}





//===============================================================================================
//= �ޱ� ��ȣȭ ����
//1. ���� XOR Code 2 ��[Rand XOR Code, CheckSum, Payload] �� XOR
//2. ���� XOR Code 1 ��[Rand XOR Code, CheckSum, Payload] �� XOR
//3. Rand XOR Code �� �ľ�.
//4. Rand XOR Code ��[CheckSum - Payload] ����Ʈ ���� xor
//5. Payload �� checksum �������� ��� �� ��Ŷ�� checksum �� ��
//===============================================================================================
bool Packet::DeCode (HEADER *SrcHeader)
{
	HEADER Buff;
	if ( SrcHeader == NULL )
	{
		GetData (( char * )&Buff.Code, 1);
		GetData (( char * )&Buff.Len, 2);
		GetData (( char * )&Buff.RandXOR, 1);
		GetData (( char * )&Buff.CheckSum, 1);
	}
	else
	{
		Buff.CheckSum = SrcHeader->CheckSum;
		Buff.Code = SrcHeader->Code;
		Buff.Len = SrcHeader->Len;
		Buff.RandXOR = SrcHeader->RandXOR;
	}

	unsigned char XORCode1 = _XORCode1;
	unsigned char XORCode2 = _XORCode2;
	char *ReadPosBuff = ReadPos;
	int DataSize = _iDataSize;


	//1. ���� XOR Code 2 ��[Rand XOR Code, CheckSum, Payload] �� XOR
	Buff.RandXOR = Buff.RandXOR ^ XORCode2;
	Buff.CheckSum = Buff.CheckSum ^ XORCode2;
	for ( int Cnt = 0; Cnt < DataSize; Cnt++ )
	{
		ReadPosBuff[Cnt] = ReadPosBuff[Cnt] ^ XORCode2;
	}


	//2. ���� XOR Code 1 ��[Rand XOR Code, CheckSum, Payload] �� XOR
	ReadPosBuff = ReadPos;
	Buff.RandXOR = Buff.RandXOR ^ XORCode1;
	Buff.CheckSum = Buff.CheckSum ^ XORCode1;
	for ( int Cnt = 0; Cnt < DataSize; Cnt++ )
	{
		ReadPosBuff[Cnt] = ReadPosBuff[Cnt] ^ XORCode1;
	}


	//3. Rand XOR Code �� �ľ�.
	//4. Rand XOR Code ��[CheckSum - Payload] ����Ʈ ���� xor
	ReadPosBuff = ReadPos;
	Buff.CheckSum = Buff.CheckSum ^ Buff.RandXOR;
	for ( int Cnt = 0; Cnt < DataSize; Cnt++ )
	{
		ReadPosBuff[Cnt] = ReadPosBuff[Cnt] ^ Buff.RandXOR;
	}

	//5. Payload �� checksum �������� ��� �� ��Ŷ�� checksum �� ��
	ReadPosBuff = ReadPos;
	int CheckSum = 0;
	for ( int Cnt = 0; Cnt < DataSize; Cnt++ )
	{
		CheckSum += ReadPosBuff[Cnt];
	}
	unsigned char Chk = ( unsigned char )CheckSum % 256;

	if ( Buff.CheckSum != Chk )
	{
		return false;
	}
	return true;
}