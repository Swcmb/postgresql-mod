/*-------------------------------------------------------------------------
 *
 * pg_implicit_compat.h
 *        隐含列向后兼容性支持的头文件
 *
 * Portions Copyright (c) 1996-2022, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/pg_implicit_compat.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_IMPLICIT_COMPAT_H
#define PG_IMPLICIT_COMPAT_H

#include "postgres.h"

/* 兼容性检查函数 */
extern bool check_backward_compatibility(Oid table_oid);
extern void ensure_legacy_behavior(Oid table_oid);
extern bool migrate_existing_table(Oid table_oid, bool add_implicit_time);
extern bool validate_operation_compatibility(Oid table_oid, const char *operation);
extern char *get_compatibility_info(Oid table_oid);

#endif /* PG_IMPLICIT_COMPAT_H */
