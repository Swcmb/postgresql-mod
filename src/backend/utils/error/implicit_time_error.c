/*-------------------------------------------------------------------------
 *
 * implicit_time_error.c
 *    隐含时间列功能的错误处理实现
 *
 * 本文件实现了隐含时间列功能相关的错误处理函数，提供统一的错误报告
 * 和日志记录接口。所有隐含时间列相关的错误都应该通过这些函数处理。
 *
 * Copyright (c) 2024, PostgreSQL Global Development Group
 *
 * src/backend/utils/error/implicit_time_error.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "utils/implicit_time_error.h"
#include "utils/elog.h"
#include "utils/guc.h"
#include "miscadmin.h"

/*
 * 错误上下文栈，用于跟踪嵌套的错误上下文
 */
#define MAX_ERROR_CONTEXT_DEPTH 10

static struct {
    char *contexts[MAX_ERROR_CONTEXT_DEPTH];
    int depth;
} implicit_error_context_stack = { {NULL}, 0 };

/*
 * implicit_time_syntax_error
 *    报告DDL语法错误
 */
void
implicit_time_syntax_error(const char *detail, int location)
{
    ereport(ERROR,
            (errcode(IMPLICIT_TIME_SYNTAX_ERROR),
             errmsg(IMPLICIT_TIME_MSG_SYNTAX_ERROR),
             errdetail("%s", detail ? detail : "语法解析失败"),
             errhint("请检查WITH TIME或WITHOUT TIME关键字的使用")));
}

/*
 * implicit_time_invalid_keyword_error
 *    报告无效关键字错误
 */
void
implicit_time_invalid_keyword_error(const char *keyword, int location)
{
    ereport(ERROR,
            (errcode(IMPLICIT_TIME_SYNTAX_ERROR),
             errmsg(IMPLICIT_TIME_MSG_INVALID_KEYWORD),
             errdetail("无效的关键字: \"%s\"", keyword ? keyword : "未知"),
             errhint("期望使用WITH TIME或WITHOUT TIME")));
}

/*
 * implicit_time_storage_error
 *    报告存储相关错误
 */
void
implicit_time_storage_error(const char *operation, const char *detail)
{
    ereport(ERROR,
            (errcode(IMPLICIT_TIME_INTERNAL_ERROR),
             errmsg(IMPLICIT_TIME_MSG_STORAGE_ERROR),
             errdetail("操作 \"%s\" 失败: %s", 
                      operation ? operation : "未知操作",
                      detail ? detail : "存储操作失败"),
             errhint("请检查磁盘空间和权限设置")));
}

/*
 * implicit_time_disk_full_error
 *    报告磁盘空间不足错误
 */
void
implicit_time_disk_full_error(const char *operation)
{
    ereport(ERROR,
            (errcode(IMPLICIT_TIME_DISK_FULL),
             errmsg("磁盘空间不足"),
             errdetail("无法完成隐含时间列操作: %s", 
                      operation ? operation : "未知操作"),
             errhint("请释放磁盘空间后重试")));
}

/*
 * implicit_time_compatibility_error
 *    报告兼容性错误
 */
void
implicit_time_compatibility_error(const char *feature, const char *detail)
{
    ereport(ERROR,
            (errcode(IMPLICIT_TIME_FEATURE_NOT_SUPPORTED),
             errmsg(IMPLICIT_TIME_MSG_COMPATIBILITY_ERROR),
             errdetail("功能 \"%s\" 与隐含时间列不兼容: %s",
                      feature ? feature : "未知功能",
                      detail ? detail : "兼容性冲突"),
             errhint("请检查功能组合的兼容性")));
}

/*
 * implicit_time_feature_not_supported_error
 *    报告功能不支持错误
 */
void
implicit_time_feature_not_supported_error(const char *feature)
{
    ereport(ERROR,
            (errcode(IMPLICIT_TIME_FEATURE_NOT_SUPPORTED),
             errmsg("功能不支持"),
             errdetail("隐含时间列不支持功能: %s", 
                      feature ? feature : "未知功能"),
             errhint("请查阅文档了解支持的功能列表")));
}

/*
 * implicit_time_internal_error
 *    报告内部错误
 */
void
implicit_time_internal_error(const char *function, const char *detail)
{
    ereport(ERROR,
            (errcode(IMPLICIT_TIME_INTERNAL_ERROR),
             errmsg(IMPLICIT_TIME_MSG_INTERNAL_ERROR),
             errdetail("函数 \"%s\" 内部错误: %s",
                      function ? function : "未知函数",
                      detail ? detail : "内部状态异常"),
             errhint("这是一个内部错误，请联系系统管理员")));
}

