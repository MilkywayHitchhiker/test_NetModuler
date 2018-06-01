#pragma once



#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

#include <process.h>
#include "lib\\Library.h"
#include "PacketPool.h"
#include "RingBuffer.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"

/*======================================================================
//LAN Server Module
//�ش� Ŭ������ TCP��� Ŭ������ �ʿ��� ��� ��ӹ޾Ƽ� ����Ұ�.
======================================================================*/
class CLanServer
{
#define SendbufMax 100
protected:
	struct IOChk
	{
		int IOCount = 0;
		int UseFlag = false;
	};


	struct Session
	{
		IOChk p_IOChk;
		SOCKET sock;
		UINT64 SessionID;

		
		long SendFlag = FALSE;
		CQueue_LF<Packet *> SendQ;
		OVERLAPPED SendOver;
		CStack_LF<Packet *> SendPack;

		CRingbuffer RecvQ;
		OVERLAPPED RecvOver;
		
	};

	HANDLE _IOCP;
	SOCKET _ListenSock;
	int _Session_Max;
	Session *Session_Array;
	CStack_LF<int> emptySession;

	char _SessionID_Count[6];

	UINT _RecvPacketTPS;
	UINT _SendPacketTPS;
	UINT _AcceptTotal;
	UINT _AcceptTPS;
	UINT _Use_Session_Cnt;

	HANDLE *Thread;
	int _WorkerThread_Num;

	bool bServerOn;

	/*======================================================================
	//������
	//���� : ����
	======================================================================*/
	CLanServer (void);
	
	/*======================================================================
	//�ı���
	//���� : ����
	======================================================================*/
	~CLanServer (void);

	/*======================================================================
	//AcceptThread
	//���� : ���� AcceptThread.
	//���� : ����
	//���� : ����
	======================================================================*/
	void AcceptThread (void);


	/*======================================================================
	//WorkerThread
	//���� : ���� WorkerThread. GQCS�� Recv�� Send �Ϸ������� ����.
	//���� : ����
	//���� : ����
	======================================================================*/
	void WorkerThread (void);



	/*======================================================================
	//InitializeNetwork
	//���� : Start�� ���� On�� Listen���� �ʱ�ȭ �� bind, listen �Լ�
	//���� : WCHAR * IP, int ��Ʈ��ȣ
	//���� : ��������
	======================================================================*/
	bool InitializeNetwork (WCHAR *IP,int PORT);



	/*======================================================================
	//FindLockSession
	//���� : ���ǰ˻� �� �ش缼�ǿ� ���� IOī��Ʈ ������ ���� �Ŵ� �۾�.
	//���� : UINT64 SessionID
	//���� : Session *, NULL���Ͻ� ����.
	======================================================================*/
	Session *FindLockSession (UINT64 SessionID);


	/*======================================================================
	//CreateSessionID
	//���� : ���ڷ� ���� �ε����� ����ũ���� ��Ʈ�����Ͽ� ����ID����.
	//���� : short index, UINT64 Unique
	//���� : UINT64 SessionID
	======================================================================*/
	UINT64 CreateSessionID (short index, UINT64 Unique)
	{
			return ((UINT64)index << 48) | (Unique);
	}


	/*======================================================================
	//indexSessionID
	//���� : ���ڷ� ���� ����ID���� �ε����� ����
	//���� : UINT64 SessionID
	//���� : short INDEX
	======================================================================*/
	short indexSessionID (UINT64 SessionID)
	{
		return (short)(SessionID >> 48);
	}


	/*======================================================================
	//PostRecv
	//���� : RecvQ�� WSARecv�� ����ϴ� �۾�.
	//���� : Session *
	//���� : ����
	======================================================================*/
	void PostRecv (Session *p);


	/*======================================================================
	//PostSend
	//���� : SendQ�� �ִ� Packet�� WSASend�� POST
	//���� : Session *
	//���� : ����
	======================================================================*/
	void PostSend (Session *p);



	/*======================================================================
	//SessionRelease
	//���� : IOCount�� 0�̸� ����. UseFlag�� false�� SessionRelease�۾� ����.
	//���� : Session *
	//���� : ����
	======================================================================*/
	void SessionRelease (Session *p);


	/*======================================================================
	//IODeCrement
	//���� : ���ڷ� ���� �ش� ������ IOCount ���� �� IOCount�� 0�Ͻ� Session Release�Լ�ȣ��
	//���� : Session *
	//���� : ����
	======================================================================*/
	void IODecrement (Session *p);

public :

	/*======================================================================
	//OnRecv
	//���� : virtual �Լ�. ��Ŷ�� Recv�Ǹ� �ش� �Լ��� ȣ��ȴ�.
	//���� : UINT64 SessionID, Packet *p
	//���� : ����
	======================================================================*/
	virtual void OnRecv (UINT64 SessionID, Packet *p) = 0;


	/*======================================================================
	//OnSend
	//���� : virtual �Լ�. Send�� �Ϸ�Ǹ� �ش��Լ��� ȣ��ȴ�.
	//���� : UINT64 SessionID, INT SendByte
	//���� : ����
	======================================================================*/
	virtual void OnSend (UINT64 SessionID, INT SendByte) = 0;


