#include "no-more-league.h"

// Checking to ensure that os is properly windows,
int systemCheck() {
  std::string environmentName(getenv("windir"));
  std::cout << environmentName; // returns "C:/WINDOWS"
  return environmentName.erase(0, 3) !=
         "WINDOWS"; // returns 0 if it DOES equal windows
}

void createPersistence() {
  // copy into startup folder
  DWORD pid = GetCurrentProcessId();
  HANDLE hProcess =
      OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
  TCHAR currPath[MAX_PATH];
  HMODULE hMod;
  DWORD cbNeeded;
  EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded);
  GetModuleFileName(hMod, currPath, pid);
  std::cout << currPath << std::endl;
  LPCTSTR newPath = STARTUP_LOCATION;
  if (!CopyFile(currPath, newPath, false)) {
    std::cout << "file copy failed. Copying to user startup folder..."
              << std::endl;
    std::cout << "error data:" << GetLastError() << std::endl;
  }; // todo add user copy here
  // add to registry TODO
}
void createPopup() {
  LPCTSTR messageText = "No league for u.\nGo do your leetcode.";
  LPCTSTR captionText = "NoLeague.exe";
  UINT type = MB_OK;
  int retVal = MessageBox(NULL, messageText, captionText, type);
}

bool findAndKill(DWORD processID) {
  bool found = false;
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
  DWORD cmpFlags = NORM_IGNOREWIDTH;
  if (strcmp(GAME_CLIENT_NAME, szProcessName) || strcmp(GAME_NAME, szProcessName)) {
    // main logic for termination and launching leetcode nonsense
    found = true;
    std::cout << "Found league." << std::endl;
    if (!TerminateProcess(hProcess, processID)) {
      std::cout << "termination failed." << std::endl;
      std::cout << "error: " << GetLastError() << std::endl;
    };
    createPopup();
    ShellExecute(0, 0, WEBSITE, 0, 0, SW_SHOW);
  }

  // Release the handle to the process.
  CloseHandle(hProcess);
  return found;
}

int main() {
  if (!systemCheck) {
    std::cout << "System running is not windows. This is built only for windows "
            "machines."
         << std::endl;
    return 1;
  }

  createPersistence();
  while (true) // loop for testing
  {
    std::cout << "running check.." << std::endl;
    DWORD processes[1024], cbNeeded, cProcesses;
    if (!EnumProcesses(processes, sizeof(processes), &cbNeeded)) {
      return 1;
    }

    cProcesses = cbNeeded / sizeof(DWORD);
    unsigned int i;
    // get processes
    for (i = 0; i < cProcesses; i++) {
      if (processes[i] != 0) {
        bool isFound = findAndKill(processes[i]);
        if (isFound) {
          break; // bad practice i know but yeno
        }
      }
    }
    // sleep function, check every... 30 seconds? Could maybe do 1 minute.
    Sleep(30000);
  }
  return 0;
}
