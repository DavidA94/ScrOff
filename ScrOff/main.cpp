#include <fstream>
#include <string>
#include <Windows.h>

// #define DEBUG
#ifdef DEBUG
std::ofstream mylog("ScrOffLog.txt");
#endif

void makeRunOnStartup() {
    const char REG_KEY_PATH[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    const char REG_VALUE_NAME[] = "ScrOff";

    char appLoc[MAX_PATH];      // Holds the location of the currently running application
    char regData[MAX_PATH];     // Holds the value/data we retrieve from the registry
    DWORD bufSize = MAX_PATH;   // Holds how big the regData buffer can be
    HKEY regKey = 0;            // Holds the opened registry key

    // Get the path to the currently running application
    GetModuleFileNameA(NULL, appLoc, MAX_PATH);

    // Create/Open to the registry key
    auto status = RegCreateKeyExA(HKEY_CURRENT_USER, REG_KEY_PATH, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, NULL, &regKey, NULL);

    // If something went wrong, then try the curent user
    if (status != ERROR_SUCCESS) {
        #ifdef DEBUG
        mylog << "Unable to acess the registry. Got code " << status << std::endl;
        #endif

        RegCloseKey(regKey);
        return;
    }
    
    // Try to get the registry value to see if it already exists
    status = RegGetValueA(HKEY_CURRENT_USER, REG_KEY_PATH, REG_VALUE_NAME, RRF_RT_REG_SZ, NULL, &regData, &bufSize);

    // If we got a bad key, then we need to create it
    if (status == ERROR_BADKEY || status == ERROR_FILE_NOT_FOUND) {
        status = RegSetValueExA(regKey, REG_VALUE_NAME, 0, REG_SZ, (unsigned char*)appLoc, (unsigned long)strlen(appLoc));

        #ifdef DEBUG
        mylog << "Got status" << status << "when adding value to registry.\n";
        #endif
    }
    // Otherwise, if all was successful, 
    else if (status == ERROR_SUCCESS) {
        // But the value in the registry is not what program is currently running
        if (strcmp(regData, appLoc) != 0) {
            // Check if it points to a valid file
            if (FILE* f = fopen(regData, "r")) {
                fclose(f);

                #ifdef DEBUG
                mylog << "The registry already points to " << regData << ", which is a valid file.\n";
                #endif
            }
            else {
                status = RegSetValueExA(regKey, REG_VALUE_NAME, 0, REG_SZ, (unsigned char*)appLoc, (unsigned long)strlen(appLoc));

                #ifdef DEBUG
                mylog << "Got status " << status << " when adding value to registry.\n";
                #endif
            }
        }
        else {
            #ifdef DEBUG
            mylog << "The registry is already set to " << appLoc << std::endl;
            #endif
        }
    }

    RegCloseKey(regKey);
}

int __stdcall WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
    const int TURN_OFF_SCREEN = 1;

    // Make this program run on startup if it isn't already
    makeRunOnStartup();

    // Register the hotkey that will turn off the monitor [Ctrl] + [Shift] + Q
    RegisterHotKey(NULL, TURN_OFF_SCREEN, MOD_CONTROL | MOD_SHIFT | MOD_ALT | MOD_NOREPEAT, 'Z');

    // Infinite loop to watch for the shortcut
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0) != 0) {
        // If we get a hotkey
        if (msg.message == WM_HOTKEY) {
            #ifdef DEBUG
            mylog << "Got Hotkey\n";
            #endif

            // If we got the hotkey that turns off the screen
            if (msg.wParam == TURN_OFF_SCREEN) {
                #ifdef DEBUG
                mylog << "Turning off Monitor\n";
                #endif
                
                // Wait half a second for the keys to be released, then turn off the screen
                Sleep(500);

                // HWND_BROADCAST: Broadcast the message to everybody
                // WM_SYSCOMMAND: Sending a system command
                // SC_MONITORPOWER: Targeting the monitor power
                // 2: Turn off the monitor
                SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
            }
        }
    }
}