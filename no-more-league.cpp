// imports
#include <windows.h>
#include <lmcons.h>

int main() {

    // persistence:
    char currUsername[UNLEN + 1];
    DWORD usernameLen = UNLEN+1;
    GetUserNameA(currUsername, &usernameLen);
    // get current file location
    // copy curr file to startup folder, app
    // 'C:/Users/{uname}/AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Startup'

    // main check look
    while(true) {
        // curr_proc = list(psu.process_iter(['pid', 'name', 'username']))
        bool inList = false;
        // process req_proc = none;
        for (i = 0; i = curr_proc.lengthl i++) {
            
        }
    }

    return 0;
}