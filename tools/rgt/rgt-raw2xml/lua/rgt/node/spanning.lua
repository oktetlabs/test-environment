---
-- Report generation tool - spanning node interface.
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

local oo            = require("loop.base")
local rgt           = {}
rgt.ts              = require("rgt.ts")
rgt.node            = {}
rgt.node.spanning   = oo.class({
                                ["start"]   = nil,  --- start timestamp
                                ["end"]     = nil,  --- end timestamp
                                ["result"]  = nil,  --- result
                                ["err"]     = nil,  --- error message
                               })

function rgt.node.spanning:__init(inst)
    assert(type(inst) == "table")
    assert(oo.instanceof(inst.start, rgt.ts))
    assert(inst.end == nil)
    assert(inst.result == nil)
    assert(inst.err == nil)

    return oo.rawnew(self, inst)
end

function rgt.node.spanning:finish(ts, result, err)
    assert(oo.instanceof(ts, rgt.ts))
    assert(ts >= self.start)
    assert(type(result) == "string")
    assert(type(err) == "string")

    self["end"] = ts
    self.result = result
    self.err    = err
end

return rgt.node.spanning
