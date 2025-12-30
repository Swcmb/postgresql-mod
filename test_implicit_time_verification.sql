-- 验证隐含时间列功能
-- 创建新的测试表
DROP TABLE IF EXISTS time_test CASCADE;

CREATE TABLE time_test (
    id SERIAL PRIMARY KEY,
    name VARCHAR(50),
    value INTEGER
) IMPLICIT TIME;

-- 插入第一条记录
INSERT INTO time_test (name, value) VALUES ('测试1', 100);

-- 等待1秒（通过pg_sleep）
SELECT pg_sleep(1);

-- 插入第二条记录
INSERT INTO time_test (name, value) VALUES ('测试2', 200);

-- 等待1秒
SELECT pg_sleep(1);

-- 更新第一条记录
UPDATE time_test SET value = 150 WHERE name = '测试1';

-- 查看记录（SELECT * 不显示隐含列，这是正确行为）
SELECT * FROM time_test ORDER BY id;

-- 检查隐含列配置
SELECT 'time_test表的隐含列配置:' as info;
SELECT ic.ic_attname, ic.ic_attnum, ic.ic_visible 
FROM pg_implicit_columns ic 
JOIN pg_class c ON ic.ic_relid = c.oid 
WHERE c.relname = 'time_test';

-- 清理
DROP TABLE time_test CASCADE;