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

function rgt.node.test:finish_head()
    local d = self.depth
    local c = self.head_chunk

    c:write((" "):rep(d))
    c:write(("<test name=\"%s\" result=\"%s\" err=\"%s\">\n"):
                format(self.name, self.result, self.err))
    d = d + 1
    c:write((" "):rep(d))
    c:write("<meta>\n")
    d = d + 1
    c:write((" "):rep(d))
    c:write(("<start-ts>%s</start-ts>\n"):format(self.start:format_short_abs()))
    c:write((" "):rep(d))
    c:write(("<end-ts>%s</end-ts>\n"):format(self["end"]:format_short_abs()))
    c:write((" "):rep(d))
    c:write(("<duration>%s</duration>\n"):
                format((self["end"] - self.start):format_short_rel()))
    c:finish()
end

function rgt.node.test:finish(ts, result, err)
    local head_chunk
    local tail_chunk

    assert(self.chunk ~= nil)

    rgt.node.spanning.finish(self, ts, result, err)
    rgt.node.basic.finish(self):finish()

    self:finish_head()
    self.head_chunk = nil

    tail_chunk = self.tail_chunk
    self.tail_chunk = nil
    tail_chunk:write((" "):rep(self.depth))
    tail_chunk:write("</test>\n")

    return tail_chunk
return rgt.node.test
