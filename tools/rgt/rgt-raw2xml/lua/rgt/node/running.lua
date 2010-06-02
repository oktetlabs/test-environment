---
-- Report generation tool - running node type.
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
rgt.ts              = require("rgt.ts")
rgt.node            = {}
rgt.node.general    = require("rgt.node.general")
rgt.node.running    = oo.class({
                                element = nil,  --- XML element name
                                args    = nil,  --- {name, value} array
                                start   = nil,  --- start timestamp
                                ["end"] = nil,  --- end timestamp
                                result  = nil,  --- result
                                err     = nil,  --- error message
                               }, rgt.node.general)

function rgt.node.running:__init(inst)
    assert(type(self.element) == "string")
    assert(type(inst) == "table")
    assert(type(inst.args) == "table")
    assert(oo.instanceof(inst.start, rgt.ts))
    assert(inst["end"] == nil)
    assert(inst.result == nil)
    assert(inst.err == nil)

    rgt.node.general:__init(inst)

    return oo.rawnew(self, inst)
end

function rgt.node.running:start_branch(branch)
    branch.chunk:start_tag("branch")
    rgt.node.general.start_branch(branch)
    return branch
end

function rgt.node.running:finish_branch(branch)
    rgt.node.general.finish_branch(branch)
    branch.chunk:end_tag("branch")
    return branch
end

function rgt.node.running:fill_attrs(attrs)
    assert(type(attrs) == "table")

    attrs.result    = self.result
    attrs.err       = self.err

    return attrs
end

function rgt.node.running:write_meta(chunk)
    assert(oo.instanceof(chunk, co.xml_chunk))

    chunk:element("start-ts", nil, self.start:format_short_abs()):
          element("end-ts", nil, self["end"]:format_short_abs()):
          element("duration", nil,
                  (self["end"] - self.start):format_short_rel())

    if #self.args > 0 then
        chunk:start_tag("params")
        for i, a in self.args do
            chunk:element("param", {name = a[1], value = a[2]})
        end
        chunk:end_tag("params")
    end

    return chunk
end

function rgt.node.running:finish(ts, result, err)
    assert(oo.instanceof(ts, rgt.ts))
    assert(ts >= self.start)
    assert(type(result) == "string")
    assert(type(err) == "string")

    self["end"] = ts
    self.result = result
    self.err    = err

    self.head:start_tag(self.element, self:fill_attrs({}))
    self.head:start_tag("meta")
    self:write_meta(self.head)
    self.head:end_tag("meta")
    self.tail:end_tag(self.element)
    return self
end

return rgt.node.running
