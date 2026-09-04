#include "no-more-league.h"

// Check if running on a Windows system
bool isSystemWindows() {
#ifdef _WIN32
    return true;
#else
    const char* windir = getenv("windir");
    if (!windir) windir = getenv("SystemRoot");
    return (windir != nullptr);
#endif
}

// Retrieve the full absolute path of the current executable
bool getExecutablePath(TCHAR* pathBuffer, DWORD bufferSize) {
    DWORD result = GetModuleFileName(NULL, pathBuffer, bufferSize);
    return (result > 0 && result < bufferSize);
}

// Check if the registry kill switch is currently engaged
bool isKillSwitchActive() {
    HKEY hKey;
    LONG lResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_APP_KEY, 0, KEY_QUERY_VALUE, &hKey);
    if (lResult != ERROR_SUCCESS) {
        return false;
    }

    DWORD dwValue = 0;
    DWORD dwSize = sizeof(dwValue);
    DWORD dwType = REG_DWORD;
    lResult = RegQueryValueEx(hKey, REG_KILL_SWITCH_VAL, NULL, &dwType, (LPBYTE)&dwValue, &dwSize);
    RegCloseKey(hKey);

    return (lResult == ERROR_SUCCESS && dwValue == 1);
}

// Set or clear the registry kill switch
bool setKillSwitch(bool active) {
    HKEY hKey;
    DWORD disposition;
    LONG lResult = RegCreateKeyEx(
        HKEY_CURRENT_USER,
        REG_APP_KEY,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &hKey,
        &disposition
    );

    if (lResult != ERROR_SUCCESS) {
        std::cerr << "Failed to open/create registry key. Error: " << lResult << std::endl;
        return false;
    }

    DWORD dwValue = active ? 1 : 0;
    lResult = RegSetValueEx(
        hKey,
        REG_KILL_SWITCH_VAL,
        0,
        REG_DWORD,
        (const BYTE*)&dwValue,
        sizeof(dwValue)
    );

    RegCloseKey(hKey);
    return (lResult == ERROR_SUCCESS);
}

// Add or remove entry in Windows Registry Run key (HKCU or HKLM)
bool setRegistryRunKey(HKEY hRootKey, bool enable) {
    HKEY hKey;
    LONG lResult = RegOpenKeyEx(hRootKey, REG_RUN_KEY, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, &hKey);
    if (lResult != ERROR_SUCCESS) {
        return false;
    }

    bool success = false;
    if (enable) {
        TCHAR exePath[MAX_PATH];
        if (getExecutablePath(exePath, MAX_PATH)) {
            TCHAR quotedPath[MAX_PATH + 4];
            _sntprintf(quotedPath, sizeof(quotedPath) / sizeof(TCHAR), TEXT("\"%s\""), exePath);
            DWORD dataSize = (DWORD)((_tcslen(quotedPath) + 1) * sizeof(TCHAR));
            lResult = RegSetValueEx(hKey, APP_NAME, 0, REG_SZ, (const BYTE*)quotedPath, dataSize);
            success = (lResult == ERROR_SUCCESS);
        }
    } else {
        lResult = RegDeleteValue(hKey, APP_NAME);
        success = (lResult == ERROR_SUCCESS || lResult == ERROR_FILE_NOT_FOUND);
    }

    RegCloseKey(hKey);
    return success;
}

