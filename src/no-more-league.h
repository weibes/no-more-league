#pragma once

#ifndef UNICODE  
  typedef std::string String; 
#else
  typedef std::wstring String; 
#endif

int systemCheck(); 
void findAndKill( DWORD processID );
void createPopup();