-- 测试查询隐含时间列的各种方法

-- 创建测试表
DROP TABLE IF EXISTS time_query_test CASCADE;

CREATE TABLE time_query_test (
    id SERIAL PRIMARY KEY,
    name VARCHAR(50),
    description TEXT
) IMPLICIT TIME;

-- 插入测试数据
INSERT INTO time_query_test (name, description) VALUES ('记录1', '第一条记录');
SELECT pg_sleep(1);
INSERT INTO time_query_test (name, description) VALUES ('记录2', '第二条记录');
SELECT pg_sleep(1);
UPDATE time_query_test SET description = '更新的第一条记录' WHERE name = '记录1';

-- 方法1: 尝试直接查询（预期失败）
SELECT '=== 方法1: 直接查询time列 ===' as test_method;
-- SELECT time FROM time_query_test; -- 这会失败

-- 方法2: 查看所有列（包括系统列）
SELECT '=== 方法2: 查看所有列信息 ===' as test_method;
SELECT attname, attnum, atttypid, attnotnull 
FROM pg_attribute 
WHERE attrelid = (SELECT oid FROM pg_class WHERE relname = 'time_query_test') 
  AND attnum > 0 
ORDER BY attnum;

-- 方法3: 检查隐含列配置
SELECT '=== 方法3: 隐含列配置 ===' as test_method;
SELECT ic.ic_attname, ic.ic_attnum, ic.ic_atttypid, ic.ic_visible
FROM pg_implicit_columns ic 
JOIN pg_class c ON ic.ic_relid = c.oid 
WHERE c.relname = 'time_query_test';

-- 方法4: 尝试使用特殊函数（如果存在）
SELECT '=== 方法4: 尝试特殊函数 ===' as test_method;
-- 这些函数可能存在于实现中
-- SELECT get_implicit_time_value(oid) FROM time_query_test; -- 可能不存在

-- 方法5: 查看表的完整结构
SELECT '=== 方法5: 表结构信息 ===' as test_method;
\d+ time_query_test

-- 方法6: 查看当前数据
SELECT '=== 方法6: 当前数据（SELECT *） ===' as test_method;
SELECT * FROM time_query_test ORDER BY id;

-- 清理
DROP TABLE time_query_test CASCADE;