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
CLIENT_DETAIL* clients;
TCHAR* clientName;
DWORD connectedClients = 0;

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

TCHAR* concatMessage(TCHAR** messageArray, DWORD numberOfWords, DWORD startIndex)
{
	TCHAR* result = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	if (result == NULL || messageArray == NULL || numberOfWords == 0)
	{
		return NULL;
	}
	_tcscpy(result, TEXT(""));

	for (int i = (int)startIndex; i < (int)numberOfWords; i++)
	{
		_tcscat(result, messageArray[i]);
		if (i != (int)numberOfWords - 1)
		{
			_tcscat(result, TEXT(" "));
		}
	}

	return result;
}

void freeArray(TCHAR** messageArray)
{
	if (messageArray != NULL)
	{
		for (int i = 0; i < MAX_MESSAGE_SIZE; i++)
		{
			if (messageArray[i] != NULL)
			{
				free(messageArray[i]);
				messageArray[i] = NULL;
			}
		}
		free(messageArray);
		messageArray = NULL;
	}
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
	TCHAR** messageArray = NULL;

	while (TRUE)
	{
		DWORD numberOfWords = 0;

		if (initDataBuffer(&dataToReceive, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
		{
			goto cleanup;
		}

		if (initDataBuffer(&dataToSend, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
		{
			goto cleanup;
		}

		if (receiveData(newClient, dataToReceive, message) != ERROR_SUCCESS)
		{
			goto cleanup;
		}
		DestroyDataBuffer(dataToReceive);
		dataToReceive = NULL;

		free(messageArray);
		messageArray = NULL;
		messageArray = splitMessage(message, &numberOfWords);

		if (_tcscmp(messageArray[0], TEXT("exit")) == 0)
		{
			removeClient(clientName);

			if (sendData(newClient, dataToSend, TEXT("exit")) != ERROR_SUCCESS)
			{
				goto cleanup;
			}

			_tprintf_s(TEXT("Client disconnecting...\n"));
			goto cleanup;
		}

		else if(_tcscmp(messageArray[0],TEXT("echo")) == 0)
		{
			TCHAR* concatedMessage = concatMessage(messageArray, numberOfWords, 1);
			if (concatedMessage == NULL)
			{
				goto cleanup;
			}
			_tprintf_s(TEXT("Message from client: %s\n"),concatedMessage);
			free(concatedMessage);
			concatedMessage = NULL;

			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("msg")) == 0)
		{
			if (!isConnected(messageArray[1]))
			{
				_tprintf_s(TEXT("Destination client not connected!\n"));
				continue;
			}

			CLIENT_DETAIL destinationClientDetail;
			getClient(messageArray[1],&destinationClientDetail);
			CM_SERVER_CLIENT* destinationClient = destinationClientDetail.clientHandle;

			TCHAR* messageToSend = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
			if (messageToSend == NULL)
			{
				goto cleanup;
			}
			_tcscpy(messageToSend, TEXT("msg "));
			_tcscat(messageToSend, clientName);
			_tcscat(messageToSend, TEXT(" "));
			_tcscat(messageToSend, concatMessage(messageArray, numberOfWords, 2));

			if (sendData(destinationClient, dataToSend, messageToSend) != ERROR_SUCCESS)
			{
				free(messageToSend);
				messageToSend = NULL;
				goto cleanup;
			}
			DestroyDataBuffer(dataToSend);
			free(messageToSend);
			messageToSend = NULL;

			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("login")) == 0)
		{
			DWORD result = (checkIfUserExists(messageArray[1]));
			if (result == ERROR_ERRORS_ENCOUNTERED)
			{
				if (sendData(newClient, dataToSend, TEXT("login failed")) != ERROR_SUCCESS)
				{
					goto cleanup;
				}
				DestroyDataBuffer(dataToSend);
				dataToSend = NULL;

				continue;
			}
			else if (result == ERROR_NOT_FOUND)
			{
				if (sendData(newClient, dataToSend, TEXT("login notFound")) != ERROR_SUCCESS)
				{
					goto cleanup;
				}
				DestroyDataBuffer(dataToSend);
				dataToSend = NULL;

				continue;
			}

			CLIENT_DETAIL clientDetail;
			clientDetail.clientHandle = newClient;
			clientDetail.isConnected = TRUE;
			clientDetail.clientName = messageArray[1];

			if (addClient(clientDetail) != ERROR_SUCCESS)
			{
				_tprintf_s(TEXT("Failed to add client to client array!\n"));
				if (sendData(newClient, dataToSend, TEXT("login failed")) != ERROR_SUCCESS)
				{
					goto cleanup;
				}
				DestroyDataBuffer(dataToSend);
				dataToSend = NULL;
				continue;
			}

			if (sendData(newClient, dataToSend, TEXT("login success")) != ERROR_SUCCESS)
			{
				goto cleanup;
			}
			DestroyDataBuffer(dataToSend);
			dataToSend = NULL;

			_tcscpy(clientName, messageArray[1]);

			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("register")) == 0)
		{
			if (addUserToFile(messageArray[1]) != ERROR_SUCCESS)
			{
				if (sendData(newClient, dataToSend, TEXT("register failed")) != ERROR_SUCCESS)
				{
					goto cleanup;
				}
				DestroyDataBuffer(dataToSend);

				continue;
			}

			if (sendData(newClient, dataToSend, TEXT("register success")) != ERROR_SUCCESS)
			{
				goto cleanup;
			}
			DestroyDataBuffer(dataToSend);

			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("client")) == 0)
		{
			TCHAR* messageToSend = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
			if (messageToSend == NULL)
			{
				goto cleanup;
			}
			_tcscpy(messageToSend, TEXT("client "));
			_tcscat(messageToSend, clientName);

			if (sendData(newClient, dataToSend, messageToSend) != ERROR_SUCCESS)
			{
				free(messageToSend);
				messageToSend = NULL;
				goto cleanup;
			}
			DestroyDataBuffer(dataToSend);

			free(messageToSend);
			messageToSend = NULL;

			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("clients")) == 0)
		{
			TCHAR* messageToSend = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
			if (messageToSend == NULL)
			{
				goto cleanup;
			}
			_tcscpy(messageToSend, TEXT("clients "));

			for (int i = 0; i < (int)connectedClients; i++)
			{
				_tcscat(messageToSend, clients[i].clientName);
				if (i != (int)connectedClients - 1)
				{
					_tcscat(messageToSend, TEXT(" "));
				}
			}

			if (sendData(newClient, dataToSend, messageToSend) != ERROR_SUCCESS)
			{
				free(messageToSend);
				messageToSend = NULL;
				goto cleanup;
			}
			DestroyDataBuffer(dataToSend);

			free(messageToSend);
			messageToSend = NULL;

			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("logout")) == 0)
		{
			if (removeClient(clientName) != ERROR_SUCCESS)
			{
				if (sendData(newClient, dataToSend, TEXT("logout failed")) != ERROR_SUCCESS)
				{
					goto cleanup;
				}
				DestroyDataBuffer(dataToSend);

				continue;
			}

			_tcscpy(clientName, TEXT("UNNAMED"));

			if (sendData(newClient, dataToSend, TEXT("logout success")) != ERROR_SUCCESS)
			{
				goto cleanup;
			}
			DestroyDataBuffer(dataToSend);
			
			continue;
		}

		else
		{
			_tprintf_s(TEXT("Unknown command received!\n"));
			continue;
		}

	cleanup:
		Sleep(500);
		if (message != NULL)
		{
			free(message);
			message = NULL;
		}

		freeArray(messageArray);

		if (dataToReceive != NULL)
		{
			DestroyDataBuffer(dataToReceive);
			dataToReceive = NULL;
		}

		if (dataToSend != NULL)
		{
			DestroyDataBuffer(dataToSend);
			dataToSend = NULL;
		}

		if (clientName != NULL)
		{
			free(clientName);
			clientName = NULL;
		}

		if(newClient != NULL)
		{
			AbandonClient(newClient);
			connectedClients--;
			_tprintf_s(TEXT("Client disconnected!\n"));
		}

		return 0;

	}
}

DWORD WINAPI ServerListener(PVOID param)
{
	CM_SERVER* server = (CM_SERVER*)param;

	TCHAR* command = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	if (command == NULL)
	{
		_tprintf_s(TEXT("Failed to allocate memory!\n"));
		exit(0);
	}

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
				clients = NULL;
				free(command);
				command = NULL;
				DestroyServer(server);
				server = NULL;
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