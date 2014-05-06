!IFDEF warn
warnFLAGS = /Wall /sdl \
	/RTC1 /RTCc /RTCs /RTCu
!ENDIF

#debugFLAGS = /Zi /Fdwiniconview.pdb
debugFLAGS = /Z7

# TODO prune the .lib files
neededFLAGS = /TP \
	/link user32.lib kernel32.lib comctl32.lib \
	comdlg32.lib gdi32.lib msimg32.lib \
	shell32.lib advapi32.lib ole32.lib \
	shlwapi.lib

all:
	del winiconview.exe *.obj
	$(CC) $(debugFLAGS) /Fewiniconview.exe *.c $(CFLAGS) $(LDFLAGS) $(warnFLAGS) $(neededFLAGS)
	del *.obj
