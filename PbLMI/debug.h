#ifndef _debug_h_
#define _debug_h_

#include <stdarg.h>

class Debug {
private:
	static char *m_file;
public:
	static void __cdecl _dprintf (const char * format, ...) {
		va_list args;
		va_start( args, format );
		if (!m_file)
			return;
		FILE *f = fopen(m_file, "at+");
		if (!f)
			return;
		vfprintf( f, format, args );
		fclose(f);
	}
	static void _setLogFile (char * file) {
		m_file = file;
	}
	static void _resetLogFile () {
		unlink(m_file);
	}

	static void _dumpMemory(const void * mem, size_t size = 0, const char * title = NULL) {
		/*
		   size == 0 - sz string
		*/
		if (title) 
			_dprintf("Dumping: %s\n", title);
		if (!size)
			size = strlen((const char*)mem);
		_dumpMemory2((const char*)mem, size);

	}
	static void _dumpMemory2(const char* mem, size_t size) {
		for (size_t i = 0; i < size; i ++) {
			_dprintf("%02X(%c) ", (int)mem[i], mem[i]);
		}
		_dprintf("\n");
	}

	Debug(char * file, bool reset = true) {
		_setLogFile(file);
		if (reset)
			_resetLogFile();
	}


};
#ifdef _DEBUG
	#define p(a) printf("%s\n", a)
	#define pf(a,b) printf(a "\n", b)
	#define dprintf Debug::_dprintf
	#define dumpMemory Debug::_dumpMemory
#else
	#define p(a)
	#define dprintf if(0) Debug::_dprintf
	#define dumpMemory if(0) Debug::_dumpMemory
#endif

#define dp dprintf
#define setLogFile Debug::_setLogFile
#define resetLogFile Debug::_resetLogFile

#define  initDebug static Debug __logfile
#endif