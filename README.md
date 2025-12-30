# PostgreSQL-mod

基于 PostgreSQL 15.15 的修改版本，新增了**隐含时间列**功能。

## 项目简介

本项目是对 PostgreSQL 数据库管理系统的扩展，实现了隐含时间列（Implicit Time Column）功能。该功能允许表自动记录每行数据的时间戳，无需手动定义时间字段，适用于需要自动追踪数据变更时间的场景。

## 测试

项目包含多个测试 SQL 文件：

- `test_insert.sql` - 测试插入操作
- `test_update.sql` - 测试更新操作
- `test_batch_transaction.sql` - 测试批量和事务操作
- `test_implicit_time_verification.sql` - 验证隐含时间列功能
- `test_query_implicit_time.sql` - 测试隐含时间列查询


## 许可证

本项目基于 PostgreSQL 许可证，遵循原 PostgreSQL 的版权和许可条款。

## 贡献

欢迎提交问题和改进建议。

## 相关文档

- PostgreSQL 官方文档：https://www.postgresql.org/docs/
- 项目技术报告：参考项目目录中的 `技术报告.pdf`