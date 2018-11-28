-- Copyright (C) 2013 - 2016 CurlyMo

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local M = {}

local function split(s, delimiter)
	result = {};
	for match in (s..delimiter):gmatch("(.-)"..delimiter) do
		table.insert(result, match);
	end
	return result;
end

local function tablelength(T)
	local count = 0
	for _ in pairs(T) do count = count + 1 end
	return count
end

function M.run(a, b, c)
	local tm = nil;
	local e = nil;

	if c ~= nil then
		error("DATE_ADD requires two arguments");
		return nil;
	elseif a ~= nil and b ~= nil then
		tm = pilight.getdevice(a);
		if tm == nil then
			tm = pilight.strptime("%Y-%m-%d %H:%M:%S", a);
		end
		array = split(b, ' ');
		if tablelength(array) ~= 2 then
			error(string.format("DATE_ADD got an invalid unit \"%s\"", b));
		end
		number = array[1];
		unit = array[2];
	else
		error("DATE_ADD requires two arguments");
		return nil;
	end

	if tonumber(number) == nil then
		error(string.format("DATE_ADD unit parameter must be numeric, \"%s\" given", type(number)));
		return nil;
	end

	if unit == 'YEAR' then
		tm['year'] = tm['year'] + number;
	elseif unit == 'MONTH' then
		tm['month'] = tm['month'] + number;
	elseif unit == 'DAY' then
		tm['day'] = tm['day'] + number;
	elseif unit == 'HOUR' then
		tm['hour'] = tm['hour'] + number;
	elseif unit == 'MINUTE' then
		tm['min'] = tm['min'] + number;
	elseif unit == 'SECOND' then
		tm['sec'] = tm['sec'] + number;
	else
		error(string.format("DATE_ADD does not accept \"%s\" as a unit", unit));
		return nil;
	end
	local t = os.time(tm);
	return os.date("%Y-%m-%d %H:%M:%S", t);
end

function M.info()
	return {
		name = "DATE_ADD",
		version = "2.0",
		reqversion = "7.0",
		reqcommit = "94"
	}
end

return M;