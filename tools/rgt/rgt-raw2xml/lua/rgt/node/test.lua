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
    inst.verdicts = {}
    return rgt.node.named.__init(self, inst)
end

function rgt.node.test:start_branch(branch)
    return rgt.node.general.start_branch(self, branch)
end

function rgt.node.test:finish_branch(branch)
    return rgt.node.general.finish_branch(self, branch)
end

function rgt.node.test:add_attrs(attrs)
    if self.tin ~= nil then
        table.insert(attrs, {"tin", tostring(self.tin)})
    end
    rgt.node.named.add_attrs(self, attrs)
    return attrs
end

function rgt.node.test:write_meta(chunk)
    rgt.node.named.write_meta(self, chunk)

    if self.page ~= nil then
        chunk:append_element("page", nil, self.page)
    end

    if self.verdicts ~= nil and #self.verdicts > 0 then
        chunk:append_start_tag("verdicts")
        for i, v in ipairs(self.verdicts) do
            chunk:append_element("verdict", nil, v)
        end
        chunk:append_end_tag("verdicts")
    end

    return chunk
end

function rgt.node.test:add_child(child)
    -- No children allowed
    assert(false)
    return self
end

function rgt.node.test:log(msg)
    if msg:is_control() then
        table.insert(self.verdicts, msg:get_text())
    end
    rgt.node.named.log(self, msg)
end

return rgt.node.test
