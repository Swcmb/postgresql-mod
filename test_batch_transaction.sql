-- 测试批量插入操作
INSERT INTO users (name, email, age) VALUES 
    ('用户1', 'user1@example.com', 20),
    ('用户2', 'user2@example.com', 21),
    ('用户3', 'user3@example.com', 22);

-- 查看批量插入结果
SELECT COUNT(*) as total_users FROM users;

-- 测试批量更新
UPDATE users SET age = age + 1 WHERE age < 25;

-- 查看批量更新结果
SELECT * FROM users ORDER BY id;

-- 测试事务提交
BEGIN;
INSERT INTO users (name, email, age) VALUES ('事务用户', 'tx@example.com', 25);
UPDATE users SET age = 26 WHERE name = '事务用户';
COMMIT;

-- 验证事务提交结果
SELECT COUNT(*) as committed_users FROM users WHERE name = '事务用户';

-- 测试事务回滚
SELECT COUNT(*) as before_rollback FROM users;

BEGIN;
INSERT INTO users (name, email, age) VALUES ('回滚用户', 'rollback@example.com', 30);
ROLLBACK;

SELECT COUNT(*) as after_rollback FROM users;