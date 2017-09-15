// TGCServer.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "SocketManager.h"
#include "MindwaveManager.h"

#define NEW_CONN_FREEZE_TIME 2000 // 2 seconds freezing time

using namespace rapidjson;

int main()
{
	int err_code = 0;

	/*while (true)
	{
		SocketManager socket_manager;
		err_code = socket_manager.Handle_Connection();
		if (err_code < 0) {
			printf("Receiving socket fails!\n");
			Sleep(NEW_CONN_FREEZE_TIME);
		}
	}*/
	
	
	MindwaveManager mindwave_manager;
    return 0;
}

