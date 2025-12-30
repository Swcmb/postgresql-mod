-- 测试UPDATE操作的时间戳自动更新
UPDATE users SET age = 26 WHERE name = 'Alice';

-- 查询更新后的数据
SELECT * FROM users;

-- 测试DELETE操作
DELETE FROM users WHERE name = 'Bob';

-- 查询删除后的数据
SELECT * FROM users;