---
-- Report generation tool - test node type.
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

local oo                = require("loop.multi")
local rgt               = {}
rgt.node                = {}
rgt.node.basic          = require("rgt.node.basic")
rgt.node.named          = require("rgt.node.named")
rgt.node.parametrized   = require("rgt.node.parametrized")
rgt.node.spanning       = require("rgt.node.spanning")
rgt.node.test           = oo.class({
                                    tin         = nil,  -- Test ID number
                                    page        = nil,  -- Page string
                                    verdicts    = nil,  -- Verdict array
                                   },
                                   rgt.node.basic,
                                   rgt.node.named,
                                   rgt.node.parametrized,
                                   rgt.node.spanning)

function rgt.node.test:__init(inst)
    rgt.node.basic:__init(inst)
    rgt.node.named:__init(inst)
    rgt.node.parametrized:__init(inst)
    rgt.node.spanning:__init(inst)

    assert(inst.tin == nil or
           type(inst.tin) == "number" and
           math.floor(inst.tin) == inst.tin and
           inst.tin >= 0)
    assert(inst.page == nil or type(inst.page) == "string")
    assert(inst.verdicts == nil or type(inst.verdicts) == "table")

    if inst.verdicts == nil then
        inst.verdicts = {}
    end

    return oo.rawnew(self, inst)
end

---
-- Start node output.
--
-- @param chunk The chunk to start output to.
--
function rgt.node.test:start(chunk)
    rgt.node.basic.start(chunk)

    self.head_chunk = self.chunk
    self.chunk = self.head_chunk:fork_next()
    self.tail_chunk = self.chunk:fork_next()
end

function rgt.node.test:finish(ts, result, err)
    rgt.node.spanning:finish(self, ts, result, err)
    rgt.node.branching:finish(self)

    self.head_chunk:start_tag("test",
                              {name = self.name,
                               result = self.result,
                               err = self.err}):
                    start_tag("meta"):
                    element("start-ts", nil, self.start:format_short_abs()):
                    element("end-ts", nil, self["end"]:format_short_abs()):
                    element("duration", nil,
                            (self["end"] - self.start):format_short_rel()):
                    element("objective", nil, self.objective)

    if #self.authors > 0 then
        self.head_chunk:start_tag("authors")
        for i, a in self.authors do
            self.head_chunk:element("author", {email = a})
        end
        self.head_chunk:end_tag("authors"):
    end

    if #self.args > 0 then
        self.head_chunk:start_tag("params")
        for i, a in self.args do
            self.head_chunk:element("param", {name = a[1], value = a[2]})
        end
        self.head_chunk:end_tag("params")
    end

    if #self.verdicts > 0 then
        self.head_chunk:start_tag("verdicts")
        for i, v in self.verdicts do
            self.head_chunk:element("verdict", nil, v)
        end
        self.head_chunk:end_tag("verdicts"):
    end

    self.head_chunk:end_tag("meta")
    self.tail_chunk:end_tag("test")
end

return rgt.node.test
