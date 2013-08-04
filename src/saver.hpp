#ifndef SAVER_H
#define SAVER_H

#include "lua.hpp"
#include "writer.hpp"

#define _SAVER_I_INF 1
#define _SAVER_I_MINF 2
#define _SAVER_I_REG 3
#define _SAVER_I_BUFF 4
#define _SAVER_I_OPTS 5
#define _SAVER_I_X 5

#define _SAVER_ERR_TOODEEP "refser.save error: table is too deep"
#define _SAVER_ERR_STACK "refser.save error: lua stack exhausted"
#define _SAVER_ERR_FUNCTION "refser.save error: attempt to save function"
#define _SAVER_ERR_USERDATA "refser.save error: attempt to save userdata"
#define _SAVER_ERR_THREAD "refser.save error: attempt to save thread"
#define _SAVER_ERR_ITEMS "refser.save error: too many items"
#define _SAVER_ERR_TOOLONG "refser.save error: tuple is too long"

class Saver {
	private:
		Lua *L;
		Writer *B;
		int count;
		int nesting;
		int maxnesting;
		int items;
		int maxitems;
		int doublecontext;
		
		void process_number(int index);
		void process_string(int index);
		void process_table(int index);
	public:
		Saver(Lua *L);
		~Saver();
		void process(int index);
		void pushresult();
};

#endif
