import psutil as psu
from time import sleep
from getpass import getuser
from ctypes import windll
from webbrowser import open as wbopen
from shutil import copy
DEBUG = False # solely for laziness, switch to False when actually building
if DEBUG:
    LEAGUE_CLIENT_POTENTIAL_NAMES = ['chrome.exe']
else:
    LEAGUE_CLIENT_POTENTIAL_NAMES = ['LolClient.exe', 'LeagueClient.exe']

# Create persistence
curr_user = getuser()
curr_file = __file__
startup_location = 'C:/Users/{uname}/AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Startup'.format(uname=curr_user)
copy(curr_file, startup_location)

# main loop to check for league proc
curr_proc = list(psu.win_service_iter())
while(True): #lol
    curr_proc = list(psu.process_iter(['pid', 'name', 'username']))
    in_list = False
    req_proc = None
    # Checking procs for league
    for proc in curr_proc:
        try:
            proc_user = proc.username().split('\\')[-1]
            if proc.name() in LEAGUE_CLIENT_POTENTIAL_NAMES and proc_user == req_user:        
                in_list = True
                req_proc = proc
                break
        except (PermissionError, psu.AccessDenied) as e:
            pass

    # do action to close LoL, then launch Leetcode
    if in_list:
        req_proc.kill()
        windll.user32.MessageBoxW(0, 'No More League, please.', 'Monkey', 0)
        wbopen('https://leetcode.com')
    else:
        print('no')
    in_list = False
    sleep(30)