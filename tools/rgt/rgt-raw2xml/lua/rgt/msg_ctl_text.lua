---
-- Report generation tool - control message text parsing module.
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

local msg_ctl_text   = {}

local function read_quoted_string(text, pos)
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

local token_parse = {}

function token_parse.tin(prm, text, pos)
    prm.tin, pos = text:match("^%s*(%d+)()", pos)
    return pos
end

function token_parse.page(prm, text, pos)
    prm.page, pos = text:match("^%s*([^%s]+)()", pos)
    return pos
end

function token_parse.authors(prm, text, pos)
    prm.authors = {}

    while true do
        local mail, new_pos = text:match("^%s*mailto:([^%s]+)()", pos)
        if mail == nil then
            break
        end
        table.insert(prm.authors, mail)
        pos = new_pos
    end

    return pos
end

function token_parse.args(prm, text, pos)
    prm.args = {}

    while true do
        local name, value_pos, value
        name, value_pos = text:match("^%s*([^=]+)=()", pos)
        if name == nil then
            break
        end
        value, pos = prm.read_quoted_string(text, value_pos)
        table.insert(prm.args, {name, value})
    end

    return pos
end

local function parse_start(prm, text, pos)
    if prm.event == "package" or prm.event == "test" then
        -- Retrieve node name and objective
        prm.name, prm.objective, pos =
            text:match("^%s*([^%s]+)%s+\"([^\"]+)\"()", pos)
    end

    -- Parse the message body
    repeat
        local token

        token, pos = text:match("^%s*([^%s]+)()", pos)

        if token ~= nil then
            local parse_fn = token_parse[token:lower()]
            
            if parse_fn == nil then
                error("Unknown token \"" .. token .. "\"")
            end

            pos = parse_fn(prm, text, pos)
        end
    until token == nil
end

local function msg_ctl_text.parse(prm, text)
    local event
    local pos

    -- Parse the message header
    prm.parent_id, prm.id, event, pos =
            text:match("^(%d+)%s+(%d+)%s+(%w+)()$")

    event = event:lower()
    prm.event = event

    -- If it is a node start
    if event == "package" or event == "session" or event == "test" then
        parse_start(prm, text, pos)
    else
        prm.err = text:match("^%s*(.*)$")
    end

    return prm
end

return msg_ctl_text
