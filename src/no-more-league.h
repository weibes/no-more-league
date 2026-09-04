#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <shlobj.h>
#include <psapi.h>
#include <shellapi.h>
#include <tchar.h>
#include <iostream>
#include <string>

#ifndef UNICODE
typedef std::string String;
#else
typedef std::wstring String;
#endif

// Application constants
static const TCHAR APP_NAME[] = TEXT("NoMoreLeague");
static const TCHAR WEBSITE[] = TEXT("https://leetcode.com");

// Registry constants for kill switch
static const TCHAR REG_APP_KEY[] = TEXT("Software\\NoMoreLeague");
static const TCHAR REG_KILL_SWITCH_VAL[] = TEXT("KillSwitch");

// Registry constants for startup Run keys
static const TCHAR REG_RUN_KEY[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run");

// Mutex name to ensure single instance
static const TCHAR MUTEX_NAME[] = TEXT("Local\\NoMoreLeagueSingleInstanceMutex");

// Known League of Legends process names to detect and terminate
static const TCHAR* const TARGET_PROCESSES[] = {
    TEXT("LeagueClient.exe"),
    TEXT("LeagueClientUx.exe"),
    TEXT("LeagueClientUxRender.exe"),
    TEXT("League of Legends.exe"),
    TEXT("LolClient.exe"),
    TEXT("LeagueCrashHandler.exe"),
    TEXT("RiotClientServices.exe")
};
static const size_t TARGET_PROCESSES_COUNT = sizeof(TARGET_PROCESSES) / sizeof(TARGET_PROCESSES[0]);

// Function prototypes
bool isSystemWindows();
bool getExecutablePath(TCHAR* pathBuffer, DWORD bufferSize);

// Kill switch management
bool isKillSwitchActive();
bool setKillSwitch(bool active);

// Multi-layered persistence mechanisms
bool setRegistryRunKey(HKEY hRootKey, bool enable);
bool copyToStartupFolder(bool allUsers);
bool removeStartupFolderFile(bool allUsers);
bool setScheduledTask(bool enable);
void createPersistence();
void removePersistence();

// Monitoring and process killing
bool isLeagueProcess(const TCHAR* processName);
bool findAndKill(DWORD processID);
void scanAndKillLeague();
void createPopup();
void printUsage(const TCHAR* exeName);