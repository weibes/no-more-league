import psutil as psu
from time import sleep
from ctypes import windll
from webbrowser import open as wbopen
curr_proc = list(psu.win_service_iter())


while(True):

    curr_proc = list(psu.process_iter(['pid', 'name']))
    in_list = False
    req_proc = None
    for proc in curr_proc:
        if proc.name() == 'LolClient.exe' or proc.name() == 'LeagueClient.exe': #TODO get actual league client name
        #if proc.name() == 'firefox.exe': #Test line        
            in_list = True
            req_proc = proc
            break

    if in_list:
        req_proc.kill()
        windll.user32.MessageBoxW(0, 'No More League, please.', 'Monkey', 0)
        wbopen('https://leetcode.com')
    else:
        print('no')
    in_list = False
    sleep(30)