// Copy executable to User or All-Users Startup folder
bool copyToStartupFolder(bool allUsers) {
    TCHAR currPath[MAX_PATH];
    if (!getExecutablePath(currPath, MAX_PATH)) {
        return false;
    }

    TCHAR startupDir[MAX_PATH];
    int csidl = allUsers ? CSIDL_COMMON_STARTUP : CSIDL_STARTUP;
    HRESULT hr = SHGetFolderPath(NULL, csidl, NULL, 0, startupDir);
    if (FAILED(hr)) {
        if (!allUsers) {
            TCHAR appData[MAX_PATH];
            if (GetEnvironmentVariable(TEXT("APPDATA"), appData, MAX_PATH) > 0) {
                _sntprintf(startupDir, MAX_PATH, TEXT("%s\\Microsoft\\Windows\\Start Menu\\Programs\\Startup"), appData);
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    const TCHAR* fileName = _tcsrchr(currPath, TEXT('\\'));
    if (fileName) {
        fileName++;
    } else {
        fileName = TEXT("no-more-league.exe");
    }

    TCHAR targetPath[MAX_PATH];
    _sntprintf(targetPath, MAX_PATH, TEXT("%s\\%s"), startupDir, fileName);

    // Skip if already running directly from this startup folder
    if (_tcsicmp(currPath, targetPath) == 0) {
        return true;
    }

    BOOL copyResult = CopyFile(currPath, targetPath, FALSE);
    return (copyResult != FALSE);
}

// Delete executable from User or All-Users Startup folder
bool removeStartupFolderFile(bool allUsers) {
    TCHAR currPath[MAX_PATH];
    getExecutablePath(currPath, MAX_PATH);

    TCHAR startupDir[MAX_PATH];
    int csidl = allUsers ? CSIDL_COMMON_STARTUP : CSIDL_STARTUP;
    HRESULT hr = SHGetFolderPath(NULL, csidl, NULL, 0, startupDir);
    if (FAILED(hr)) {
        if (!allUsers) {
            TCHAR appData[MAX_PATH];
            if (GetEnvironmentVariable(TEXT("APPDATA"), appData, MAX_PATH) > 0) {
                _sntprintf(startupDir, MAX_PATH, TEXT("%s\\Microsoft\\Windows\\Start Menu\\Programs\\Startup"), appData);
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    const TCHAR* fileName = _tcsrchr(currPath, TEXT('\\'));
    if (fileName) {
        fileName++;
    } else {
        fileName = TEXT("no-more-league.exe");
    }

    TCHAR targetPath[MAX_PATH];
    _sntprintf(targetPath, MAX_PATH, TEXT("%s\\%s"), startupDir, fileName);

    // Only delete if we are not currently running from that exact file
    if (_tcsicmp(currPath, targetPath) != 0) {
        return (DeleteFile(targetPath) != FALSE || GetLastError() == ERROR_FILE_NOT_FOUND);
    }

    return true;
}

// Register or delete a Windows Task Scheduler task running at user logon
bool setScheduledTask(bool enable) {
    TCHAR currPath[MAX_PATH];
    if (!getExecutablePath(currPath, MAX_PATH)) {
        return false;
    }

    TCHAR cmd[MAX_PATH * 2 + 128];
    if (enable) {
        _sntprintf(cmd, sizeof(cmd) / sizeof(TCHAR),
            TEXT("schtasks /create /tn \"%s\" /tr \"\\\"%s\\\"\" /sc onlogon /f"),
            APP_NAME, currPath);
    } else {
        _sntprintf(cmd, sizeof(cmd) / sizeof(TCHAR),
            TEXT("schtasks /delete /tn \"%s\" /f"),
            APP_NAME);
    }

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 5000);
        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return (exitCode == 0);
    }
    return false;
}

// Install all persistence mechanisms (multi-layered)
void createPersistence() {
    std::cout << "[Persistence] Enforcing startup persistence..." << std::endl;

    // Method 1: Current User Registry Run key (No admin needed)
    if (setRegistryRunKey(HKEY_CURRENT_USER, true)) {
        std::cout << "  [+] HKCU Run key: active" << std::endl;
    } else {
        std::cout << "  [-] HKCU Run key: failed" << std::endl;
    }

    // Method 2: Current User Startup folder (No admin needed)
    if (copyToStartupFolder(false)) {
        std::cout << "  [+] User Startup folder: active" << std::endl;
    } else {
        std::cout << "  [-] User Startup folder: failed (Error " << GetLastError() << ")" << std::endl;
    }

    // Method 3: Windows Task Scheduler (On logon)
    if (setScheduledTask(true)) {
        std::cout << "  [+] Task Scheduler (onlogon): active" << std::endl;
    } else {
        std::cout << "  [-] Task Scheduler: skipped/failed" << std::endl;
    }

    // Method 4: All Users Registry Run key (succeeds if running as admin)
    if (setRegistryRunKey(HKEY_LOCAL_MACHINE, true)) {
        std::cout << "  [+] HKLM Run key: active (admin)" << std::endl;
    }

    // Method 5: All Users Startup folder (succeeds if running as admin)
    if (copyToStartupFolder(true)) {
        std::cout << "  [+] All Users Startup folder: active (admin)" << std::endl;
    }
}

// Remove all persistence mechanisms
void removePersistence() {
    std::cout << "[Kill Switch] Removing all startup persistence..." << std::endl;

    if (setRegistryRunKey(HKEY_CURRENT_USER, false)) {
        std::cout << "  [+] Removed HKCU Run key." << std::endl;
    }
    if (setRegistryRunKey(HKEY_LOCAL_MACHINE, false)) {
        std::cout << "  [+] Removed HKLM Run key." << std::endl;
    }
    if (removeStartupFolderFile(false)) {
        std::cout << "  [+] Removed User Startup folder copy." << std::endl;
    }
    if (removeStartupFolderFile(true)) {
        std::cout << "  [+] Removed All Users Startup folder copy." << std::endl;
    }
    if (setScheduledTask(false)) {
        std::cout << "  [+] Removed Task Scheduler task." << std::endl;
    }
    std::cout << "[Kill Switch] Cleanup complete." << std::endl;
}

// Check if a process name matches League of Legends components
bool isLeagueProcess(const TCHAR* processName) {
    if (processName == NULL || _tcslen(processName) == 0) {
        return false;
    }
    for (size_t i = 0; i < TARGET_PROCESSES_COUNT; i++) {
        if (_tcsicmp(processName, TARGET_PROCESSES[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Display alert pop-up to the user
void createPopup() {
    LPCTSTR messageText = TEXT("No league for u.\nGo do your leetcode.");
    LPCTSTR captionText = TEXT("NoLeague.exe");
    UINT type = MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL;
    MessageBox(NULL, messageText, captionText, type);
}

// Inspect a process by ID and terminate if it matches League of Legends
bool findAndKill(DWORD processID) {
    if (processID == 0 || processID == GetCurrentProcessId()) {
        return false;
    }

    // Query process name
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    if (hProcess == NULL) {
        hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
        if (hProcess == NULL) {
            return false;
        }
    }

    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
    HMODULE hMod;
    DWORD cbNeeded;

    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
        GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
    } else {
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageName(hProcess, 0, szProcessName, &size)) {
            const TCHAR* baseName = _tcsrchr(szProcessName, TEXT('\\'));
            if (baseName != NULL) {
                _tcscpy_s(szProcessName, MAX_PATH, baseName + 1);
            }
        }
    }
    CloseHandle(hProcess);

    // Check if process matches League of Legends
    if (!isLeagueProcess(szProcessName)) {
        return false;
    }

    std::cout << "[Detection] Found League of Legends process: " << szProcessName
              << " (PID: " << processID << ")" << std::endl;

    // Terminate process
    bool terminated = false;
    HANDLE hKill = OpenProcess(PROCESS_TERMINATE, FALSE, processID);
    if (hKill != NULL) {
        terminated = (TerminateProcess(hKill, 1) != FALSE);
        CloseHandle(hKill);
    }

    // Fallback via taskkill command if direct handle termination is denied
    if (!terminated) {
        TCHAR cmd[128];
        _sntprintf(cmd, sizeof(cmd) / sizeof(TCHAR), TEXT("taskkill /F /PID %u"), processID);
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, 3000);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            terminated = true;
        }
    }

    if (terminated) {
        std::cout << "[Action] Terminated " << szProcessName << "." << std::endl;
        createPopup();
        ShellExecute(NULL, TEXT("open"), WEBSITE, NULL, NULL, SW_SHOWNORMAL);
    } else {
        std::cerr << "[Error] Failed to terminate " << szProcessName << ". Error: " << GetLastError() << std::endl;
    }

    return true;
}

// Enumerate running processes and terminate League if present
void scanAndKillLeague() {
    DWORD processes[2048], cbNeeded, cProcesses;
    if (!EnumProcesses(processes, sizeof(processes), &cbNeeded)) {
        return;
    }

    cProcesses = cbNeeded / sizeof(DWORD);
    for (unsigned int i = 0; i < cProcesses; i++) {
        if (processes[i] != 0) {
            if (findAndKill(processes[i])) {
                break;
            }
        }
    }
}

// Print command-line help
void printUsage(const TCHAR* exeName) {
    std::cout << "No More League - Anti-League Background Guardian\n"
              << "Usage: " << exeName << " [option]\n\n"
              << "Options:\n"
              << "  (no args)      Run background monitoring with multi-layered persistence\n"
              << "  --kill, -k     Activate kill switch: remove persistence and terminate\n"
              << "  --resume, -r   Deactivate kill switch: re-enable persistence and monitor\n"
              << "  --status, -s   Display current persistence and kill switch status\n"
              << "  --help, -h     Show this help screen\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    if (!isSystemWindows()) {
        std::cerr << "This application is designed only for Windows systems." << std::endl;
        return 1;
    }

    // CLI Argument Handling
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--kill" || arg == "-k" || arg == "--uninstall" || arg == "--stop") {
            setKillSwitch(true);
            removePersistence();
            std::cout << "Kill switch activated. No More League is deactivated." << std::endl;
            return 0;
        } else if (arg == "--resume" || arg == "-r" || arg == "--install" || arg == "--start") {
            setKillSwitch(false);
            std::cout << "Kill switch deactivated. Starting monitoring..." << std::endl;
        } else if (arg == "--status" || arg == "-s") {
            bool active = isKillSwitchActive();
            std::cout << "Kill Switch Status: " << (active ? "ACTIVATED (disabled)" : "DEACTIVATED (active)") << std::endl;
            return 0;
        } else if (arg == "--help" || arg == "-h" || arg == "/?") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cout << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // Check if Kill Switch is engaged in Registry
    if (isKillSwitchActive()) {
        std::cout << "[Info] Kill switch is active in registry (HKCU\\Software\\NoMoreLeague\\KillSwitch = 1).\n"
                  << "Monitoring is deactivated. To re-enable, run with --resume or set KillSwitch to 0."
                  << std::endl;
        return 0;
    }

    // Single-instance protection via Named Mutex
    HANDLE hMutex = CreateMutex(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cout << "Another instance of No More League is already running. Exiting." << std::endl;
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    std::cout << "============================================" << std::endl;
    std::cout << "         No More League - Active            " << std::endl;
    std::cout << "============================================" << std::endl;

    // Enforce initial persistence across multiple vectors
    createPersistence();

    // Main background monitoring loop
    while (true) {
        // Dynamic kill switch check: allows remote/registry deactivation while running
        if (isKillSwitchActive()) {
            std::cout << "[Kill Switch] Detected active kill switch. Disabling and cleaning up..." << std::endl;
            removePersistence();
            break;
        }

        // Self-healing: re-enforce persistence in case registry or startup folder was tampered with
        createPersistence();

        // Scan and terminate League of Legends
        std::cout << "[Check] Scanning active processes..." << std::endl;
        scanAndKillLeague();

        // Sleep for 30 seconds before next check
        Sleep(30000);
    }

    if (hMutex) {
        CloseHandle(hMutex);
    }
    return 0;
}