	/*======================================================================
	//OnClientJoin
	//���� : virtual �Լ�. ���ο� Client�� �����ϸ� �ش��Լ��� ȣ��ȴ�.
	//���� : UINT64 SessionID, WCHAR *IP, int PORT
	//���� : ����
	======================================================================*/
	virtual bool OnClientJoin (UINT64 SessionID, WCHAR *IP, int PORT) = 0;

	/*======================================================================
	//OnClientLeave
	//���� : virtual �Լ�. Client�� ���������Ǹ� �ش��Լ��� ȣ��ȴ�.
	//���� : UINT64 SessionID
	//���� : ����
	======================================================================*/
	virtual void OnClientLeave (UINT64 SessionID) = 0;


	/*======================================================================
	//SendPacket
	//���� : Packet�� SendQ�� �ְ� WSASend�Լ� ȣ��
	//���� : UINT64 SessionID, Packet *
	//���� : ����
	======================================================================*/
	void SendPacket (UINT64 SessionID, Packet *pack);


	/*======================================================================
	//Disconnect
	//���� : �ش� ������ TCP ���� ���� ��û �Լ�.
	//���� : UINT64 SessionID
	//���� : ����
	======================================================================*/
	void Disconnect (UINT64 SessionID);


	/*======================================================================
	//Start
	//���� : NetworkServer On
	//���� : WCHAR * ����IP, int PORT��ȣ, int �������� Session��, int ��Ŀ������ �ִ����
	//���� : ���� �۵� ����. true false;
	======================================================================*/
	bool Start (WCHAR *ServerIP, int PORT, int Session_Max, int WorkerThread_Num);


	/*======================================================================
	//Stop
	//���� : NetworkServer Off
	//���� : ����
	//���� : ���� ���� ����. true false;
	======================================================================*/
	bool Stop (void);


	/*////////////////////////////////////////////////////////////////////////////////////////////////////
	//�ܺο��� Ȯ�� �� �� �ִ� ��ġ �������̽�����.
	////////////////////////////////////////////////////////////////////////////////////////////////////*/
	/*======================================================================
	//RecvTPS
	//���� : �ʴ� Recv��.
	//���� : bool Reset����. true�� ��� RecvTPS�� 0���� �ʱ�ȭ��.
	//���� : UINT
	======================================================================*/
	UINT RecvTPS (bool Reset)
	{
		UINT RecvTPS = _RecvPacketTPS;
		if ( Reset )
		{
			InterlockedExchange ((volatile LONG *)&_RecvPacketTPS, 0);
		}
		return RecvTPS;
	}

	/*======================================================================
	//SendTPS
	//���� : �ʴ� Send��.
	//���� : bool Reset����. true�� ��� SendTPS�� 0���� �ʱ�ȭ��.
	//���� : UINT
	======================================================================*/
	UINT SendTPS (bool Reset)
	{
		UINT SendTPS = _SendPacketTPS;
		if ( Reset )
		{
			InterlockedExchange (( volatile LONG * )&_SendPacketTPS, 0);
		}
		return SendTPS;
	}


	/*======================================================================
	//AcceptTotal
	//���� : ������ �����ڷ� �����ؿ� Ŭ���̾�Ʈ�� �� ����
	//���� : ����
	//���� : UINT
	======================================================================*/
	UINT AcceptTotal (void)
	{
		return _AcceptTotal;
	}


	/*======================================================================
	//AcceptTPS
	//���� : �ʴ� Accept��
	//���� : bool Reset����. true�� ��� AcceptTPS�� 0���� �ʱ�ȭ��.
	//���� : UINT
	======================================================================*/
	UINT AcceptTPS (bool Reset)
	{
		UINT AcceptTPS = _AcceptTPS;
		if ( Reset )
		{
			InterlockedExchange (( volatile LONG * )&_AcceptTPS, 0);
		}
		return AcceptTPS;
	}

	/*======================================================================
	//Use_SessionCnt
	//���� : ���� �������ִ� Ŭ���̾�Ʈ�� ����.
	//���� : ����
	//���� : UINT
	======================================================================*/
	UINT Use_SessionCnt (void)
	{
		return _Use_Session_Cnt;
	}

	/*======================================================================
	//Full_MemPoolCnt
	//���� : ��ŶǮ�� �� ������
	//���� : ����
	//���� : double
	======================================================================*/
	int Full_MemPoolCnt (void)
	{
		return Packet::PacketPool->GetFullCount ();
	}

	/*======================================================================
	//Alloc_MemPoolCnt
	//���� : ���� Alloc���� Packet�� �� ������
	//���� : ����
	//���� : double
	======================================================================*/
	int Alloc_MemPoolCnt (void)
	{
		return Packet::PacketPool->GetAllocCount ();
	}
protected:

	/*======================================================================
	//AcceptThread
	//���� : AcceptThread() �Լ� ����.
	//���� : LPVOID pParam; = CLanServer this pointer �� �Ϸ� �Ѱܹ���.
	//���� : 0
	======================================================================*/
	static unsigned int WINAPI AcceptThread (LPVOID pParam);


	/*======================================================================
	//WorkerThread
	//���� : WorkerThread() �Լ� ����.
	//���� : LPVOID pParam; = CLanServer this pointer �� �Ϸ� �Ѱܹ���.
	//���� : 0
	======================================================================*/
	static unsigned int WINAPI WorkerThread (LPVOID pParam);
};
