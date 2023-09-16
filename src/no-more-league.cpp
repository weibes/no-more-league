// imports
#include <windows.h>
#include <lmcons.h>
#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;

// Checking to ensure that
int systemCheck() {
    string environmentName(getenv("windir"));
    cout << environmentName; // returns "C:/WINDOWS"
    return environmentName.erase(0, 3) != "WINDOWS"; // returns 0 if it DOES equal windows
}

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
/*
int checkProc() {
    while (false) {
        // curr_proc = list(psu.process_iter(['pid', 'name', 'username']))
        bool inList = false;
        // process req_proc = none;
        string curr_proc = "hello"; // TEMPORARY, TODO PUT IN PROPERLY
        for (int i = 0; i = curr_proc.length();  i++) {

        }
    }
    return 0;
}

*/
int main() {
    if (!systemCheck) {
        return 1;
    }
    // we know we are on windows now
    setupPersistence();
    //checkProc();
    // return 0;
}


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
