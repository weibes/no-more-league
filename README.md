# No More League - Software to stop you from running League

![league logo crossed out](./no_more_league.png)

A lightweight Windows background guardian that detects League of Legends processes, terminates them on launch, displays an alert, and redirects you to LeetCode to break the addiction.

---

## Features

- **Process Detection & Termination**: Automatically monitors for League of Legends clients and game executables (`LeagueClient.exe`, `League of Legends.exe`, `LeagueClientUx.exe`, etc.) and terminates them immediately.
- **Multi-Layered Startup Persistence**: Hardens against accidental or impulsive removal across multiple startup vectors:
  - **User Registry Run Key**: `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`
  - **User Startup Folder**: `%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup`
  - **Windows Task Scheduler**: Registered on-logon scheduled task (`schtasks`)
  - **Admin Vectors (Optional)**: Automatically writes to `HKLM` Run key and Common Startup folder if run as Administrator.
  - **Self-Healing Loop**: The background monitor periodically verifies and re-enforces all persistence mechanisms if any were deleted or tampered with.
- **Single-Instance Protection**: Enforces a named Windows mutex (`Local\NoMoreLeagueSingleInstanceMutex`) so duplicate instances never conflict or create double popups.
- **Fail-Safe Kill Switch**: Built-in kill switch to cleanly deactivate the software and uninstall all persistence entries whenever you are ready.

---

## Building

Requires CMake 3.10+ and a C++17 Windows compiler (MSVC or MinGW):

```cmd
cmake -B build
cmake --build build --config Release
```

The compiled binary will be placed in `build/src/Release/no-more-league.exe` (MSVC) or `build/src/no-more-league.exe` (MinGW).

---

## Usage

### Normal Background Execution
Simply run the executable:
```cmd
no-more-league.exe
```
This automatically establishes persistence across all startup locations and begins background monitoring every 30 seconds.

### Command Line Options

- `no-more-league.exe --kill` (or `-k`): **Activates the kill switch**, removes all persistence entries (Registry keys, Startup folder files, Task Scheduler task), and exits.
- `no-more-league.exe --resume` (or `-r`): **Deactivates the kill switch**, re-installs startup persistence, and resumes monitoring.
- `no-more-league.exe --status` (or `-s`): Displays whether the kill switch is currently active.
- `no-more-league.exe --help` (or `-h`): Shows the help screen.

---

## Kill Switch via Registry

If the process is running in the background or you want to disable it without the command line:

1. Double-click **`killswitch_on.reg`** to set `HKCU\Software\NoMoreLeague\KillSwitch = 1`.
   - The running background process will detect this during its next check, remove all persistence entries, and exit automatically.
2. To turn it back on later, double-click **`killswitch_off.reg`** and run `no-more-league.exe`.
