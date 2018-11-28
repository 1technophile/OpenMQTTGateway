-- Copyright (C) 2013 - 2016 CurlyMo

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local M = {}

function M.run(a, b, c, d)
	local tm = nil;
	local e = nil;

	if d ~= nil then
		error("DATE_FORMAT requires no more than three arguments");
		return nil;
	elseif a ~= nil and b ~= nil and c ~= nil then
		e = c;
		tm = pilight.strptime(b, a);
	elseif a ~= nil and b ~= nil then
		e = b;
		tm = pilight.getdevice(a);
		if tm == nil then
			error(string.format("DATE_FORMAT device \"%s\" is not a datetime protocol", a));
		end
	else
		error("DATE_FORMAT requires at least two arguments");
		return nil;
	end
	local t = os.time(tm);
	return os.date(e, t);
end


function M.info()
	return {
		name = "DATE_FORMAT",
		version = "1.0",
		reqversion = "6.0",
		reqcommit = "94"
	}
end

return M;