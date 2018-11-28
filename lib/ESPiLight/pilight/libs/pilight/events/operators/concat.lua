-- Copyright (C) 2013 - 2016 CurlyMo

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local M = {}

function M.run(a, b)
	return a .. b;
end

function M.associativity()
	return 40;
end

function M.precedence()
	return 1;
end

function M.info()
	return {
		name = ".",
		version = "1.0",
		reqversion = "5.0",
		reqcommit = "87"
	}
end

return M;