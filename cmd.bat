:: Run as Administrator
bcdedit /set testsigning on
bcdedit /set nointegritychecks on
bcdedit /debug on
bcdedit /dbgsettings serial debugport:1 baudrate:115200
reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System" /v EnableLUA /t REG_DWORD /d 0 /f
shutdown /r /t 5