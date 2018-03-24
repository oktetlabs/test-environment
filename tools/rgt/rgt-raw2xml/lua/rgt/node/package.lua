---
-- Report generation tool - package node type.
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
rgt.node.named      = require("rgt.node.named")
rgt.node.package    = oo.class({element = "pkg"}, rgt.node.named)

return rgt.node.package
