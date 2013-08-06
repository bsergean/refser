#include "loader.hpp"

#include <stdlib.h>
#include "format.hpp"

#define ensure(cond) { \
	if(!(cond)) { \
		throw _LOADER_ERR_MAILFORMED; \
	} \
}

#define valid_number_char(c) (((c) <= '9' && (c) >= '0') || (c) == 'e' || (c) == '.' || (c) == '-')

Loader::Loader(Lua *L) {
	this->L = L;
	this->nesting = 0;
	this->items = 0;
	
	L->rawgeti(_LOADER_I_REG, 0);
	this->count = L->tonumber(-1);
	L->pop();
	
	L->rawgeti(_LOADER_I_OPTS, 1);
	this->maxnesting = L->tonumber(-1);
	L->pop();
	
	L->rawgeti(_LOADER_I_OPTS, 2);
	this->maxtuple = L->tonumber(-1);
	L->pop();
	
	L->rawgeti(_LOADER_I_OPTS, 3);
	this->maxitems = L->tonumber(-1);
	L->pop();
	
	L->rawgeti(_LOADER_I_OPTS, 4);
	this->doublecontext = L->toboolean(-1);
	L->pop();
	
	L->remove(_LOADER_I_OPTS);
	
	this->s = L->checklstring(_LOADER_I_X, &this->len);
	
	this->B = new FixBuf(L, _LOADER_I_BUFF);
}

Loader::~Loader() {
	delete this->B;
}

void Loader::eat() {
	this->s++;
	this->len--;
}

void Loader::eat(size_t size) {
	this->s += size;
	this->len -= size;
}

void Loader::process_number() {
	size_t i = 0;
	lua_Number x;
	while(valid_number_char(this->s[i]) && i < _FORMAT_NUMBER_MAX) {
		i++;
	}
	ensure(!valid_number_char(this->s[i]));
	x = strtod(this->s, NULL);
	ensure(x || (i == 1 && *this->s == '0'));
	this->eat(i);
	this->L->pushnumber(x);
	if(!this->L->rawequal(-1, -1)) {
		throw _LOADER_ERR_MAILFORMED;
	}
}

void Loader::process_string() {
	char esc;
	size_t i = 0;
	this->B->reset();
	while(this->s[i] != '"') {
		if(this->s[i] == '\\') {
			ensure(this->len > i + 1);
			switch(this->s[i+1]) {
				case '\\': {
					esc = '\\';
					break;
				}
				case 'n': {
					esc = '\n';
					break;
				}
				case 'r': {
					esc = '\r';
					break;
				}
				case '"': {
					esc = '"';
					break;
				}
				case 'z': {
					esc = '\0';
					break;
				}
				default: {
					throw _LOADER_ERR_MAILFORMED;
					break;
				}
			}
			this->B->add(this->s, i);
			this->B->add(esc);
			this->eat(i + 2);
			i = 0;
		}
		else {
			i++;
		}
		ensure(this->len > i);
	}
	
	this->B->add(this->s, i);
	
	this->eat(i + 1);
	
	this->B->pushresult();		
}

void Loader::process_table() {
	int i = 1;
	
	this->nesting++;
	if(this->nesting > this->maxnesting) {
		throw _LOADER_ERR_TOODEEP;
	}
	
	this->count++;
	this->L->newtable();
	this->L->pushvalue(-1);
	this->L->rawseti(_LOADER_I_REG, this->count);
	
	if(this->doublecontext) {
		this->L->pushvalue(-1);
		this->L->pushnumber(this->count);
		this->L->rawset(_LOADER_I_REG);
	}
	
	while(*this->s != _FORMAT_ARRAY_HASH_SEP) {
		this->process(_LOADER_ROLE_VALUE);
		this->L->rawseti(-2, i++);
	}
	
	this->eat();
	
	while(*this->s != _FORMAT_TABLE_END) {
		this->process(_LOADER_ROLE_KEY);
		this->process(_LOADER_ROLE_VALUE);
		this->L->rawset(-3);
	}
	
	this->eat();
	this->nesting--;
}

// reads next value from string
// puts it on top of lua stack
void Loader::process(int role) {
	this->items++;
	if(this->items > this->maxitems) {
		throw _LOADER_ERR_ITEMS;
	}
	ensure(this->len);
	if(!this->L->checkstack(2)) {
		throw _LOADER_ERR_STACK;
	}
	this->eat();
	switch(this->s[-1]) {
		case _FORMAT_NIL: {
			ensure(role == _LOADER_ROLE_NONE);
			this->L->pushnil();
			break;
		}
		case _FORMAT_TRUE: {
			this->L->pushboolean(1);
			break;
		}
		case _FORMAT_FALSE: {
			this->L->pushboolean(0);
			break;
		}
		case _FORMAT_INF: {
			this->L->pushvalue(_LOADER_I_INF);
			break;
		}
		case _FORMAT_MINF: {
			this->L->pushvalue(_LOADER_I_MINF);
			break;
		}
		case _FORMAT_NAN: {
			ensure(role != _LOADER_ROLE_KEY);
			this->L->pushvalue(_LOADER_I_NAN);
			break;
		}
		case _FORMAT_TABLE_REF: {
			this->process_number();
			this->L->rawget(_LOADER_I_REG);
			if(this->L->isnil(-1)) {
				throw _LOADER_ERR_CONTEXT;
			}
			break;
		}
		case _FORMAT_TABLE_START: {
			this->process_table();
			break;
		}
		case _FORMAT_NUMBER: {
			this->process_number();
			break;
		}
		case _FORMAT_STRING: {
			this->process_string();
			break;
		}
		default: {
			throw _LOADER_ERR_MAILFORMED;
			break;
		}
	}
}

int Loader::done() {
	return !this->len;
}

void Loader::pushresult() {
	this->L->pushnumber(this->count);
	this->L->rawseti(_LOADER_I_REG, 0);
	this->L->pushnumber(this->L->gettop() - _LOADER_I_X);
	this->L->replace(_LOADER_I_X);
	
}
