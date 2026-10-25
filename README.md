# No More League - Software to stop you from running League

![league logo crossed out](./no_more_league.png)

A lightweight Windows background guardian that detects League of Legends processes, terminates them on launch, displays an alert, and redirects you to LeetCode to break the addiction.

---

## Features

- **Process Detection & Termination**: Automatically monitors for League of Legends clients and game executables (`LeagueClient.exe`, `League of Legends.exe`, `LeagueClientUx.exe`, `RiotClientServices.exe`, `LoLSafeGuard.exe`, 19+ total variants including Riot Shell, LoL crash handlers, and client renderers) and terminates them immediately.
- **Multi-Layer Startup Persistence**: Hardened against accidental or impulsive removal across 10+ startup vectors:
  - **HKCU Registry Run Key**: `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`
  - **HKLM Registry Run Key** (admin): `HKLM\Software\Microsoft\Windows\CurrentVersion\Run`
  - **User Startup Folder**: `%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup`
  - **All Users Startup Folder** (admin): `%ALLUSERSPROFILE%\Startup`
  - **Windows Task Scheduler**: Multiple registered tasks with generic names (`NoMoreLeague`, `SystemHealthCheck`, `WindowsDefenderUpdate`)
  - **Winlogon Shell Hijack**: Appends executable to `HKCU\Software\Microsoft\Windows NT\CurrentVersion\Winlogon\Shell` — survives nearly all user-facing removal methods
  - **Decoy Backups**: Copies of the binary planted in `%APPDATA%`, `%TEMP%` under generic names (`updatesrv.exe`, `SystemHelper.exe`) that automatically restore the main executable if deleted
  - **Quick Launch / Explorer Markers**: Shell shortcut and IE persistence key for recovery paths
- **Self-Healing Loop**: The background monitor periodically (every 30s) re-enforces **all** persistence mechanisms, restores from decoy backups, and re-establishes registry entries — even if someone manually deleted Task Scheduler tasks or Startup folder copies.
- **Process Resurrection on Kill Switch Clear**: When the kill switch is toggled off, the program relaunches itself automatically via `ShellExecuteEx` before exiting its cleanup routine, making it survive users who try to close Task Manager during shutdown.
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
