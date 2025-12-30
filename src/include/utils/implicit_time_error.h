/*-------------------------------------------------------------------------
 *
 * implicit_time_error.h
 *    隐含时间列功能的错误处理定义
 *
 * 本文件定义了隐含时间列功能相关的错误代码、错误消息和错误处理函数。
 * 提供统一的错误处理接口，确保错误信息的一致性和可读性。
 *
 * Copyright (c) 2024, PostgreSQL Global Development Group
 *
 * src/include/utils/implicit_time_error.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef IMPLICIT_TIME_ERROR_H
#define IMPLICIT_TIME_ERROR_H

#include "utils/elog.h"

/*
 * 隐含时间列相关的错误代码
 * 使用现有的PostgreSQL错误代码体系
 */

/* DDL语法错误 */
#define IMPLICIT_TIME_SYNTAX_ERROR ERRCODE_SYNTAX_ERROR

/* 功能不支持错误 */
#define IMPLICIT_TIME_FEATURE_NOT_SUPPORTED ERRCODE_FEATURE_NOT_SUPPORTED

/* 内部错误 */
#define IMPLICIT_TIME_INTERNAL_ERROR ERRCODE_INTERNAL_ERROR

/* 数据损坏错误 */
#define IMPLICIT_TIME_DATA_CORRUPTED ERRCODE_DATA_CORRUPTED

/* 磁盘空间不足错误 */
#define IMPLICIT_TIME_DISK_FULL ERRCODE_DISK_FULL

/*
 * 错误消息常量
 */
#define IMPLICIT_TIME_MSG_SYNTAX_ERROR "隐含时间列语法错误"
#define IMPLICIT_TIME_MSG_INVALID_KEYWORD "无效的TIME关键字使用"
#define IMPLICIT_TIME_MSG_STORAGE_ERROR "隐含时间列存储错误"
#define IMPLICIT_TIME_MSG_COMPATIBILITY_ERROR "隐含时间列兼容性错误"
#define IMPLICIT_TIME_MSG_INTERNAL_ERROR "隐含时间列内部错误"
#define IMPLICIT_TIME_MSG_COLUMN_EXISTS "隐含时间列已存在"
#define IMPLICIT_TIME_MSG_COLUMN_NOT_FOUND "未找到隐含时间列"
#define IMPLICIT_TIME_MSG_INVALID_TABLE "表不支持隐含时间列"

/*
 * 错误处理函数声明
 */

/* DDL语法错误处理 */
extern void implicit_time_syntax_error(const char *detail, int location);
extern void implicit_time_invalid_keyword_error(const char *keyword, int location);

/* 存储错误处理 */
extern void implicit_time_storage_error(const char *operation, const char *detail);
extern void implicit_time_disk_full_error(const char *operation);

/* 兼容性错误处理 */
extern void implicit_time_compatibility_error(const char *feature, const char *detail);
extern void implicit_time_feature_not_supported_error(const char *feature);

/* 内部错误处理 */
extern void implicit_time_internal_error(const char *function, const char *detail);
extern void implicit_time_memory_error(const char *operation);

/* 列管理错误处理 */
extern void implicit_time_column_exists_error(const char *table_name);
extern void implicit_time_column_not_found_error(const char *table_name);
extern void implicit_time_invalid_table_error(const char *table_name, const char *reason);

/* 通用错误报告函数 */
extern void implicit_time_ereport(int elevel, int errcode, 
                                 const char *primary_msg,
                                 const char *detail_msg,
                                 const char *hint_msg,
                                 const char *context_msg,
                                 int error_location);

/* 错误上下文管理 */
extern void implicit_time_error_context_push(const char *context);
extern void implicit_time_error_context_pop(void);

/* 调试和诊断函数 */
extern void implicit_time_debug_log(const char *function, const char *message);
extern void implicit_time_warning_log(const char *function, const char *message);

#endif /* IMPLICIT_TIME_ERROR_H */