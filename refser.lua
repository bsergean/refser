local _M = {}

local crefser = require "refser.crefser"
local aux = require "refser.auxiliary"

local function makecontext(t)
	local mt = getmetatable(t) or {}
	if not mt.__mode then
		mt.__mode = "kv"
		setmetatable(t, mt)
	end
	if not t[0] then
		t[0] = 0
	end
	return t
end

_M.maxnesting = 250

_M.maxtuple = 20

_M.maxitems = 10 ^ 6

function _M.save(...)
	return _M.customsave(...) {}
end

function _M.load(s)
	return _M.customload(s) {}
end

function _M.customsave(...)
	local args = aux.pack(...)
	return function(opts)
		return crefser.save(
			1/0,
			-1/0,
			makecontext(opts.context or {}),
			nil,
			{opts.maxnesting or _M.maxnesting, opts.maxtuple or _M.maxtuple, opts.maxitems or _M.maxitems, opts.doublecontext},
			aux.unpack(args)
		)
	end
end

function _M.customload(s)
	return function(opts)
		return crefser.load(
			1/0,
			-1/0,
			0/0,
			makecontext(opts.context or {}),
			nil,
			{opts.maxnesting or _M.maxnesting, opts.maxtuple or _M.maxtuple, opts.maxitems or _M.maxitems, opts.doublecontext},
			s
		)
	end
end

return _M
