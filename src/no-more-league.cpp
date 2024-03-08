// imports
#include "no-more-league.h"
#include <Windows.h>
#include <iostream>
#include <lmcons.h>
#include <psapi.h>
#include <shellapi.h>
#include <string>
#include <tchar.h>

using namespace std;

// Checking to ensure that os is properly windows,
int systemCheck() {
  std::string environmentName(getenv("windir"));
  cout << environmentName; // returns "C:/WINDOWS"
  return environmentName.erase(0, 3) !=
         "WINDOWS"; // returns 0 if it DOES equal windows
}

void createPersistence() {
  DWORD pid = GetCurrentProcessId();
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                FALSE, pid);
  TCHAR currFolder[MAX_PATH];
  HMODULE hMod;
  DWORD cbNeeded;
  EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded);
  GetModuleFileName(hMod, currFolder, pid);
  std::cout << currFolder << std::endl;

}
void createPopup() {
  LPCTSTR messageText = "No league for u.\nGo do your leetcode.";
  LPCTSTR captionText = "NoLeague.exe";
  UINT type = MB_OK;
  int retVal = MessageBox(NULL, messageText, captionText, type);
}

void findAndKill(DWORD processID) {
  TCHAR szProcessName[MAX_PATH] = TEXT("<unkown>");

  // Get a handle to the process.

  HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                FALSE, processID);
  // Get the process name.

  if (NULL != hProcess) {
    HMODULE hMod;
    DWORD cbNeeded;

    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
      GetModuleBaseName(hProcess, hMod, szProcessName,
                        sizeof(szProcessName) / sizeof(TCHAR));
    }
  }

  // check if league, kill if so
  TCHAR clientName[17] = TEXT("LeagueClient.exe");
  TCHAR gameName[22] = TEXT("League of Legends.exe");
  if (szProcessName == clientName || szProcessName == gameName) {
    // main logic for termination and launching leetcode nonsense
    TerminateProcess(hProcess, processID);
    createPopup();
    ShellExecute(0, 0, "https://leetcode.com", 0, 0, SW_SHOW);
  }

  // Release the handle to the process.
  CloseHandle(hProcess);
}

int main() {
  if (!systemCheck) {
    cout << "System running is not windows. This is built only for windows "
            "machines."
         << endl;
    return 1;
  }

  createPersistence();
  while (true) // loop for testing
  {
    DWORD processes[1024], cbNeeded, cProcesses;
    if (!EnumProcesses(processes, sizeof(processes), &cbNeeded)) {
      return 1;
    }

    cProcesses = cbNeeded / sizeof(DWORD);
    unsigned int i;
    // get processes
    for (i = 0; i < cProcesses; i++) {
      if (processes[i] != 0) {
        findAndKill(processes[i]);
      }
    }
    // sleep function, check every... 30 seconds? Could maybe do 1 minute.
    Sleep(30000);
  }
  return 0;
}
