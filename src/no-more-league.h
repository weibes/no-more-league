#pragma once
#include <Windows.h>
#include <iostream>
#include <lmcons.h>
#include <psapi.h>
#include <shellapi.h>
#include <string>
#include <tchar.h>

#ifndef UNICODE
typedef std::string String;
#else
typedef std::wstring String;
#endif

LPCTSTR STARTUP_LOCATION =
    "C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\StartUp";
LPCTSTR WEBSITE = "https://leetcode.com";

TCHAR GAME_CLIENT_NAME[17] = TEXT("LeagueClient.exe");
TCHAR GAME_NAME[22] = TEXT("League of Legends.exe");

int systemCheck();
bool findAndKill(DWORD processID);
void createPopup();