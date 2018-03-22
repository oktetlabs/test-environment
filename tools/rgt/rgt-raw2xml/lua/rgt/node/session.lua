---
-- Report generation tool - session node type.
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
rgt.node.session    = oo.class({element = "session"}, rgt.node.running)

return rgt.node.session
