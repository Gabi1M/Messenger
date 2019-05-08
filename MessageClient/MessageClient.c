#include "stdafx.h"

// communication library
#include "communication_api.h"
#include <Windows.h>
#include <crtdbg.h>

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
		_tprintf_s(TEXT("ReceiveDataFromServer failed with err-code=0x%X!\n"), error);
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

void freeArray(TCHAR*** messageArray)
{
	if ((*messageArray) != NULL)
	{
		for (int i = 0; i < MAX_MESSAGE_SIZE; i++)
		{
			if ((*messageArray)[i] != NULL)
			{
				free((*messageArray)[i]);
				(*messageArray)[i] = NULL;
			}
		}
		free(*messageArray);
		*messageArray = NULL;
	}
}

void smallBufferhandler(const TCHAR* expression, const TCHAR* function, const TCHAR* file, unsigned int line, uintptr_t pReserved)
{
	UNREFERENCED_PARAMETER(expression);
	UNREFERENCED_PARAMETER(function);
	UNREFERENCED_PARAMETER(file);
	UNREFERENCED_PARAMETER(line);
	UNREFERENCED_PARAMETER(pReserved);
	_tprintf_s(TEXT("Command too long!\nMaximum characters: 255\n"));
	//abort();
}

DWORD WINAPI ClientListener(PVOID param)
{
	CM_CLIENT* client = (CM_CLIENT*)param;

	CM_DATA_BUFFER* dataToReceive = NULL;

	TCHAR** messageArray = NULL;

	TCHAR* message = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	if (message == NULL)
	{
		goto cleanup;
	}

	while (TRUE)
	{

		if (initDataBuffer(&dataToReceive, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
		{
			goto cleanup;
		}

		if (receiveData(client, dataToReceive, message) != ERROR_SUCCESS)
		{
			goto cleanup;
		}
		DestroyDataBuffer(dataToReceive);
		dataToReceive = NULL;

		DWORD numberOfWords = 0;
		messageArray = splitMessage(message,&numberOfWords);
		if (messageArray == NULL)
		{
			goto cleanup;
		}

		if (_tcscmp(messageArray[0], TEXT("exit")) == 0)
		{
			_tprintf_s(TEXT("Client shutting down...\n"));
			goto cleanup;
		}

		else if(_tcscmp(messageArray[0],TEXT("msg")) == 0)
		{
			TCHAR* msg = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
			if (msg == NULL)
			{
				goto cleanup;
			}

			_tcscpy(msg, concatMessage(messageArray,numberOfWords,2));
			_tprintf_s(TEXT("Message received from %s: %s\n"),messageArray[1], msg);

			free(msg);
			msg = NULL;

			continue;
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
			continue;
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
			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("list")) == 0)
		{
			TCHAR* concat = concatMessage(messageArray, numberOfWords, 1);
			if (concat == NULL)
			{
				goto cleanup;
			}
			_tprintf_s(TEXT("Connected clients: %s\n"), concat);
			free(concat);

			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("client")) == 0)
		{
			_tprintf_s(TEXT("Connected client on server: %s\n"), messageArray[1]);
			continue;
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
			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("history")) == 0)
		{
			if (_tcscmp(messageArray[1], TEXT("noMessages")) == 0)
			{
				_tprintf_s(TEXT("No messages in history!\n"));
			}
			else
			{
				_tprintf_s(TEXT("H: %s\n"), concatMessage(messageArray, numberOfWords, 1));
			}
			continue;
		}

		else
		{
			_tprintf_s(TEXT("Response from server received but it's unknown command!\n"));
			continue;
		}

	cleanup:
		Sleep(1500);
		if (dataToReceive != NULL)
		{
			DestroyDataBuffer(dataToReceive);
			dataToReceive = NULL;
		}

		if (message != NULL)
		{
			free(message);
			message = NULL;
		}

		freeArray(&messageArray);

		if (client != NULL)
		{
			DestroyClient(client);
			client = NULL;
			UninitCommunicationModule();
		}

		return 0;
	}
}

