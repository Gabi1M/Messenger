#include "stdafx.h"

// communication library
#include "communication_api.h"
#include <Windows.h>

#define MAX_MESSAGE_SIZE 255

TCHAR* clientName;

DWORD sendData(CM_CLIENT* client, CM_DATA_BUFFER* dataToSend, TCHAR* messageToSend)
{
	SIZE_T messageToSendSize = _tcslen(messageToSend);
	CM_SIZE sendByteCount;
	CM_ERROR error;

	char* aux = malloc(MAX_MESSAGE_SIZE);
	wcstombs(aux, messageToSend, MAX_MESSAGE_SIZE);

	error = CopyDataIntoBuffer(dataToSend, (const CM_BYTE*)aux, (CM_SIZE)messageToSendSize);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("CopyDataIntoBuffer failed with error code: 0x%X\n"), GetLastError());
		return ERROR_ERRORS_ENCOUNTERED;
	}

	error = SendDataToServer(client, dataToSend, &sendByteCount);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("SendDataToServer failed with error code: 0x%X\n"), GetLastError());
		return ERROR_ERRORS_ENCOUNTERED;
	}

	return ERROR_SUCCESS;
}

DWORD receiveData(CM_CLIENT* client, CM_DATA_BUFFER* dataToReceive, TCHAR* bufferedMessage)
{
	CM_SIZE receivedByteCount;
	CM_ERROR error;
	char* aux = malloc(MAX_MESSAGE_SIZE);

	error = ReceiveDataFormServer(client, dataToReceive, &receivedByteCount);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("ReceiveDataFromClient failed with err-code=0x%X!\n"), error);
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
	TCHAR** result = (TCHAR * *)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR*));
	for (int i = 0; i < MAX_MESSAGE_SIZE; i++)
	{
		result[i] = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	}

	*numberOfWords = 0;

	//int length = _tcslen(message);
	TCHAR* word = _tcstok(message, TEXT(" "));
	while (word != NULL)
	{
		_tcscpy(result[*numberOfWords], word);
		(*numberOfWords)++;
		word = _tcstok(NULL, TEXT(" "));
	}

	return result;
}

DWORD cleanDataBuffer(CM_DATA_BUFFER** buffer)
{
	/*strcpy((char*)(*buffer)->DataBuffer, "");
	(*buffer)->UsedBufferSize = 0;*/

	CopyDataIntoBuffer(*buffer, (CM_BYTE*)"", sizeof(""));

	return ERROR_SUCCESS;
}

