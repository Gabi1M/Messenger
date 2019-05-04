#include "stdafx.h"

// communication library
#include "communication_api.h"
#include "ThreadPool.h"
#include "Persistance.h"

#define MAX_MESSAGE_SIZE 255

typedef struct _CLIENT_DETAIL
{
	CM_SERVER_CLIENT* clientHandle;
	TCHAR* clientName;
	BOOL isConnected;
}CLIENT_DETAIL;

THREAD_POOL threadPool;
DWORD connectedClients = 0;
CLIENT_DETAIL* clients;

TCHAR* clientName;

DWORD sendData(CM_SERVER_CLIENT* destinationClient, CM_DATA_BUFFER* dataToSend, TCHAR* messageToSend)
{
	CM_ERROR error;
	CM_SIZE sendByteCount = 0;
	SIZE_T messageToSendSize = _tcslen(messageToSend);

	char* aux = malloc(MAX_MESSAGE_SIZE);
	wcstombs(aux, messageToSend, MAX_MESSAGE_SIZE);

	error = CopyDataIntoBuffer(dataToSend, (const CM_BYTE*)aux, (CM_SIZE)messageToSendSize);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("Copy data into buffer failed with error code: 0x%X\n"), GetLastError());
		return ERROR_ERRORS_ENCOUNTERED;
	}

	error = SendDataToClient(destinationClient, dataToSend, &sendByteCount);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("Send data to client failed with error code: 0x%X\n"), GetLastError());
		return ERROR_ERRORS_ENCOUNTERED;
	}

	return ERROR_SUCCESS;
}

DWORD receiveData(CM_SERVER_CLIENT* sourceClient, CM_DATA_BUFFER* dataToReceive, TCHAR* bufferedMessage)
{
	CM_ERROR error;
	CM_SIZE receivedByteCount = 0;
	char* aux = malloc(MAX_MESSAGE_SIZE);

	error = ReceiveDataFromClient(sourceClient, dataToReceive, &receivedByteCount);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("ReceiveDataFromClient failed with error code: 0x%X\n"), GetLastError());
		return ERROR_ERRORS_ENCOUNTERED;
	}

	strcpy(aux, (char*)dataToReceive->DataBuffer);
	mbstowcs(bufferedMessage, aux, MAX_MESSAGE_SIZE);

	return ERROR_SUCCESS;
}

DWORD initDataBuffer(CM_DATA_BUFFER** buffer, CM_SIZE size)
{
	CM_ERROR error;
	error = CreateDataBuffer(buffer, size);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("Failed to create data buffer with error code: 0x%X\n"), GetLastError());
		return ERROR_ERRORS_ENCOUNTERED;
	}
	return ERROR_SUCCESS;
}

DWORD cleanDataBuffer(CM_DATA_BUFFER** buffer)
{
	/*strcpy((char*)(*buffer)->DataBuffer, "");
	(*buffer)->UsedBufferSize = 0;*/

	CopyDataIntoBuffer(*buffer, (CM_BYTE*)"", sizeof(""));

	return ERROR_SUCCESS;
}

TCHAR** splitMessage(TCHAR* message, DWORD* numberOfWords)
{
	TCHAR** result = (TCHAR**)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR*));
	for (int i = 0; i < MAX_MESSAGE_SIZE; i++)
	{
		result[i] = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	}

	*numberOfWords = 0;

	TCHAR* word = _tcstok(message, TEXT(" "));
	while (word != NULL)
	{
		_tcscpy(result[*numberOfWords], word);
		(*numberOfWords)++;
		word = _tcstok(NULL, TEXT(" "));
	}

	return result;
}

DWORD initClientsArray()
{
	for (int i = 0; i < 10; i++)
	{
		clients[i].clientHandle = NULL;
		clients[i].clientName = (TCHAR*)malloc(50 * sizeof(TCHAR));
		_tcscpy(clients[i].clientName, TEXT(""));
		clients[i].isConnected = FALSE;
	}

	return ERROR_SUCCESS;
}

DWORD addClient(CLIENT_DETAIL clientDetail)
{
	for (int i = 0; i < 10; i++)
	{
		if (clients[i].clientHandle == NULL)
		{
			clients[i] = clientDetail;
			return ERROR_SUCCESS;
		}
	}
	return ERROR_ERRORS_ENCOUNTERED;
}

void getClient(TCHAR* name, CLIENT_DETAIL* client)
{
	for (int i = 0; i < 10; i++)
	{
		if (_tcscmp(clients[i].clientName, name) == 0)
		{
			*client = clients[i];
			return;
		}
	}
}

