---
-- Report generation tool - named node type.
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
rgt.node.running    = require("rgt.node.running")
rgt.node.named      = oo.class({
                                name        = nil,  --- Name string
                                objective   = nil,  --- Objective string
                                authors     = nil,  --- Author e-mail array
                               }, rgt.node.running)

function rgt.node.named:__init(inst)
    assert(type(inst) == "table")
    assert(type(inst.name) == "string")
    assert(type(inst.objective) == "string")
    assert(type(inst.authors) == "table")

    rgt.node.general:__init(inst)

    return oo.rawnew(self, inst)
end

function rgt.node.named:fill_attrs(attrs)
    assert(type(attrs) == "table")
    rgt.node.running.fill_attrs(self, attrs)
    attrs.name = self.name
    return attrs
end

function rgt.node.named:write_meta(chunk)
    assert(oo.instanceof(chunk, co.xml_chunk))

    rgt.node.running.write_meta(self, chunk)

    chunk:element("objective", nil, self.objective)

    if #self.authors > 0 then
        chunk:start_tag("authors")
        for i, e in self.authors do
            chunk:element("author", {email = e})
        end
        chunk:end_tag("authors")
    end

    return chunk
end

return rgt.node.named
