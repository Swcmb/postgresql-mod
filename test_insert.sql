-- 测试隐含时间列功能
INSERT INTO users (name, email, age) VALUES ('Alice', 'alice@example.com', 25);
INSERT INTO users (name, email, age) VALUES ('Bob', 'bob@example.com', 30);
INSERT INTO logs (message) VALUES ('系统启动');
INSERT INTO logs (message) VALUES ('用户登录');

-- 查询数据
SELECT * FROM users;
SELECT * FROM logs;