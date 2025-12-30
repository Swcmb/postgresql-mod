/*-------------------------------------------------------------------------
 *
 * pg_implicit_columns.h
 *	  definition of the "implicit_columns" system catalog (pg_implicit_columns)
 *
 * Portions Copyright (c) 1996-2022, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/pg_implicit_columns.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_IMPLICIT_COLUMNS_H
#define PG_IMPLICIT_COLUMNS_H

#include "catalog/genbki.h"
#include "catalog/pg_implicit_columns_d.h"
#include "access/attnum.h"
#include "access/relation.h"
#include "datatype/timestamp.h"

/* ----------------
 *		pg_implicit_columns definition.  cpp turns this into
 *		typedef struct FormData_pg_implicit_columns
 * ----------------
 */
CATALOG(pg_implicit_columns,7000,ImplicitColumnsRelationId)
{
	Oid			ic_relid BKI_LOOKUP(pg_class);		/* 表OID */
	NameData	ic_attname;							/* 隐含列名 */
	int16		ic_attnum;							/* 属性编号 */
	Oid			ic_atttypid BKI_LOOKUP(pg_type);	/* 数据类型OID */
	bool		ic_visible BKI_DEFAULT(f);			/* 是否在SELECT *中可见 */
} FormData_pg_implicit_columns;

/* ----------------
 *		Form_pg_implicit_columns corresponds to a pointer to a tuple with
 *		the format of pg_implicit_columns relation.
 * ----------------
 */
typedef FormData_pg_implicit_columns *Form_pg_implicit_columns;

DECLARE_UNIQUE_INDEX(pg_implicit_columns_relid_attname_index, 7003, ImplicitColumnsRelidAttnameIndexId, on pg_implicit_columns using btree(ic_relid oid_ops, ic_attname name_ops));

/*
 * ImplicitColumn - 隐含列的描述结构体
 */
typedef struct ImplicitColumn
{
	char	   *column_name;		/* 隐含列名称 */
	Oid			column_type;		/* 隐含列数据类型OID */
	AttrNumber	attnum;				/* 属性编号 */
	bool		is_active;			/* 是否激活 */
	Timestamp	created_time;		/* 创建时间 */
} ImplicitColumn;

/*
 * TableImplicitInfo - 表的隐含列信息结构体
 */
typedef struct TableImplicitInfo
{
	Oid			table_oid;			/* 表的OID */
	bool		has_implicit_time;	/* 是否有隐含时间列 */
	AttrNumber	time_attnum;		/* 时间列的属性编号 */
	List	   *implicit_columns;	/* 隐含列列表 (ImplicitColumn*) */
	int			num_implicit_cols;	/* 隐含列数量 */
} TableImplicitInfo;

/* 函数声明 */

/*
 * 隐含列管理接口
 */
extern void add_implicit_time_column(Relation rel);
extern void remove_implicit_time_column(Relation rel);
extern bool table_has_implicit_time(Oid table_oid);
extern AttrNumber get_implicit_time_attnum(Oid table_oid);
extern Timestamp get_current_timestamp(void);

/*
 * 隐含列验证接口
 */
extern bool is_implicit_column(Oid table_oid, const char *column_name);
extern bool validate_implicit_column_type(Oid type_oid);

/*
 * 隐含列信息管理接口
 */
extern TableImplicitInfo *get_table_implicit_info(Oid table_oid);
extern void free_table_implicit_info(TableImplicitInfo *info);
extern ImplicitColumn *create_implicit_column(const char *name, Oid type_oid, AttrNumber attnum);
extern void free_implicit_column(ImplicitColumn *col);

#endif							/* PG_IMPLICIT_COLUMNS_H */