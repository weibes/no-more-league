// imports
#include <Windows.h>
#include <lmcons.h>
#include <tchar.h>
#include <psapi.h>
#include <iostream>
#include <string>
using namespace std;

// Checking to ensure that
int systemCheck() 
{
    string environmentName(getenv("windir"));
    cout << environmentName; // returns "C:/WINDOWS"
    return environmentName.erase(0, 3) != "WINDOWS"; // returns 0 if it DOES equal windows
}
/* 
TODO: Implement.
int setupPersistence() {
    // persistence:
    char currUsername[UNLEN + 1];
    DWORD usernameLen = UNLEN+1;
    GetUserNameA(currUsername, &usernameLen);
    // get current file location
    // copy curr file to startup folder, app
    // 'C:/Users/{uname}/AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Startup'
    return 0;
}
*/

void PrintProcessNameAndID( DWORD processID )
{
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

    // Get a handle to the process.

    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, processID );

    // Get the process name.

    if (NULL != hProcess )
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
             &cbNeeded) )
        {
            GetModuleBaseName( hProcess, hMod, szProcessName, 
                               sizeof(szProcessName)/sizeof(TCHAR) );
        }
    }

    // Print the process name and identifier.

    _tprintf( TEXT("%s  (PID: %u)\n"), szProcessName, processID );

    // Release the handle to the process.

    CloseHandle( hProcess );
}

int main() 
{
    if (!systemCheck) 
    {
        cout << "System running is not windows. This is built only for windows machines atm." << endl;
        return 1;
    }
    DWORD processes[1024], cbNeeded, cProcesses;
    if (!EnumProcesses(processes, sizeof(processes), &cbNeeded))
    {
        return 1;
    }

    // number of process IDs
    cProcesses = cbNeeded / sizeof(DWORD);
    unsigned int i;
    for (i = 0; i < cProcesses; i++)
    {
        if (processes[i] != 0)
        {
            PrintProcessNameAndID(processes[i]);
        }
    }
    return 0;
}