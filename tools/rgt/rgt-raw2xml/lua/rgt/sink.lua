---
-- Report generation tool - XML format sink module.
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

local oo            = require("loop.base")
local rgt           = {}
rgt.node            = {}
rgt.node.root       = require("rgt.node.root")
rgt.node.package    = require("rgt.node.package")
rgt.node.session    = require("rgt.node.session")
rgt.node.test       = require("rgt.node.test")
rgt.xml_sink        = oo.class({
                                root    = nil,  -- Root node
                                map     = nil,  -- ID->node map
                               })

function rgt.xml_sink:__init(file)
    local inst  = {}

    inst.root = rgt.node.root{id = 0, depth = 0}
    inst.map = {[0] = inst.root}

    return oo.rawnew(self, inst)
end

local function ctl_read_quoted_string(text, pos)
    local unquoted

    if text:sub(pos, pos + 1) ~= "\"" then
        unquoted, pos = match("([^%s]*)()")
    else
        local chunk, char, new_pos

        pos = pos + 1
        unquoted = ""

        repeat
            chunk, char, new_pos = text:match("^(.-)([\"\\])()", pos)
            unquoted = unquoted .. chunk
            pos = new_pos + 1
        until char == "\""
    end

    return unquoted, pos
end

local ctl_token_parse = {}

function ctl_token_parse.tin(ctl, text, pos)
    ctl.tin, pos = text:match("^%s*(%d+)()", pos)
    return pos
end

function ctl_token_parse.page(ctl, text, pos)
    ctl.page, pos = text:match("^%s*([^%s]+)()", pos)
    return pos
end

function ctl_token_parse.authors(ctl, text, pos)
    ctl.authors = {}

    while true do
        local mail, new_pos = text:match("^%s*mailto:([^%s]+)()", pos)
        if mail == nil then
            break
        end
        table.insert(ctl.authors, mail)
        pos = new_pos
    end

    return pos
end

function ctl_token_parse.args(ctl, text, pos)
    ctl.args = {}

    while true do
        local name, value_pos, value
        name, value_pos = text:match("^%s*([^=]+)=()", pos)
        if name == nil then
            break
        end
        value, pos = ctl_read_quoted_string(text, value_pos)
        table.insert(ctl.args, {name, value})
    end

    return pos
end

local function ctl_parse_start_text(ctl, text, pos)
    if ctl.event == "package" or ctl.event == "test" then
        -- Retrieve node name and objective
        ctl.name, ctl.objective, pos =
            text:match("^%s*([^%s]+)%s+\"([^\"]+)\"()", pos)
    end

    -- Parse the message body
    repeat
        local token

        token, pos = text:match("^%s*([^%s]+)()", pos)

        if token ~= nil then
            local parse_fn = ctl_token_parse[token:lower()]
            
            if parse_fn == nil then
                error("Unknown token \"" .. token .. "\"")
            end

            pos = parse_fn(ctl, text, pos)
        end
    until token == nil
end

local function ctl_parse_text(ctl, text)
    local event
    local pos

    -- Parse the message header
    ctl.parent_id, ctl.id, event, pos =
            text:match("^(%d+)%s+(%d+)%s+(%w+)()$")

    event = event:lower()
    ctl.event = event

    -- If it is a node start
    if event == "package" or event == "session" or event == "test" then
        ctl_parse_start_text(ctl, text, pos)
    else
        ctl.err = text:match("^%s*(.*)$")
    end
end


function rgt.xml_sink:put(msg)
    -- If it is not a control message
    if msg.id ~= 0 or msg.user ~= "Control" or msg.entity ~= "Tester" then
        local node = map[msg.id]

        node:log(msg.ts, msg.level, msg.entity, msg.user, msg.text)
    else
        local ctl = {}
        local parent, node

        ctl_parse_text(ctl, msg.text)

        -- lookup parent node
        parent = self.map[ctl.parent_id]

        -- If it is a node opening
        if ctl.event == "package" or
           ctl.event == "session" or
           ctl.event == "test" then
            -- create new node
            node = rgt.node[ctl.event]{
                    start       = msg.ts,
                    name        = ctl.name,
                    objective   = ctl.objective,
                    tin         = ctl.tin
                    page        = ctl.page,
                    authors     = ctl.authors,
                    args        = ctl.args,
                    start       = msg.ts}

            -- add to the map
            self.map[ctl.id] = node

            -- add to the parent's children list
            parent:add_child(ctl.id, node)
        else
            -- lookup node
            node = self.map[ctl.id]

            -- finish the node with the result
            node:finish(msg.ts, ctl.event, ctl.err)

            -- remove the node from the parent
            parent:del_child(ctl.id)

            -- remove the node from the map
            self.map[ctl.id] = nil
        end
    end
end

