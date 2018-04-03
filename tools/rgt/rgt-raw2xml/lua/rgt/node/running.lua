---
-- Report generation tool - running node type.
--
-- Copyright (C) 2003-2018 OKTET Labs.
-- 
-- @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
-- 
-- @release $Id$
--

local oo            = require("loop.simple")
local rgt           = {}
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
    branch.chunk:append_start_tag("branch")
    rgt.node.general.start_branch(self, branch)
    return branch
end

function rgt.node.running:finish_branch(branch)
    rgt.node.general.finish_branch(self, branch)
    branch.chunk:append_end_tag("branch")
    return branch
end

function rgt.node.running:add_attrs(attrs)
    table.insert(attrs, {"result", self.result:upper()})
    table.insert(attrs, {"err", self.err})

    return attrs
end

function rgt.node.running:write_meta(chunk)
    chunk:append_element("start-ts", nil, self.start_ts:format_short_abs()):
          append_element("end-ts", nil, self.end_ts:format_short_abs()):
          append_element("duration", nil,
                         (self.end_ts - self.start_ts):format_short_rel())

    return chunk
end

function rgt.node.running:finish(ts, result, err)
    self.end_ts = ts
    self.result = result
    self.err    = err

    self.head:append_start_tag(self.element, self:add_attrs({}))
    self.head:append_start_tag("meta")
    self:write_meta(self.head)
    -- FIXME MIMICKING ORIGINAL - move to write_meta
    if self.args ~= nil and #self.args > 0 then
        self.head:append_start_tag("params")
        for i, a in ipairs(self.args) do
            self.head:append_element("param",
                                     {{"name", a[1]}, {"value", a[2]}})
        end
        self.head:append_end_tag("params")
    end
    self.head:append_end_tag("meta")
    rgt.node.general.finish(self)
    self.tail:append_end_tag(self.element)
    return self
end

return rgt.node.running
