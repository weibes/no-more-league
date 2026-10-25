# How to Kill No More League

This document details how to actually deactivate NoMoreLeague. By design, it is **not trivial** to remove — that is the point. You must be deliberate about it.

## Prerequisite: Know what you're doing

NoMoreLeague monitors your system continuously. It restarts itself from multiple hidden locations (registry keys, startup folders, scheduled tasks, decoy backup copies). Closing Task Manager or deleting one file will **not** remove it.

## The Hard Way (Intended Kill Switch)

This is the method built into the tool:

1. Open **Registry Editor** (`regedit.exe`)
2. Navigate to `HKEY_CURRENT_USER\Software\NoMoreLeague`
3. Set the DWORD value `KillSwitch` to `1`
4. Wait up to 30 seconds for the process to detect it and begin cleanup
5. **Watch Task Manager** — after cleanup, NoMoreLeague will auto-relaunch *once* to confirm deletion before finally exiting. Let this run. It should terminate after ~5 seconds.

To re-enable: Set `KillSwitch` back to `0` or delete the value, then run `no-more-league.exe`.

## The Forced Way (If You Really Need To Play)

If you can't be bothered with the proper method, here is everything it installs:

### Step 1: Stop the running process
Open Task Manager and end `no-more-league.exe`. Multiple copies may exist if decoy backups have already revived it.

### Step 2: Remove registry entries

**Open regedit and delete these keys:**
- `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run\NoMoreLeague`
- `HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Run\NoMoreLeague` (only exists if previously run as admin)
- `HKEY_CURRENT_USER\Software\Microsoft\Windows NT\CurrentVersion\Winlogon\Shell` — **restore to just `explorer.exe`** (do not delete the Shell key entirely!)
- Any of these values under HKCU in the Winlogon section:
  - `NoMoreLeagueBackup`

### Step 3: Remove scheduled tasks

Open an elevated Command Prompt and run:

```cmd
schtasks /delete /tn "NoMoreLeague" /f
schtasks /delete /tn "SystemHealthCheck" /f
schtasks /delete /tn "WindowsDefenderUpdate" /f
```

### Step 4: Delete copied files

Remove `no-more-league.exe` from these locations:
- `%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\`
- `%ALLUSERSPROFILE%\Start Menu\Programs\Startup\` (as admin)
- `%TEMP%\updatesrv.exe`
- `%USERPROFILE%\SystemHelper.exe`

### Step 5: Reboot

After all the above, a reboot is necessary. On startup the Winlogon Shell may still trigger it once, but persistence should be fully broken. If it comes back, **it means you missed something** — start over and check every location again.

## Why It Comes Back (Self-Healing Explanation)

Every 30 seconds while running, NoMoreLeague:
1. Re-writes all registry keys to their startup locations
2. Copies itself into Startup folders
3. Re-registers scheduled tasks
4. Restores decoy backup files from `%APPDATA%` and `%TEMP%`
5. Creates new shell shortcuts

This is why you must do **all steps above before killing the process**, or at minimum ensure every persistence vector is cleared first.

## Still Comes Back?

If it survives a clean reboot, check:
- **Task Scheduler library** — there may be hidden tasks not discoverable via `schtasks /query` (look for recently created tasks with names you don't recognize)
- **Group Policy startup scripts** — unlikely but possible if your machine has domain GPOs applied
- **Third-process injection** — NoMoreLeague cannot protect against rootkits or process injectors
- The most likely explanation: you missed one of the decoy files which revived it during a reboot

Run `schtasks /query` and examine all tasks for anything pointing at `%APPDATA%` or `%TEMP%`.
