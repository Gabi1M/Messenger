#pragma once

#include <Windows.h>

DWORD addUserToFile(TCHAR* username);

DWORD checkIfUserExists(TCHAR* username);

DWORD addMessageToFile(TCHAR* originName, TCHAR* destinationName, TCHAR* message);

TCHAR** getMessagesFromFile(TCHAR* name, DWORD desiredMessageCount, DWORD *actualMessageCount);