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
                                element     = nil,  --- XML element name
                                args        = nil,  --- {name, value} array
                                start_ts    = nil,  --- start timestamp
                                end_ts      = nil,  --- end timestamp
                                result      = nil,  --- result
                                err         = nil,  --- error message
                               }, rgt.node.general)

function rgt.node.running:__init(inst)
    return rgt.node.general.__init(self, inst)
end

function rgt.node.running:start_branch(branch)
    branch.chunk:start_tag("branch")
    rgt.node.general.start_branch(self, branch)
    return branch
end

function rgt.node.running:finish_branch(branch)
    rgt.node.general.finish_branch(self, branch)
    branch.chunk:end_tag("branch")
    return branch
end

function rgt.node.running:add_attrs(attrs)
    table.insert(attrs, {"result", self.result:upper()})
    table.insert(attrs, {"err", self.err})

    return attrs
end

function rgt.node.running:write_meta(chunk)
    chunk:element("start-ts", nil, self.start_ts:format_short_abs()):
          element("end-ts", nil, self.end_ts:format_short_abs()):
          element("duration", nil,
                  (self.end_ts - self.start_ts):format_short_rel())

    return chunk
end

function rgt.node.running:finish(ts, result, err)
    self.end_ts = ts
    self.result = result
    self.err    = err

    self.head:start_tag(self.element, self:add_attrs({}))
    self.head:start_tag("meta")
    self:write_meta(self.head)
    -- FIXME MIMICKING ORIGINAL - move to write_meta
    if self.args ~= nil and #self.args > 0 then
        self.head:start_tag("params")
        for i, a in ipairs(self.args) do
            self.head:element("param", {{"name", a[1]}, {"value", a[2]}})
        end
        self.head:end_tag("params")
    end
    self.head:end_tag("meta")
    rgt.node.general.finish(self)
    self.tail:end_tag(self.element)
    return self
end

return rgt.node.running
