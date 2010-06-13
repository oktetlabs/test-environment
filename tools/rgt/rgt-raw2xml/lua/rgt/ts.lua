---
-- Report generation tool - timestamp module.
--
-- Copyright (C) 2010 Test Environment authors (see file AUTHORS in the
-- root directory of the distribution).
-- 
-- Test Environment is free software; you can redistribute it and/or
-- modify it under the terms of the GNU General Public License as
-- published by the Free Software Foundation; either version 2 of
-- the License, or (at your option) any later version.
-- 
-- Test Environment is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
-- 
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 59 Temple Place, Suite 330, Boston,
-- MA  02111-1307  USA
-- 
-- @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
-- 
-- @release $Id$
--

local oo    = require("loop.base")
local rgt   = {}
rgt.ts      = oo.class({
                        s   = nil,  -- Seconds
                        us  = nil,  -- Microseconds
                       })


function rgt.ts:__init(seconds, microseconds)
    return oo.rawnew(self, {s = seconds, us = microseconds})
end

function rgt.ts.__sub(x, y)
    if x.us > y.us then
        return rgt.ts(x.s - y.s, x.us - y.us)
    else
        return rgt.ts(x.s - y.s - 1, 10 ^ 6 - (y.us - x.us))
    end
end

function rgt.ts.__eq(x, y)
    return x.s == y.s and x.us == y.us
end

function rgt.ts.__lt(x, y)
    return x.s < y.s or x.s == y.s and x.us < y.us
end

function rgt.ts.__le(x, y)
    return x.s < y.s or x.s == y.s and x.us <= y.us
end

function rgt.ts:format_short_abs()
    -- return os.date(("%%H:%%M:%%S %3u ms"):
    -- FIXME MIMICKING ORIGINAL
    return os.date(("%%H:%%M:%%S %u ms"):
            format(self.us / 10 ^ 3), self.s)
end

function rgt.ts:format_short_rel()
    -- return ("%.2u:%.2u:%.2u %3u ms"):
    -- FIXME MIMICKING ORIGINAL
    return ("%u:%u:%u %u ms"):
                format(math.floor(self.s / (60 * 60)),
                       math.floor(self.s % (60 * 60) / 60),
                       self.s % 60,
                       self.us / 10 ^ 3)
end


return rgt.ts
