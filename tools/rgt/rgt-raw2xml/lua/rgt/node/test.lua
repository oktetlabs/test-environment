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
rgt.node            = {}
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

    rgt.node.named:__init(inst)

    return oo.rawnew(self, inst)
end

function rgt.node.test:fill_attrs(attrs)
    assert(type(attrs) == "table")

    rgt.node.named.fill_attrs(self, attrs)

    attrs.tin = self.tin

    return attrs
end

function rgt.node.test:write_meta(chunk)
    assert(oo.instanceof(chunk, co.xml_chunk))

    rgt.node.named.write_meta(self, chunk)

    if self.page ~= nil then
        chunk:element("page", nil, self.page)
    end

    if #self.verdicts > 0 then
        chunk:start_tag("verdicts")
        for i, v in self.verdicts do
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
end

return rgt.node.test
