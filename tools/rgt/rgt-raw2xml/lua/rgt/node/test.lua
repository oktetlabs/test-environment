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

local oo            = require("loop.simple")
local co            = {}
co.xml_chunk        = require("co.xml_chunk")
local rgt           = {}
rgt.msg_fmt_str     = require("rgt.msg_fmt_str")
rgt.node            = {}
rgt.node.general    = require("rgt.node.general")
rgt.node.named      = require("rgt.node.named")
rgt.node.test       = oo.class({
                                element     = "test",
                                tin         = nil,      -- Test ID number
                                page        = nil,      -- Page string
                                verdicts    = nil,      -- Verdict array
                               },
                               rgt.node.named)

function rgt.node.test:__init(inst)
    assert(type(inst) == "table")
    assert(inst.tin == nil or
           type(inst.tin) == "number" and
           math.floor(inst.tin) == inst.tin and
           inst.tin >= 0)
    assert(inst.page == nil or type(inst.page) == "string")
    assert(inst.verdicts == nil)

    inst.verdicts = {}

    return rgt.node.named.__init(self, inst)
end

function rgt.node.test:start_branch(branch)
    assert(type(branch) == "table")
    return rgt.node.general.start_branch(self, branch)
end

function rgt.node.test:finish_branch(branch)
    assert(type(branch) == "table")
    return rgt.node.general.finish_branch(self, branch)
end

function rgt.node.test:add_attrs(attrs)
    assert(type(attrs) == "table")
    if self.tin ~= nil then
        table.insert(attrs, {"tin", tostring(self.tin)})
    end
    rgt.node.named.add_attrs(self, attrs)
    return attrs
end

function rgt.node.test:write_meta(chunk)
    assert(oo.instanceof(chunk, co.xml_chunk))

    rgt.node.named.write_meta(self, chunk)

    if self.page ~= nil then
        chunk:element("page", nil, self.page)
    end

    if self.verdicts ~= nil and #self.verdicts > 0 then
        chunk:start_tag("verdicts")
        for i, v in ipairs(self.verdicts) do
            chunk:element("verdict", nil, v)
        end
        chunk:end_tag("verdicts")
    end

    return chunk
end

function rgt.node.test:add_child(child)
    assert(oo.instanceof(child, rgt.node.general))
    -- No children allowed
    assert(false)
    return self
end

function rgt.node.test:log(msg)
    if msg.user == "Control" then
        table.insert(self.verdicts, rgt.msg_fmt_str(msg.fmt, msg.args))
    end
    rgt.node.named.log(self, msg)
end

return rgt.node.test