DWORD WINAPI ClientListener(PVOID param)
{
	CM_CLIENT* client = (CM_CLIENT*)param;

	TCHAR* message = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));

	CM_DATA_BUFFER* dataToReceive = NULL;

	while (TRUE)
	{

		if (initDataBuffer(&dataToReceive, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
		{
			return (DWORD)-1;
		}

		if (receiveData(client, dataToReceive, message) != ERROR_SUCCESS)
		{
			DestroyDataBuffer(dataToReceive);
			return (DWORD)-1;
		}
		DestroyDataBuffer(dataToReceive);

		DWORD numberOfWords = 0;
		TCHAR** messageArray = splitMessage(message,&numberOfWords);

		if (_tcscmp(messageArray[0], TEXT("exit")) == 0)
		{
			DestroyClient(client);
			UninitCommunicationModule();
			free(clientName);
			free(message);

			_tprintf_s(TEXT("Client shut down!\n"));
			system("pause");
		}

		else if(_tcscmp(messageArray[0],TEXT("msg")) == 0)
		{
			TCHAR* msg = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
			_tcscpy(msg, TEXT(""));
			for (int i = 1; i < (int)numberOfWords; i++)
			{
				_tcscat(msg, messageArray[i]);
				_tcscat(msg, TEXT(" "));
			}
			_tprintf_s(TEXT("Message received: %s\n"), msg);
			free(msg);
		}

		else if (_tcscmp(messageArray[0], TEXT("login")) == 0)
		{
			if (_tcscmp(messageArray[1], TEXT("success")) == 0)
			{
				_tprintf_s(TEXT("Login successful!\n"));
			}
			else if(_tcscmp(messageArray[1],TEXT("failed")) == 0)
			{
				_tprintf_s(TEXT("Login failed! Some errors were encountered!\n"));
				_tcscpy(clientName, TEXT("UNNAMED"));
			}
			else
			{
				_tprintf_s(TEXT("Login failed! User not found!\n"));
				_tcscpy(clientName, TEXT("UNNAMED"));
			}
		}

		else if (_tcscmp(messageArray[0], TEXT("register")) == 0)
		{
			if (_tcscmp(messageArray[1], TEXT("success")) == 0)
			{
				_tprintf_s(TEXT("Register successful!\n"));
			}
			else
			{
				_tprintf_s(TEXT("Register failed! Some errors were encountered!\n"));
			}
		}

		else if (_tcscmp(messageArray[0], TEXT("client")) == 0)
		{
			_tprintf_s(TEXT("Connected client on server: %s\n"), messageArray[1]);
		}

		else if (_tcscmp(messageArray[0], TEXT("logout")) == 0)
		{
			if (_tcscmp(messageArray[1], TEXT("success")) == 0)
			{
				_tcscpy(clientName, TEXT("UNNAMED"));
				_tprintf_s(TEXT("Logout successfull!\n"));
			}
			else
			{
				_tprintf_s(TEXT("Logout failed!\n"));
			}
		}

		else
		{
			_tprintf_s(TEXT("Response from server received but it's unknown command!\n"));
		}

	}
}

int _tmain(int argc, TCHAR* argv[])
{
	(void)argc;
	(void)argv;

	EnableCommunicationModuleLogger();

	CM_ERROR error = InitCommunicationModule();
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("InitCommunicationModule failed with err-code=0x%X!\n"), error);
		return -1;
	}

	CM_CLIENT* client = NULL;
	error = CreateClientConnectionToServer(&client);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("CreateClientConnectionToServer failed with err-code=0x%X!\n"), error);
		UninitCommunicationModule();
		return -1;
	}

	_tprintf_s(TEXT("We are connected to the server...\n"));

	//Create listener thread
	CreateThread(NULL, 0, ClientListener, (PVOID)client, 0, NULL);

	TCHAR* message = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));

	CM_DATA_BUFFER* dataToSend = NULL;

	clientName = (TCHAR*)malloc(50 * sizeof(TCHAR));
	_tcscpy(clientName, TEXT("UNNAMED"));
	_tprintf_s(TEXT("Connected client: %s\n"), clientName);

	while (TRUE)
	{
		Sleep(500);
		_tprintf_s(TEXT("Enter command: "));
		_getts_s(message, MAX_MESSAGE_SIZE);

		DWORD numberOfWords = 0;
		TCHAR** messageArray = splitMessage(message,&numberOfWords);

		if (initDataBuffer(&dataToSend, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
		{
			return -1;
		}

		if (_tcscmp(messageArray[0], TEXT("exit")) == 0)
		{
			if (sendData(client, dataToSend, message) != ERROR_SUCCESS)
			{
				break;
			}
			DestroyDataBuffer(dataToSend);
			dataToSend = NULL;

			free(message);
			for (int i = 0; i < MAX_MESSAGE_SIZE; i++)
			{
				free(messageArray[i]);
			}
			free(messageArray);
			dataToSend = NULL;
			break;
		}

		else if (_tcscmp(messageArray[0], TEXT("echo")) == 0)
		{
			for (int i = 1; i < (int)numberOfWords; i++)
			{
				_tcscat(message, TEXT(" "));
				_tcscat(message, messageArray[i]);
			}
			if (sendData(client, dataToSend, message) != ERROR_SUCCESS)
			{
				break;
			}
		}

		else if (_tcscmp(messageArray[0], TEXT("msg")) == 0)
		{
			for (int i = 1; i < (int)numberOfWords; i++)
			{
				_tcscat(message, TEXT(" "));
				_tcscat(message, messageArray[i]);
			}
			if (sendData(client, dataToSend, message) != ERROR_SUCCESS)
			{
				break;
			}
		}

		else if (_tcscmp(messageArray[0], TEXT("login")) == 0)
		{
			_tcscat(message, TEXT(" "));
			_tcscat(message, messageArray[1]);
			_tcscpy(clientName, messageArray[1]);

			if (sendData(client, dataToSend, message) != ERROR_SUCCESS)
			{
				break;
			}
		}

		else if (_tcscmp(messageArray[0], TEXT("register")) == 0)
		{
			_tcscat(message, TEXT(" "));
			_tcscat(message, messageArray[1]);

			if (sendData(client, dataToSend, message) != ERROR_SUCCESS)
			{
				break;
			}
		}

		else if (_tcscmp(messageArray[0], TEXT("client")) == 0)
		{
			_tprintf_s(TEXT("Connected client: %s\n"), clientName);
			if (sendData(client, dataToSend, TEXT("client")))
			{
				break;
			}
		}

		else if (_tcscmp(messageArray[0], TEXT("logout")) == 0)
		{
			if (sendData(client, dataToSend, TEXT("logout")) != ERROR_SUCCESS)
			{
				break;
			}
		}

		else
		{
			_tprintf_s(TEXT("Unknown command! \n"));
		}
		DestroyDataBuffer(dataToSend);
	}

	_tprintf_s(TEXT("Client is shutting down now...\n"));
	return 0;
}

