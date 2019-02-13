test_run = require('test_run').new()
engine = test_run:get_cfg('engine')
box.sql.execute('pragma sql_default_engine=\''..engine..'\'')

box.cfg{}

box.sql.execute("CREATE TABLE t1(a INT PRIMARY KEY, b INT UNIQUE)")
box.sql.execute("CREATE TABLE t2(a INT PRIMARY KEY, b INT REFERENCES t1(b) ON UPDATE CASCADE ON UPDATE CASCADE)")
box.sql.execute("CREATE TABLE t2(a INT PRIMARY KEY, b INT REFERENCES t1(b) ON DELETE CASCADE ON DELETE CASCADE)")
box.sql.execute("CREATE TABLE t2(a INT PRIMARY KEY, b INT REFERENCES t1(b) ON DELETE CASCADE ON UPDATE CASCADE)")

box.sql.execute("DROP TABLE t2")
box.sql.execute("DROP TABLE t1")
