---
-- Report generation tool - sink module.
--
-- Copyright (C) 2003-2018 OKTET Labs.
-- 
-- @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
-- 
-- @release $Id$
--

local oo            = require("loop.simple")
local rgt           = {}
rgt.co              = require("rgt.co")
rgt.node            = {}
rgt.node.root       = require("rgt.node.root")
rgt.node.package    = require("rgt.node.package")
rgt.node.session    = require("rgt.node.session")
rgt.node.test       = require("rgt.node.test")
rgt.sink            = oo.class({
                                mngr    = nil,  --- The output chunk manager
                                chunk   = nil,  --- The output chunk
                                map     = nil,  --- ID->node map
                               })

function rgt.sink:__init(tmp_dir, max_mem)
    return oo.rawnew(self, {mngr = rgt.co.mngr(tmp_dir, max_mem), map = {}})
end

function rgt.sink:take_file(file)
    self.chunk = self.mngr:take_file(file)
end

function rgt.sink:start()
    self.map[0] = rgt.node.root({}):take_chunk(self.chunk):start()
    self.chunk = nil
end

function rgt.sink:put(msg)
    local prm, event
    local parent, node

    -- If it is not a control message
    if not msg:is_tester_control() then
        -- log the message to its node
        node = self.map[msg:get_id()]
        if node == nil then
            error(("Node ID %u not found"):format(msg.get_id()))
        end
        node:log(msg)
        return
    end

    -- parse control message text
    prm = msg:parse_tester_control()

    --[[
    io.stderr:write("CONTROL PARAMETERS:\n")
    for k, v in pairs(prm) do
        io.stderr:write(k .. "=")
        if type(v) == "table" then
            io.stderr:write("{")
            for vk, vv in pairs(v) do
                io.stderr:write(vk .. "=")
                if type(vv) == "table" then
                    io.stderr:write("{")
                    for vvk, vvv in pairs(vv) do
                        io.stderr:write(vvk .. "=\"" .. vvv .. "\", ")
                    end
                    io.stderr:write("}")
                else
                    io.stderr:write("\"" .. vv .. "\"")
                end
                io.stderr:write(", ")
            end
            io.stderr:write("}")
        else
            io.stderr:write("\"" .. v .. "\"")
        end
        io.stderr:write("\n")
    end
    --]]

    -- lookup parent node
    parent = self.map[prm.parent_id]
    if parent == nil then
        error(("Node ID %u not found"):format(prm.parent_id))
    end

    event = prm.event:lower()

    -- If it is a node opening
    if event == "package" or
       event == "session" or
       event == "test" then
        -- create new node
        node = rgt.node[event]({
                start_ts    = msg:get_ts(),
                name        = prm.name,
                objective   = prm.objective,
                tin         = prm.tin,
                page        = prm.page,
                authors     = prm.authors,
                args        = prm.args})

        -- add to the map
        self.map[prm.id] = node

        -- add to the parent's children list
        parent:add_child(node)

        -- start node output
        node:start()
    else
        -- lookup node
        node = self.map[prm.id]
        if node == nil then
            error(("Node ID %u not found"):format(prm.id))
        end

        -- finish the node with the result
        node:finish(msg:get_ts(), prm.event, prm.err)

        -- remove the node from the parent
        parent:del_child(node)

        -- remove the node from the map
        self.map[prm.id] = nil
    end
end

function rgt.sink:finish()
    -- Finish the root node and take its chunk
    self.chunk = self.map[0]:finish():yield_chunk()
    -- Remove the root node
    self.map[0] = nil
end

function rgt.sink:yield_file()
    -- Finish and remove the chunk
    self.chunk:finish()
    self.chunk = nil

    -- Take the file and size from the manager
    return self.mngr:yield_file()
end

return rgt.sink
