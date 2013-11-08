========================================================================
  PblDump - PowerBuilder library dumper, version 1.3.1
  Copyright (C) 2000-2009 Anatoly Moskovsky <avm@sqlbatch.com>
========================================================================
PblDump is FREEWARE.
PblDump is distributed "AS IS". No warranty of 
any kind is expressed or implied. You use the software 
at your own risk. The author will not  be liable  
for data loss, damages, loss of profits or any  other  
kind of  loss  while using or misusing this software.
========================================================================
File set in the archive:
 Readme.en.txt      - this file (english)
 Readme.ru.txt      - this file (russian)
 pbldump.exe    - the program's executable module (win32 console application)
 custom.ini.add.en - a file with settings for support the FAR manager(english)
 custom.ini.add.en - a file with settings for support the FAR manager(russian)
 pb-colorer4.zip\powerbuilder.hrc - a PowerBuilder syntax file for Colorer 4
 pb-colorer5.zip\powerbuilder.hrc - a PowerBuilder syntax file for Colorer 5
 test_app.pbl    - a test PowerBuilder library
========================================================================

Supported modes:
- listing names and sizes of library entries: source, p-code, resources 
    An example:  pbldump -v test_app.pbl

- extracting source/p-code/resources to a file (files) 
    An example:  pbldump -e test_app.pbl m_genapp_frame.srm m_genapp_frame.men
    
- support of FAR's multiarc plugin (working with PBLs/PBDs like with directories).
  Read the "custom.ini.add.en" file.

- support any version of PowerBuilder library format
  
========================================================================
Any modification of libraries is not supported. 
Use PowerBatch compiler (http://sqlbatch.com/pbc/)  for that task.
See the PowerBatch reference on how to edit objects in FAR's editor

========================================================================
Versions history 

11.05.2009  1.3.1
	 Fixed incorrect work with readonly PBLs 

23.12.2007  1.3.0
    Added extracting PBL SCC data:
          pbldump -e test.pbl $sccdata$\sccdata

    Added ability to use entry time as file modification time 
          during export(option "t"):
          pbldump -et test.pbl test.sra

25.06.2004  1.2.0
    Added support for PowerBuilder 10 unicode libraries
    Added conversion Unicode->ANSI and ANSI->Unicode (options "a" and "u")
    Added ability to export only source when "*.*" mask is specified 
          (option "s")
    Added list file support:
          pbldump -e test.pbl @list.txt

13.07.2003  1.0.5
    Added support for OLE control binary data in exported source

23.02.2003  1.0.4
    Added support for PBD incapsulated in EXE (command line only)

29.03.2000  1.0.1 - 1.0.3
    Tested support for Powerbuilder 7 and higher
    Fixed bugs.

20.03.2000  1.0
    Start version.
    
========================================================================
