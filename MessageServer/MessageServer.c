#include "stdafx.h"

// communication library
#include "communication_api.h"
#include "ThreadPool.h"
#include <Windows.h>

#define MAX_MESSAGE_SIZE 255

THREAD_POOL threadPool;

DWORD connectedClients = 0;

DWORD sendData(CM_SERVER_CLIENT* destinationClient, CM_DATA_BUFFER* dataToSend, TCHAR* messageToSend, TCHAR* clientName)
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

	_tcscpy(clientName, clientName);

	return ERROR_SUCCESS;
}

DWORD receiveData(CM_SERVER_CLIENT* sourceClient, CM_DATA_BUFFER* dataToReceive, TCHAR* bufferedMessage, TCHAR* clientName)
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

	_tcscpy(clientName, clientName);
	//_tcscpy(bufferedMessage, (TCHAR*)(dataToReceive->DataBuffer));
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

TCHAR** splitMessage(TCHAR* message, int* numberOfWords)
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

int ClientWorker(PVOID clientParam)
{
	connectedClients++;

	CM_SERVER_CLIENT* newClient = (CM_SERVER_CLIENT*)clientParam;

	CM_DATA_BUFFER* dataToReceive = NULL;

	TCHAR* message = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	TCHAR* clientName = (TCHAR*)malloc(30 * sizeof(TCHAR));

	while (TRUE)
	{
		if (initDataBuffer(&dataToReceive, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
		{
			free(message);
			free(clientName);
			return -1;
		}

		if (receiveData(newClient, dataToReceive, message, clientName) != ERROR_SUCCESS)
		{
			free(message);
			free(clientName);
			return -1;
		}

		DestroyDataBuffer(dataToReceive);

		int numberOfWords = 0;
		TCHAR** messageArray = splitMessage(message,&numberOfWords);

		if (_tcscmp(messageArray[0], TEXT("exit")) == 0)
		{
			free(message);
			free(clientName);
			for (int i = 0; i < MAX_MESSAGE_SIZE; i++)
			{
				free(messageArray[i]);
			}
			free(messageArray);
			AbandonClient(newClient);
			connectedClients--;
			_tprintf_s(TEXT("Client disconnected!\n"));
			return 0;
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

				free(command);
				DestroyServer(server);
				UninitCommunicationModule();
				ThreadPoolDestroy(&threadPool);
				_tprintf_s(TEXT("Server shut down\n"));
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

	CreateThread(NULL, 0, ServerListener, (PVOID)server, 0, NULL);

	_tprintf_s(TEXT("Server is Up & Running...\n"));

	while (TRUE)
	{
		CM_SERVER_CLIENT* newClient = NULL;
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

	_tprintf_s(TEXT("Server is shutting down now...\n"));

	DestroyServer(server);
	UninitCommunicationModule();
	ThreadPoolDestroy(&threadPool);

	return 0;
}