int _tmain(int argc, TCHAR* argv[])
{
	(void)argc;
	(void)argv;

	EnableCommunicationModuleLogger();

	CM_CLIENT* client = NULL;
	CM_DATA_BUFFER* dataToSend = NULL;
	TCHAR** messageArray = NULL;
	CM_ERROR error;

	TCHAR* message = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	if (message == NULL)
	{
		goto cleanup;
	}

	error = InitCommunicationModule();
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("InitCommunicationModule failed with err-code=0x%X!\n"), error);
		goto cleanup;
	}

	error = CreateClientConnectionToServer(&client);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("CreateClientConnectionToServer failed with err-code=0x%X!\n"), error);
		goto cleanup;
	}

	_tprintf_s(TEXT("We are connected to the server...\n"));
	_tprintf_s(TEXT("Use \"help\" to see all available commands.\n"));

	//Create listener thread
	HANDLE hListener = CreateThread(NULL, 0, ClientListener, (PVOID)client, 0, NULL);

	clientName = (TCHAR*)malloc(50 * sizeof(TCHAR));
	if (clientName == NULL)
	{
		goto cleanup;
	}
	_tcscpy(clientName, TEXT("UNNAMED"));
	_tprintf_s(TEXT("Connected client: %s\n"), clientName);

	_invalid_parameter_handler oldHandler, newHandler;
	newHandler = smallBufferhandler;
	oldHandler = _set_invalid_parameter_handler(newHandler);
	_CrtSetReportMode(_CRT_ASSERT, 0);

	while (TRUE)
	{
		Sleep(500);
		_tprintf_s(TEXT("Enter command: "));
		_getts_s(message, MAX_MESSAGE_SIZE);

		if (_tcscmp(message, TEXT("")) == 0)
		{
			continue;
		}

		DWORD numberOfWords = 0;

		freeArray(&messageArray);
		messageArray = splitMessage(message,&numberOfWords);
		if (messageArray == NULL)
		{
			goto cleanup;
		}

		if (initDataBuffer(&dataToSend, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
		{
			goto cleanup;
		}

		if (_tcscmp(messageArray[0], TEXT("exit")) == 0)
		{
			if (numberOfWords != 1)
			{
				_tprintf_s(TEXT("Too few arguments!\n"));
				continue;
			}

			if (sendData(client, dataToSend, message) != ERROR_SUCCESS)
			{
				goto cleanup;
			}

			WaitForSingleObject(hListener, INFINITE);
			CloseHandle(hListener);
			goto cleanup;
		}

		else if (_tcscmp(messageArray[0], TEXT("echo")) == 0)
		{
			if (numberOfWords < 2)
			{
				_tprintf_s(TEXT("Too few arguments!\n"));
				continue;
			}

			_tcscpy(message, concatMessage(messageArray, numberOfWords, 0));
			if (sendData(client, dataToSend, message) != ERROR_SUCCESS)
			{
				goto cleanup;
			}

			DestroyDataBuffer(dataToSend);
			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("msg")) == 0)
		{
			if (numberOfWords < 3)
			{
				_tprintf_s(TEXT("Too few arguments!\n"));
				continue;
			}

			if (_tcscmp(clientName, TEXT("UNNAMED")) == 0)
			{
				_tprintf_s(TEXT("No user logged in!\n"));
				continue;
			}

			_tcscpy(message, concatMessage(messageArray, numberOfWords, 0));
			if (sendData(client, dataToSend, message) != ERROR_SUCCESS)
			{
				goto cleanup;
			}

			DestroyDataBuffer(dataToSend);
			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("login")) == 0)
		{
			if (numberOfWords < 2)
			{
				_tprintf_s(TEXT("Too few arguments!\n"));
				continue;
			}

			if (_tcscmp(clientName, TEXT("UNNAMED")) != 0)
			{
				_tprintf_s(TEXT("User already connected: %s\n"), clientName);
				continue;
			}

			_tcscat(message, TEXT(" "));
			_tcscat(message, messageArray[1]);
			_tcscpy(clientName, messageArray[1]);

			if (sendData(client, dataToSend, message) != ERROR_SUCCESS)
			{
				goto cleanup;
			}

			DestroyDataBuffer(dataToSend);
			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("register")) == 0)
		{
			if (numberOfWords < 2)
			{
				_tprintf_s(TEXT("Too few arguments!\n"));
				continue;
			}

			if (_tcscmp(clientName, TEXT("UNNAMED")) != 0)
			{
				_tprintf_s(TEXT("User already connected: %s\n"), clientName);
				continue;
			}

			_tcscat(message, TEXT(" "));
			_tcscat(message, messageArray[1]);

			if (sendData(client, dataToSend, message) != ERROR_SUCCESS)
			{
				goto cleanup;
			}

			DestroyDataBuffer(dataToSend);
			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("client")) == 0)
		{
			if (numberOfWords != 1)
			{
				_tprintf_s(TEXT("Too few arguments!\n"));
				continue;
			}

			if (_tcscmp(clientName, TEXT("UNNAMED")) == 0)
			{
				_tprintf_s(TEXT("No user logged in!\n"));
				continue;
			}

			_tprintf_s(TEXT("Connected client: %s\n"), clientName);
			if (sendData(client, dataToSend, TEXT("client")))
			{
				goto cleanup;
			}

			DestroyDataBuffer(dataToSend);
			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("list")) == 0)
		{
			if (numberOfWords != 1)
			{
				_tprintf_s(TEXT("Too few arguments!\n"));
				continue;
			}

			if (_tcscmp(clientName, TEXT("UNNAMED")) == 0)
			{
				_tprintf_s(TEXT("No user logged in!\n"));
				continue;
			}

			if (sendData(client, dataToSend, TEXT("list")))
			{
				goto cleanup;
			}

			DestroyDataBuffer(dataToSend);
			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("logout")) == 0)
		{
			if (numberOfWords != 1)
			{
				_tprintf_s(TEXT("Too few arguments!\n"));
				continue;
			}

			if (_tcscmp(clientName, TEXT("UNNAMED")) == 0)
			{
				_tprintf_s(TEXT("No user logged in!\n"));
				continue;
			}

			if (sendData(client, dataToSend, TEXT("logout")) != ERROR_SUCCESS)
			{
				goto cleanup;
			}

			DestroyDataBuffer(dataToSend);
			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("history")) == 0)
		{
			if (numberOfWords < 2)
			{
				_tprintf_s(TEXT("Too few arguments!\n"));
				continue;
			}

			if (_tcscmp(clientName, TEXT("UNNAMED")) == 0)
			{
				_tprintf_s(TEXT("No user logged in!\n"));
				continue;
			}

			_tcscat(message, TEXT(" "));
			_tcscat(message, messageArray[1]);

			if (sendData(client, dataToSend, message) != ERROR_SUCCESS)
			{
				goto cleanup;
			}

			DestroyDataBuffer(dataToSend);
			continue;
		}

		else if (_tcscmp(messageArray[0], TEXT("help")) == 0)
		{
			_tprintf_s(TEXT("---HELP---\n"));
			_tprintf_s(TEXT("1. echo [msg] -> transmit a message to the server.\n"));
			_tprintf_s(TEXT("2. register [username] -> registers the user with the given username to allow it to fully use the app.\n"));
			_tprintf_s(TEXT("3. login [username] -> logs in the user with the given username. If user not registered, operation fails.\n"));
			_tprintf_s(TEXT("4. logout -> logs out the current user. If current user is not logged in, operation fails.\n"));
			_tprintf_s(TEXT("5. msg [username] [msg] -> sends the message to the user with the given username. If the current user is not logged in or the destination user is not registered, the operation fails.\n"));
			_tprintf_s(TEXT("6. client -> prints current connected user.\n"));
			_tprintf_s(TEXT("7. list -> prints all current connected users.\n"));
			_tprintf_s(TEXT("8. history [username] -> prints all messages sent to the user with the given username.\n"));
			_tprintf_s(TEXT("9. help -> display this help.\n"));
			_tprintf_s(TEXT("10. exit -> logs out the current user if connected and shuts down the client.\n"));

			continue;
		}

		else
		{
			_tprintf_s(TEXT("Unknown command! \n"));
			DestroyDataBuffer(dataToSend);
			continue;
		}

	cleanup:
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

		if (message != NULL)
		{
			free(message);
			message = NULL;
		}

		freeArray(&messageArray);

		break;
	}

	_tprintf_s(TEXT("Client shut down!\n"));
	_CrtDumpMemoryLeaks();
	return 0;
}

