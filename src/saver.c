#include "saver.h"

#include <stdlib.h>
#include "format.h"

// initializes saver
void saver_init(saver *S, lua_State *L, int maxnesting) {
	S->L = L;
	S->B = malloc(sizeof *S->B);
	fixbuf_init(L, S->B, _SAVER_I_BUFF);
	S->count = 0;
	S->maxnesting = maxnesting;
}

static int luaB_rawgeti(lua_State *L, int index, int i) {
	lua_rawgeti(L, index, i);
	return !lua_isnil(L, -1);
}

static int saver_process_number(saver *S, int index) {
	int len;
	lua_Number x = lua_tonumber(S->L, index);
	len = sprintf(fixbuf_prepare(S->B, _FORMAT_NUMBER_MAX), "%.*g", _FORMAT_NUMBER_LEN, x);
	S->B->used += len;
	fixbuf_addchar(S->B, _FORMAT_NUMBER_DELIM);
	return 0;
}

static int saver_process_string(saver *S, int index) {
	size_t len;
	const char *s = lua_tolstring(S->L, index, &len);
	fixbuf_addqstring(S->B, s, len);
	return 0;
}

static int saver_process_table(saver *S, int index, int nesting) {
	int err;
	lua_Number x;
	int i = 1;
	
	if(nesting > S->maxnesting) {
		return _SAVER_ERR_TOODEEP;
	}
				
	lua_pushvalue(S->L, index);
	lua_pushnumber(S->L, ++S->count);
	lua_rawset(S->L, _SAVER_I_REG);
		
	fixbuf_addchar(S->B, _FORMAT_TABLE_START);
				
	while(luaB_rawgeti(S->L, index, i++)) {
		if(err = saver_process(S, lua_gettop(S->L), nesting)) {
			return err;
		}
		lua_pop(S->L, 1);
	}
	
	fixbuf_addchar(S->B, _FORMAT_ARRAY_HASH_SEP);
	
	while(lua_next(S->L, index)) {
		if((lua_type(S->L, -2) != LUA_TNUMBER) || (!is_int(x = lua_tonumber(S->L, -2))) || (x >= i) || (x <= 0)) {
			if(err = saver_process(S, lua_gettop(S->L) - 1, nesting)) {
				return err;
			}
			if(err = saver_process(S, lua_gettop(S->L), nesting)) {
				return err;
			}
		}
		lua_pop(S->L, 1);
	}
				
	fixbuf_addchar(S->B, _FORMAT_TABLE_END);
	return 0;
}

// adds value at index to buffer
// stack-balanced
// returns 0 or error code
int saver_process(saver *S, int index, int nesting) {
	if(!lua_checkstack(S->L, 2)) {
		return _SAVER_ERR_STACK;
	}
	switch(lua_type(S->L, index)) {
		case LUA_TNIL: {
			fixbuf_addchar(S->B, _FORMAT_NIL);
			break;
		}
		case LUA_TBOOLEAN: {
			if(lua_toboolean(S->L, index)) {
				fixbuf_addchar(S->B, _FORMAT_TRUE);
			}
			else {
				fixbuf_addchar(S->B, _FORMAT_FALSE);
			}
			break;
		}
		case LUA_TNUMBER: {
			if(lua_rawequal(S->L, index, index)) {
				if(lua_rawequal(S->L, index, _SAVER_I_INF)) {
					fixbuf_addchar(S->B, _FORMAT_INF);
				}
				else {
					if(lua_rawequal(S->L, index, _SAVER_I_MINF)) {
						fixbuf_addchar(S->B, _FORMAT_MINF);
					}
					else {
						fixbuf_addchar(S->B, _FORMAT_NUMBER);
						return saver_process_number(S, index);
					}
				}
			}
			else {
				fixbuf_addchar(S->B, _FORMAT_NAN);
			}
			break;
		}
		case LUA_TSTRING: {
			return saver_process_string(S, index);
			break;
		}
		case LUA_TTABLE: {
			lua_pushvalue(S->L, index);
			lua_rawget(S->L, _SAVER_I_REG);
			if(lua_isnil(S->L, -1)) {
				lua_pop(S->L, 1);
				return saver_process_table(S, index, nesting + 1);
			}
			else {
				int err;
				fixbuf_addchar(S->B, _FORMAT_TABLE_REF);
				if (err = saver_process_number(S, lua_gettop(S->L))) {
					return err;
				}
				lua_pop(S->L, 1);
			}
			break;
		}
		case LUA_TFUNCTION: {
			return _SAVER_ERR_FUNCTION;
			break;
		}
		case LUA_TUSERDATA: {
			return _SAVER_ERR_USERDATA;
			break;
		}
		case LUA_TLIGHTUSERDATA: {
			return _SAVER_ERR_USERDATA;
			break;
		}
		case LUA_TTHREAD: {
			return _SAVER_ERR_THREAD;
			break;
		}
		default: {
			return _SAVER_ERR_UNKNOWN;
			break;
		}
	}
	return 0;
}

// prepares saver for freeing
void saver_free(saver *S) {
	free(S->B);
}

