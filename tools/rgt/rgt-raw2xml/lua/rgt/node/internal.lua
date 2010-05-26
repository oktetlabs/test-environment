---
-- Report generation tool - internal node type.
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
local rgt           = {}
rgt.node            = {}
rgt.node.basic      = require("rgt.node.basic")
rgt.node.internal   = oo.class({
                                children    = nil   --- {id, node} array
                               }, rgt.node.basic)

function rgt.node.internal:__init(inst)
    assert(type(inst) == "table")

    rgt.node.basic:__init(inst)

    inst.children = {}

    return oo.rawnew(self, inst)
end


function rgt.node.internal:log(ts, level, entity, user, text)
    if self.children[1] ~= nil then
        self.children[1][2]:log(ts, level, entity, user, text)
    else
        rgt.node.basic.log(self, ts, level, entity, user, text)
    end
end

function rgt.node.internal:add_child(id, node)
    assert(id ~= nil)
    assert(oo.instanceof(node, rgt.node.basic))

    if self.logging then
        self:finish_logging()
    end

    table.insert(self.children, {id, node})
end

function rgt.node.internal:del_child(id)
    assert(id ~= nil)
    assert(self.children[1] ~= nil)

    for i, id_node in ipairs(self.children) do
        if id_node[1] == id then
            -- Remove the child
            table.remove(self.children, i)
            return
        end
    end

    assert(false, "Child ID " .. id .. " not found")
end

return rgt.node.internal
