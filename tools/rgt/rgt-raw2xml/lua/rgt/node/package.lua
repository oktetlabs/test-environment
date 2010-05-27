---
-- Report generation tool - package node type.
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

local oo            = require("loop.multi")
local rgt           = {}
rgt.node            = {}
rgt.node.branching  = require("rgt.node.branching")
rgt.node.package    = oo.class({
                               },
                               rgt.node.branching,
                               rgt.node.named,
                               rgt.node.parametrized,
                               rgt.node.spanning)

function rgt.node.package:__init(inst)
    rgt.node.branching:__init(inst)
    rgt.node.named:__init(inst)
    rgt.node.parametrized:__init(inst)
    rgt.node.spanning:__init(inst)

    return oo.rawnew(self, inst)
end

function rgt.node.package:finish(ts, result, err)
    rgt.node.spanning:finish(self, ts, result, err)
    rgt.node.branching:finish(self)

    self.head_chunk:start_tag("pkg",
                              {name = self.name,
                               result = self.result,
                               err = self.err}):
                    start_tag("meta"):
                    element("start-ts", nil, self.start:format_short_abs()):
                    element("end-ts", nil, self["end"]:format_short_abs()):
                    element("duration", nil,
                            (self["end"] - self.start):format_short_rel()):
                    element("objective", nil, self.objective)

    if #self.authors > 0 then
        self.head_chunk:start_tag("authors")
        for i, a in self.authors do
            self.head_chunk:element("author", {email = a})
        end
        self.head_chunk:end_tag("authors"):
    end

    if #self.args > 0 then
        self.head_chunk:start_tag("params")
        for i, a in self.args do
            self.head_chunk:element("param", {name = a[1], value = a[2]})
        end
        self.head_chunk:end_tag("params")
    end

    self.head_chunk:end_tag("meta")
    self.tail_chunk:end_tag("pkg")
end

return rgt.node.package