/*
 * implicit_time_memory_error
 *    报告内存分配错误
 */
void
implicit_time_memory_error(const char *operation)
{
    ereport(ERROR,
            (errcode(ERRCODE_OUT_OF_MEMORY),
             errmsg("内存不足"),
             errdetail("隐含时间列操作 \"%s\" 内存分配失败",
                      operation ? operation : "未知操作"),
             errhint("请检查系统内存使用情况")));
}

/*
 * implicit_time_column_exists_error
 *    报告列已存在错误
 */
void
implicit_time_column_exists_error(const char *table_name)
{
    ereport(ERROR,
            (errcode(ERRCODE_DUPLICATE_COLUMN),
             errmsg(IMPLICIT_TIME_MSG_COLUMN_EXISTS),
             errdetail("表 \"%s\" 已经包含隐含时间列",
                      table_name ? table_name : "未知表"),
             errhint("请检查表结构或使用ALTER TABLE修改")));
}

/*
 * implicit_time_column_not_found_error
 *    报告列未找到错误
 */
void
implicit_time_column_not_found_error(const char *table_name)
{
    ereport(ERROR,
            (errcode(ERRCODE_UNDEFINED_COLUMN),
             errmsg(IMPLICIT_TIME_MSG_COLUMN_NOT_FOUND),
             errdetail("表 \"%s\" 不包含隐含时间列",
                      table_name ? table_name : "未知表"),
             errhint("请使用WITH TIME创建表或添加隐含时间列")));
}

/*
 * implicit_time_invalid_table_error
 *    报告无效表错误
 */
void
implicit_time_invalid_table_error(const char *table_name, const char *reason)
{
    ereport(ERROR,
            (errcode(ERRCODE_WRONG_OBJECT_TYPE),
             errmsg(IMPLICIT_TIME_MSG_INVALID_TABLE),
             errdetail("表 \"%s\" 不支持隐含时间列: %s",
                      table_name ? table_name : "未知表",
                      reason ? reason : "表类型不兼容"),
             errhint("请检查表的类型和属性")));
}

/*
 * implicit_time_ereport
 *    通用错误报告函数
 */
void
implicit_time_ereport(int elevel, int error_code, 
                     const char *primary_msg,
                     const char *detail_msg,
                     const char *hint_msg,
                     const char *context_msg,
                     int error_location)
{
    ereport(elevel,
            (errcode(error_code),
             errmsg("%s", primary_msg ? primary_msg : "隐含时间列错误"),
             detail_msg ? errdetail("%s", detail_msg) : 0,
             hint_msg ? errhint("%s", hint_msg) : 0,
             context_msg ? errcontext("%s", context_msg) : 0));
}

/*
 * implicit_time_error_context_push
 *    推入错误上下文
 */
void
implicit_time_error_context_push(const char *context)
{
    if (implicit_error_context_stack.depth >= MAX_ERROR_CONTEXT_DEPTH)
    {
        elog(WARNING, "隐含时间列错误上下文栈溢出");
        return;
    }
    
    if (context)
    {
        implicit_error_context_stack.contexts[implicit_error_context_stack.depth] = 
            pstrdup(context);
        implicit_error_context_stack.depth++;
    }
}

/*
 * implicit_time_error_context_pop
 *    弹出错误上下文
 */
void
implicit_time_error_context_pop(void)
{
    if (implicit_error_context_stack.depth > 0)
    {
        implicit_error_context_stack.depth--;
        if (implicit_error_context_stack.contexts[implicit_error_context_stack.depth])
        {
            pfree(implicit_error_context_stack.contexts[implicit_error_context_stack.depth]);
            implicit_error_context_stack.contexts[implicit_error_context_stack.depth] = NULL;
        }
    }
}

/*
 * implicit_time_debug_log
 *    记录调试信息
 */
void
implicit_time_debug_log(const char *function, const char *message)
{
    elog(DEBUG1, "隐含时间列调试 [%s]: %s",
         function ? function : "未知函数",
         message ? message : "无消息");
}

/*
 * implicit_time_warning_log
 *    记录警告信息
 */
void
implicit_time_warning_log(const char *function, const char *message)
{
    elog(WARNING, "隐含时间列警告 [%s]: %s",
         function ? function : "未知函数",
         message ? message : "无消息");
}