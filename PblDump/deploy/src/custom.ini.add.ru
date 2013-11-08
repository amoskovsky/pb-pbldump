;для подключения PBLDUMP в FAR необходимо добавить содержимое 
;этого файла к файлу "c:\program files\far\plugins\multiarc\formats\custom.ini",
;предварительно удалив секцию [PBL] если она уже есть,
;поместить PBLDUMP.EXE в один из каталогов переменной %PATH% 
;и перезапустить FAR

[PBL]
;Powerbuilder library format (*.pbl, *.pbd)
ID=48 44 52 2A
IDOnly=1
Extension=pbl
List="pbldump -v"
ExtractWithoutPath=pbldump -eta %%A @%%L
Errorlevel=1
Start="^--"
End="^--"
Format0="nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnzzzzzzzzz dd tt yyyy hh mm ss"
