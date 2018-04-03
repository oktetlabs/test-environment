---
-- Report generation tool - named node type.
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
rgt.node.running    = require("rgt.node.running")
rgt.node.named      = oo.class({
                                name        = nil,  --- Name string
                                objective   = nil,  --- Objective string
                                authors     = nil,  --- Author e-mail array
                               }, rgt.node.running)

function rgt.node.named:__init(inst)
    return rgt.node.running.__init(self, inst)
end

function rgt.node.named:add_attrs(attrs)
    table.insert(attrs, {"name", self.name})
    rgt.node.running.add_attrs(self, attrs)
    return attrs
end

function rgt.node.named:write_meta(chunk)
    rgt.node.running.write_meta(self, chunk)

    if #self.objective > 0 then
        chunk:append_element("objective", nil, self.objective)
    end

    if self.authors ~= nil and #self.authors > 0 then
        chunk:append_start_tag("authors")
        for i, e in ipairs(self.authors) do
            chunk:append_element("author", {{"email", e}})
        end
        chunk:append_end_tag("authors")
    end

    return chunk
end

return rgt.node.named
