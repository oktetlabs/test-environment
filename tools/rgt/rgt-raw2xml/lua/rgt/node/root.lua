---
-- Report generation tool - root node type.
--
-- Copyright (C) 2003-2018 OKTET Labs.
-- 
-- @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
-- 
-- @release $Id$
--

local oo            = require("loop.simple")
local rgt           = {}
rgt.msg             = require("rgt.msg")
rgt.node            = {}
rgt.node.general    = require("rgt.node.general")
rgt.node.root       = oo.class({}, rgt.node.general)

function rgt.node.root:start()
    self.head:append("<?xml version=\"1.0\"?>\n")
    self.head:append_start_tag("proteos:log_report",
                               {{"xmlns:proteos",
                                 "http://www.oktetlabs.ru/proteos"}})
    self.head:finish()
    self.head = nil
    return rgt.node.general.start(self)
end

function rgt.node.root:add_child(child)
    -- One branch at most
    assert(self.branches[1].child == nil)
    rgt.node.general.add_child(self, child)
    return self
end

function rgt.node.root:finish()
    rgt.node.general.finish(self)
    self.tail:append_end_tag("proteos:log_report")
    return self
end

return rgt.node.root
