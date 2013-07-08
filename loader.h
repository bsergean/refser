#ifndef LOADER_H
#define LOADER_H

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "fixbuf.h"

#define _LOADER_I_INF 1
#define _LOADER_I_MINF 2
#define _LOADER_I_NAN 3
#define _LOADER_I_X 4
#define _LOADER_I_REG 5
#define _LOADER_I_BUFF 6

#define _LOADER_ERR_TOODEEP 1
#define _LOADER_ERR_MAILFORMED 2

typedef struct loader {
	lua_State *L;
	fixbuf *B;
	const char *s;
	size_t len;
	int count;
} loader;

// initializes loader
void loader_init(loader *LO, lua_State *L, const char *s, size_t len);

// reads next value from string
// puts it on top of lua stack
// returns 0 or error code
int loader_process(loader *LO);

#endif