DWORD removeClient(TCHAR* name)
{
	for (int i = 0; i < 10; i++)
	{
		if (_tcscmp(clients[i].clientName, name) == 0)
		{
			for (int j = i; j < 10-1; j++)
			{
				clients[j] = clients[j + 1];
			}
			return ERROR_SUCCESS;
		}
	}
	return ERROR_ERRORS_ENCOUNTERED;
}

BOOL isConnected(TCHAR* name)
{
	for (int i = 0; i < 10; i++)
	{
		if (_tcscmp(clients[i].clientName, name) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

int ClientWorker(PVOID clientParam)
{
	connectedClients++;

	CM_SERVER_CLIENT* newClient = (CM_SERVER_CLIENT*)clientParam;

	CM_DATA_BUFFER* dataToReceive = NULL;
	CM_DATA_BUFFER* dataToSend = NULL;

	TCHAR* message = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));

	while (TRUE)
	{
		if (initDataBuffer(&dataToReceive, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
		{
			free(message);
			return -1;
		}

		if (receiveData(newClient, dataToReceive, message) != ERROR_SUCCESS)
		{
			free(message);
			return -1;
		}
		DestroyDataBuffer(dataToReceive);

		DWORD numberOfWords = 0;
		TCHAR** messageArray = splitMessage(message,&numberOfWords);

		if (_tcscmp(messageArray[0], TEXT("exit")) == 0)
		{
			free(message);
			for (int i = 0; i < MAX_MESSAGE_SIZE; i++)
			{
				free(messageArray[i]);
			}
			free(messageArray);

			if (initDataBuffer(&dataToSend, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
			{
				free(message);
				return -1;
			}

			sendData(newClient, dataToSend, TEXT("exit"));
			DestroyDataBuffer(dataToSend);

			_tprintf_s(TEXT("Client disconnecting...\n"));
			Sleep(3000);
			AbandonClient(newClient);
			connectedClients--;
			_tprintf_s(TEXT("Client disconnected!\n"));
			return 0;
		}

		else if(_tcscmp(messageArray[0],TEXT("echo")) == 0)
		{
			_tprintf_s(TEXT("Message from client: "));
			for (int i = 1; i < (int)numberOfWords; i++)
			{
				_tprintf_s(TEXT("%s "), messageArray[i]);
			}
			_tprintf_s(TEXT("\n"));
		}

		else if (_tcscmp(messageArray[0], TEXT("msg")) == 0)
		{
			CLIENT_DETAIL destinationClientDetail;
			if (!isConnected(messageArray[1]))
			{
				_tprintf_s(TEXT("Destination client not connected!\n"));
				continue;
			}
			getClient(messageArray[1],&destinationClientDetail);
			CM_SERVER_CLIENT* destinationClient = destinationClientDetail.clientHandle;

			TCHAR* messageToSend = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
			_tcscpy(messageToSend, TEXT("msg"));
			for (int i = 2; i < (int)numberOfWords; i++)
			{
				_tcscat(messageToSend, TEXT(" "));
				_tcscat(messageToSend, messageArray[i]);
			}

			if (initDataBuffer(&dataToSend, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
			{
				free(message);
				return -1;
			}

			sendData(destinationClient, dataToSend, messageToSend);
			DestroyDataBuffer(dataToSend);
			free(messageToSend);
		}

		else if (_tcscmp(messageArray[0], TEXT("login")) == 0)
		{
			CLIENT_DETAIL clientDetail;
			clientDetail.clientHandle = newClient;
			clientDetail.isConnected = TRUE;
			clientDetail.clientName = messageArray[1];

			if (initDataBuffer(&dataToSend, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
			{
				free(message);
				return -1;
			}

			DWORD result = (checkIfUserExists(messageArray[1]));
			if (result == ERROR_ERRORS_ENCOUNTERED)
			{
				sendData(newClient, dataToSend, TEXT("login failed"));
				DestroyDataBuffer(dataToSend);
				continue;
			}
			else if (result == ERROR_NOT_FOUND)
			{
				sendData(newClient, dataToSend, TEXT("login notFound"));
				DestroyDataBuffer(dataToSend);
				continue;
			}

			if (addClient(clientDetail) != ERROR_SUCCESS)
			{
				_tprintf_s(TEXT("Failed to add client to client array!\n"));
				sendData(newClient, dataToSend, TEXT("login failed"));
				DestroyDataBuffer(dataToSend);
				continue;
			}

			sendData(newClient, dataToSend, TEXT("login success"));
			_tcscpy(clientName, messageArray[1]);
			DestroyDataBuffer(dataToSend);
		}

		else if (_tcscmp(messageArray[0], TEXT("register")) == 0)
		{
			if (initDataBuffer(&dataToSend, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
			{
				free(message);
				return -1;
			}

			if (addUserToFile(messageArray[1]) != ERROR_SUCCESS)
			{
				sendData(newClient, dataToSend, TEXT("register failed"));
				DestroyDataBuffer(dataToSend);
				continue;
			}

			sendData(newClient, dataToSend, TEXT("register success"));
			DestroyDataBuffer(dataToSend);
		}

		else if (_tcscmp(messageArray[0], TEXT("client")) == 0)
		{
			if (initDataBuffer(&dataToSend, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
			{
				free(message);
				return -1;
			}

			TCHAR* messageToSend = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
			_tcscpy(messageToSend, TEXT("client "));
			_tcscat(messageToSend, clientName);

			sendData(newClient, dataToSend, messageToSend);
			DestroyDataBuffer(dataToSend);
			free(messageToSend);
		}

		else if (_tcscmp(messageArray[0], TEXT("logout")) == 0)
		{
			if (initDataBuffer(&dataToSend, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
			{
				free(message);
				return -1;
			}

			if (removeClient(clientName) != ERROR_SUCCESS)
			{
				sendData(newClient, dataToSend, TEXT("logout failed"));
				DestroyDataBuffer(dataToSend);
				continue;
			}
			_tcscpy(clientName, TEXT("UNNAMED"));

			sendData(newClient, dataToSend, TEXT("logout success"));
			DestroyDataBuffer(dataToSend);
		}

		else
		{
			_tprintf_s(TEXT("Unknown command received!\n"));
		}
	}
}

DWORD WINAPI ServerListener(PVOID param)
{
	CM_SERVER* server = (CM_SERVER*)param;

	TCHAR* command = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	while (TRUE)
	{
		_getts_s(command, MAX_MESSAGE_SIZE);
		if (_tcscmp(command, TEXT("exit")) == 0)
		{
			if (connectedClients != 0)
			{
				_tprintf_s(TEXT("Cannot close server! There are connected clients!\n"));
			}
			else
			{
				_tprintf_s(TEXT("Server is shutting down now...\n"));

				free(clients);
				free(command);
				DestroyServer(server);
				UninitCommunicationModule();
				ThreadPoolDestroy(&threadPool);
				_tprintf_s(TEXT("Server shut down\n"));
				system("pause");
				exit(0);
			}
		}
		else
		{
			_tprintf_s(TEXT("Invalid command!\n"));
		}
	}
}

int _tmain(int argc, TCHAR* argv[])
{
	(void)argc;
	(void)argv;

	//TODO
	//Modify the number of clients transmited to the thread pool.
	ThreadPoolInit(&threadPool, 2);

	EnableCommunicationModuleLogger();

	CM_ERROR error = InitCommunicationModule();
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("InitCommunicationModule failed with err-code=0x%X!\n"), error);
		return -1;
	}

	CM_SERVER* server = NULL;
	error = CreateServer(&server);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("CreateServer failed with err-code=0x%X!\n"), error);
		UninitCommunicationModule();
		return -1;
	}

	//Create listener thread
	HANDLE hListener = CreateThread(NULL, 0, ServerListener, (PVOID)server, 0, NULL);

	_tprintf_s(TEXT("Server is Up & Running...\n"));

	clients = (CLIENT_DETAIL*)malloc(10 * sizeof(CLIENT_DETAIL));
	initClientsArray();
	clientName = (TCHAR*)malloc(50 * sizeof(TCHAR));
	_tcscpy(clientName, TEXT("UNNAMED"));

	while (TRUE)
	{
		CM_SERVER_CLIENT* newClient = NULL;
		//Must find a way to stop this once ready to close server
		error = AwaitNewClient(server, &newClient);
		if (CM_IS_ERROR(error))
		{
			_tprintf_s(TEXT("AwaitNewClient failed with err-code=0x%X!\n"), error);
			DestroyServer(server);
			UninitCommunicationModule();
			return -1;
		}

		ThreadPoolStartNewWorker(&threadPool, ClientWorker, (PVOID)newClient);
	}

	WaitForSingleObject(hListener, INFINITE);
	_tprintf_s(TEXT("Server is shutting down now...\n"));

	DestroyServer(server);
	UninitCommunicationModule();
	ThreadPoolDestroy(&threadPool);

	return 0;
}