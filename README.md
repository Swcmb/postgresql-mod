# PostgreSQL-mod

基于 PostgreSQL 15.15 的修改版本，新增了**隐含时间列**功能。

## 项目简介

本项目是对 PostgreSQL 数据库管理系统的扩展，实现了隐含时间列（Implicit Time Column）功能。该功能允许表自动记录每行数据的时间戳，无需手动定义时间字段，适用于需要自动追踪数据变更时间的场景。

## 主要特性

### 1. 隐含时间列

- **自动时间戳**：在创建表时使用 `IMPLICIT TIME` 选项，系统会自动为表添加隐含的时间列
- **自动更新**：插入和更新操作时，时间戳会自动更新为当前时间
- **透明存储**：隐含列存储在元数据中，对普通查询不可见

### 2. SQL 语法扩展

```sql
-- 创建带隐含时间列的表
CREATE TABLE table_name (
    id SERIAL PRIMARY KEY,
    name VARCHAR(50),
    value INTEGER
) IMPLICIT TIME;

-- 为现有表添加隐含时间列
ALTER TABLE table_name ADD IMPLICIT TIME;

-- 删除隐含时间列
ALTER TABLE table_name DROP IMPLICIT TIME;
```

### 3. 系统视图

新增系统视图 `pg_implicit_columns_view` 用于查看隐含列配置：

```sql
SELECT * FROM pg_implicit_columns_view;
```

### 4. 向后兼容性

- 支持向后兼容性检查
- 非表类型（如视图、索引等）不支持隐含时间列
- 分区表使用隐含时间列时会有性能警告

## 核心实现

### 修改的文件

- `src/backend/access/common/heaptuple.c` - 添加隐含时间列的元组处理函数
- `src/backend/catalog/pg_implicit_columns.c` - 隐含列元数据管理
- `src/backend/catalog/pg_implicit_compat.c` - 向后兼容性检查
- `src/backend/commands/tablecmds.c` - ALTER TABLE 命令扩展
- `src/backend/catalog/system_views.sql` - 系统视图定义
- `src/include/catalog/pg_implicit_columns.h` - 头文件定义

### 新增函数

- `heap_form_tuple_with_implicit()` - 创建带隐含时间列的元组
- `heap_update_implicit_time()` - 更新隐含时间列
- `extract_implicit_time()` - 提取隐含时间列值
- `add_implicit_time_column()` - 添加隐含时间列
- `table_has_implicit_time()` - 检查表是否有隐含时间列

## 测试

项目包含多个测试 SQL 文件：

- `test_insert.sql` - 测试插入操作
- `test_update.sql` - 测试更新操作
- `test_batch_transaction.sql` - 测试批量和事务操作
- `test_implicit_time_verification.sql` - 验证隐含时间列功能
- `test_query_implicit_time.sql` - 测试隐含时间列查询

### 运行测试

```bash
psql -f test_insert.sql
psql -f test_update.sql
psql -f test_batch_transaction.sql
psql -f test_implicit_time_verification.sql
psql -f test_query_implicit_time.sql
```

## 安装

### 前置要求

- PostgreSQL 15.15 源代码
- C 编译器（GCC 或 Clang）
- Make 工具
- Flex 和 Bison

### 编译步骤

```bash
# 配置
./configure

# 编译
make

# 安装
make install

# 初始化数据库
initdb -D /path/to/data

# 启动服务
pg_ctl -D /path/to/data start
```

## 使用示例

### 创建带隐含时间列的表

```sql
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(50),
    email VARCHAR(100),
    age INTEGER
) IMPLICIT TIME;

-- 插入数据（自动记录时间）
INSERT INTO users (name, email, age) VALUES ('Alice', 'alice@example.com', 25);

-- 更新数据（自动更新时间）
UPDATE users SET age = 26 WHERE name = 'Alice';
```

### 查看隐含列配置

```sql
SELECT ic.ic_relid, c.relname, ic.ic_attname, ic.ic_visible
FROM pg_implicit_columns ic
JOIN pg_class c ON ic.ic_relid = c.oid;
```

### 为现有表添加隐含时间列

```sql
ALTER TABLE existing_table ADD IMPLICIT TIME;
```

## 限制

1. 隐含时间列仅支持普通表（RELKIND_RELATION）
2. 分区表使用隐含时间列可能影响性能
3. 隐含时间列对普通 SELECT * 查询不可见
4. 需要向后兼容性检查才能添加隐含时间列

## 技术细节

### 元组结构

隐含时间列存储在元组的末尾，经过内存对齐处理：

```
| 元组头 | 用户数据 | 对齐填充 | 隐含时间列 |
```

### 时间戳类型

使用 PostgreSQL 原生的 `TimestampTz` 类型存储时间戳，确保时区安全。

## 许可证

本项目基于 PostgreSQL 许可证，遵循原 PostgreSQL 的版权和许可条款。

## 贡献

欢迎提交问题和改进建议。

## 相关文档

- PostgreSQL 官方文档：https://www.postgresql.org/docs/
- 项目技术报告：参考项目目录中的 `技术报告.pdf`