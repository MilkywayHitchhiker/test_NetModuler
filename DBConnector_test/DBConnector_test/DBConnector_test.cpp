// DBConnector_test.cpp : �ܼ� ���� ���α׷��� ���� �������� �����մϴ�.
//

#include "stdafx.h"
#include "DBConnector.h"
Hitchhiker::CDBConnector *DBConnector;

int main()
{
	MYSQL_ROW row;
	char ID[10][5];
	char pass[10][5];
	DBConnector = new Hitchhiker::CDBConnector (L"127.0.0.1", L"root", L"dhwjddnr1!", L"test", 3306, 3);

	if ( DBConnector->Connect () == false )
	{
		wprintf (L"Connect���� ���μ��� ����.\n");
		return 0;
	}

	int Cnt;
	for ( Cnt = 0; Cnt < 10; Cnt++ )
	{
		DBConnector->Query (L"INSERT INTO `test`.`autoincrement` (`Data`) VALUES ('%d');", Cnt);
	}
	for ( Cnt = 1; Cnt < 9; Cnt++ )
	{
		DBConnector->Query (L"Select ID,Password FROM `test`.`account` WHERE count = '%d';", Cnt);
		while ( (row = DBConnector->FetchRow()) != NULL )
		{
			printf ("ID = %s, Pass = %s \n", row[0], row[1]);
		}
		DBConnector->FreeResult ();
	}
		
    return 0;
}

