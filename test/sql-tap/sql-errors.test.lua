#!/usr/bin/env tarantool
test = require("sqltester")
test:plan(16)

-- This suite is aimed to test SQL errors.

-- Errors during preparing.

test:do_catchsql_test(
    "sql-errors-1.1",
    [[
        SELECT * FROM t;
    ]], {
        -- <sql-errors-1.1>
        1, "Table 'T' does not exist"
        -- </sql-errors-1.1>
})

test:do_catchsql_test(
    "sql-errors-1.2",
    [[
        CREATE TABLE t (id INT PRIMARY KEY);
        DROP INDEX i ON t;
        DROP TABLE t;
    ]], {
        -- <sql-errors-1.2>
        1, "No index 'I' is defined in table 'T'"
        -- </sql-errors-1.2>
})

test:do_catchsql_test(
    "sql-errors-1.3",
    [[
        DROP VIEW v;
    ]], {
        -- <sql-errors-1.3>
        1, "View 'V' does not exist"
        -- </sql-errors-1.3>
})

test:do_catchsql_test(
    "sql-errors-1.4",
    [[
        a1 b2 c3;
    ]], {
        -- <sql-errors-1.4>
        1, "Near 'a1': syntax error"
        -- </sql-errors-1.4>
})

test:do_test(
    "sql-errors-1.5",
    function()
        r = {pcall(box.sql.execute, '')}
        if r[1] == false then
            r[1] = 1
        end
        return r
    end, {
        -- <sql-errors-1.5>
        1,"Syntax error: empty request"
        -- </sql-errors-1.5>
})

-- Length of wrong_name more than 65000.
wrong_name = 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
for i = 1,10 do wrong_name = wrong_name .. wrong_name end

test:do_catchsql_test(
    "sql-errors-1.6",
    "CREATE TABLE " .. wrong_name .. " (id INT PRIMARY KEY);",
    {
        -- <sql-errors-1.6>
        1, "Invalid identifier 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' (identifier name is too long)"
        -- </sql-errors-1.6>
})

test:do_catchsql_test(
    "sql-errors-1.7",
    [[
        DROP TABLE IF EXISTS t;
        CREATE TABLE t (id INT PRIMARY KEY);
        CREATE TABLE t (id INT PRIMARY KEY);
        DROP TABLE t;
    ]], {
        -- <sql-errors-1.7>
        1, "Table 'T' already exists"
        -- </sql-errors-1.7>
})

-- More than 2000 columns in new table.
create_statement = [[
    DROP TABLE IF EXISTS t;
    CREATE TABLE t (s0 INT PRIMARY KEY
]]
for i = 1,2000 do create_statement = create_statement .. ', s' .. i .. ' INT' end
create_statement = create_statement .. ')'

test:do_catchsql_test(
    "sql-errors-1.8",
    create_statement,
    {
        -- <sql-errors-1.8>
        1, "Can't create table 'T': too many columns"
        -- </sql-errors-1.8>
})

test:do_catchsql_test(
    "sql-errors-1.9",
    [[
        DROP TABLE IF EXISTS t;
        CREATE TABLE t (id INT PRIMARY KEY, a INT, a TEXT);
    ]], {
        -- <sql-errors-1.9>
        1, "Duplicate column name: 'A'"
        -- </sql-errors-1.9>
})

test:do_catchsql_test(
    "sql-errors-1.10",
    [[
        DROP TABLE IF EXISTS t;
        CREATE TABLE t (id INT PRIMARY KEY, a TEXT DEFAULT(id));
    ]], {
        -- <sql-errors-1.10>
        1, "Default value of column 'A' is not constant"
        -- </sql-errors-1.10>
})

test:do_catchsql_test(
    "sql-errors-1.11",
    [[
        DROP TABLE IF EXISTS t;
        CREATE TABLE t (id INT PRIMARY KEY, a INT PRIMARY KEY);
    ]], {
        -- <sql-errors-1.11>
        1, "Table 'T' has more than one primary key"
        -- </sql-errors-1.11>
})

test:do_catchsql_test(
    "sql-errors-1.12",
    [[
        DROP TABLE IF EXISTS t;
        CREATE TABLE t (id INT PRIMARY KEY, a INT PRIMARY KEY);
    ]], {
        -- <sql-errors-1.12>
        1, "Table 'T' has more than one primary key"
        -- </sql-errors-1.12>
})

test:do_catchsql_test(
    "sql-errors-1.13",
    [[
        DROP TABLE IF EXISTS t;
        CREATE TABLE t (id INT, a INT, PRIMARY KEY('ID'));
    ]], {
        -- <sql-errors-1.13>
        1, "Expressions prohibited in PRIMARY KEY"
        -- </sql-errors-1.13>
})

test:do_catchsql_test(
    "sql-errors-1.14",
    [[
        DROP TABLE IF EXISTS t;
        CREATE TABLE t (id TEXT PRIMARY KEY AUTOINCREMENT);
    ]], {
        -- <sql-errors-1.14>
        1, "AUTOINCREMENT is only allowed on an INTEGER PRIMARY KEY or INT PRIMARY KEY"
        -- </sql-errors-1.14>
})

test:do_catchsql_test(
    "sql-errors-1.15",
    [[
        DROP TABLE IF EXISTS t;
        CREATE TABLE t (id INT PRIMARY KEY);
        DROP VIEW t;
    ]], {
        -- <sql-errors-1.15>
        1, "Use DROP TABLE to delete table 'T'"
        -- </sql-errors-1.15>
})

test:do_catchsql_test(
    "sql-errors-1.16",
    [[
        DROP TABLE IF EXISTS t;
        CREATE TABLE t (id INT PRIMARY KEY);
        CREATE VIEW v as select * from t;
        DROP TABLE v;
    ]], {
        -- <sql-errors-1.16>
        1, "Use DROP VIEW to delete view 'V'"
        -- </sql-errors-1.16>
})

test:execsql('DROP VIEW v')

test:finish_test()
