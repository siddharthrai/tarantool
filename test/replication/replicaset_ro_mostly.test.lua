-- gh-3257 check bootstrap with read-only replica in cluster.
-- Old behaviour: failed, since read-only is chosen by uuid.
test_run = require('test_run').new()

SERVERS = {'replica_uuid_ro1', 'replica_uuid_ro2'}

uuid = require('uuid')
uuid1 = uuid.new()
uuid2 = uuid.new()
function sort_cmp(a, b) return a.time_low > b.time_low and true or false end
function sort(t) table.sort(t, sort_cmp) return t end
UUID = sort({uuid1, uuid2}, sort_cmp)

create_cluster_cmd1 = 'create server %s with script="replication/%s.lua"'
create_cluster_cmd2 = 'start server %s with args="%s %s", wait_load=False, wait=False'

test_run:cmd("setopt delimiter ';'")
function create_cluster_uuid(servers, uuids)
    for i, name in ipairs(servers) do
        test_run:cmd(create_cluster_cmd1:format(name, name))
    end
    for i, name in ipairs(servers) do
        test_run:cmd(create_cluster_cmd2:format(name, uuids[i], "0.1"))
    end
end;
test_run:cmd("setopt delimiter ''");

-- Deploy a cluster.
create_cluster_uuid(SERVERS, UUID)
test_run:wait_fullmesh(SERVERS)

-- Add third replica
name = 'replica_uuid_ro3'
test_run:cmd(create_cluster_cmd1:format(name, name))
test_run:cmd(create_cluster_cmd2:format(name, uuid.new(), "0.1"))
test_run:cmd('switch replica_uuid_ro3')
test_run:cmd('switch default')

-- Cleanup.
test_run:drop_cluster(SERVERS)
