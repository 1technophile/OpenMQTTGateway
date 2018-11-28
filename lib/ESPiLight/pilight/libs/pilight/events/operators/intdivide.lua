-- Copyright (C) 2013 - 2016 CurlyMo

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local M = {}

function M.run(a, b)
	if pilight.tonumber(a) == 0 or pilight.tonumber(b) == 0 then
		return 0;
	else
		aa = pilight.tonumber(a);
		bb = pilight.tonumber(b);
		if aa < 0 then
			return -math.floor(-aa / bb);
		else
			return math.floor(aa / bb);
		end
	end
end

function M.associativity()
	return 70;
end

function M.precedence()
	return 1;
end

function M.info()
	return {
		name = "\\",
		version = "1.0",
		reqversion = "5.0",
		reqcommit = "87"
	}
end

return M;