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
- **Hidden Registry Kill Switch**: There is **no command-line option** to stop the program. The only way to deactivate it is to know the kill-switch registry value and flip it by hand — a deliberate speed bump against impulsive quitting.

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

None. The executable ignores all arguments. This is intentional: there is no `--kill`,
`--resume`, or `--status` flag, so the program cannot be shut off with a quick command.

---

## Kill Switch via Registry

The only way to deactivate the guardian is through the registry — you have to know
the exact key and value:

1. Set the DWORD value `HKCU\Software\NoMoreLeague\KillSwitch` to `1`
   (e.g. in `regedit`, or via a `.reg` file you write yourself).
   - On its next 30-second check, the running background process detects this,
     removes all persistence entries (Run keys, Startup folder copies, the
     scheduled task), and exits.
2. To re-enable, set `KillSwitch` back to `0` (or delete the value) and run
   `no-more-league.exe` again.

Because this is a self-control tool, the registry step is deliberately not exposed
through the program itself — the friction of having to remember and edit the key by
hand is the point.
