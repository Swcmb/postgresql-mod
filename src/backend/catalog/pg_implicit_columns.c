/*-------------------------------------------------------------------------
 *
 * pg_implicit_columns.c
 *	  隐含列管理接口实现
 *
 * 本文件实现了PostgreSQL隐含时间列功能的核心管理接口，包括：
 * - 添加和删除隐含时间列
 * - 查询表是否包含隐含时间列
 * - 获取隐含时间列的属性编号
 * - 生成当前时间戳
 *
 * Portions Copyright (c) 1996-2022, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/catalog/pg_implicit_columns.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/relation.h"
#include "access/table.h"
#include "catalog/catalog.h"
#include "catalog/dependency.h"
#include "catalog/heap.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "utils/timestamp.h"
#include "utils/datetime.h"
#include "utils/builtins.h"
#include "fmgr.h"
#include "catalog/pg_attribute.h"
#include "catalog/pg_attrdef.h"
#include "catalog/pg_class.h"
#include "catalog/pg_description.h"
#include "catalog/pg_implicit_columns.h"
#include "catalog/pg_implicit_columns_d.h"
#include "catalog/pg_type.h"
#include "commands/defrem.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/inval.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"

/* 隐含时间列的默认名称 */
#define IMPLICIT_TIME_COLUMN_NAME "time"
/*
 * table_has_implicit_time
 *
 * 检查指定的表是否包含隐含时间列
 * 通过查询pg_implicit_columns系统表来确定
 */
bool
table_has_implicit_time(Oid table_oid)
{
	Relation	implicit_rel;
	ScanKeyData skey[1];
	SysScanDesc scan;
	HeapTuple	tuple;
	bool		found = false;
	
	/* 参数验证 */
	if (!OidIsValid(table_oid))
	{
		elog(DEBUG1, "table_has_implicit_time: 无效的table_oid %u", table_oid);
		return false;
	}
	
	/* 打开pg_implicit_columns系统表 */
	implicit_rel = table_open(ImplicitColumnsRelationId, AccessShareLock);
	
	/* 设置扫描键：查找指定表的隐含时间列 */
	ScanKeyInit(&skey[0],
				Anum_pg_implicit_columns_ic_relid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(table_oid));
	
	/* 开始扫描 */
	scan = systable_beginscan(implicit_rel, ImplicitColumnsRelidAttnameIndexId,
							  true, NULL, 1, skey);
	
	/* 查找隐含时间列 */
	while (HeapTupleIsValid(tuple = systable_getnext(scan)))
	{
		Form_pg_implicit_columns form = (Form_pg_implicit_columns) GETSTRUCT(tuple);
		
		/* 检查是否是时间列 */
		if (strcmp(NameStr(form->ic_attname), IMPLICIT_TIME_COLUMN_NAME) == 0)
		{
			found = true;
			break;
		}
	}
	
	/* 清理 */
	systable_endscan(scan);
	table_close(implicit_rel, AccessShareLock);
	
	elog(DEBUG1, "table_has_implicit_time: 表 %u %s隐含时间列", 
		 table_oid, found ? "有" : "没有");
	
	return found;
}

/*
 * get_implicit_time_attnum
 *
 * 获取指定表的隐含时间列的属性编号
 * 通过查询pg_implicit_columns系统表获取真实的属性编号
 */
AttrNumber
get_implicit_time_attnum(Oid table_oid)
{
	Relation	implicit_rel;
	ScanKeyData skey[2];
	SysScanDesc scan;
	HeapTuple	tuple;
	AttrNumber	attnum = InvalidAttrNumber;
	
	/* 参数验证 */
	if (!OidIsValid(table_oid))
	{
		elog(DEBUG1, "get_implicit_time_attnum: 无效的table_oid %u", table_oid);
		return InvalidAttrNumber;
	}
	
	/* 打开pg_implicit_columns系统表 */
	implicit_rel = table_open(ImplicitColumnsRelationId, AccessShareLock);
	
	/* 设置扫描键：查找指定表的隐含时间列 */
	ScanKeyInit(&skey[0],
				Anum_pg_implicit_columns_ic_relid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(table_oid));
	ScanKeyInit(&skey[1],
				Anum_pg_implicit_columns_ic_attname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(IMPLICIT_TIME_COLUMN_NAME));
	
	/* 开始扫描 */
	scan = systable_beginscan(implicit_rel, ImplicitColumnsRelidAttnameIndexId,
							  true, NULL, 2, skey);
	
	/* 查找隐含时间列 */
	tuple = systable_getnext(scan);
	if (HeapTupleIsValid(tuple))
	{
		Form_pg_implicit_columns form = (Form_pg_implicit_columns) GETSTRUCT(tuple);
		attnum = form->ic_attnum;
	}
	
	/* 清理 */
	systable_endscan(scan);
	table_close(implicit_rel, AccessShareLock);
	
	elog(DEBUG1, "get_implicit_time_attnum: 表 %u 的隐含时间列属性编号为 %d", 
		 table_oid, attnum);
	
	return attnum;
}

/*
 * get_current_timestamp
 *
 * 获取当前时间戳，用于隐含时间列
 * 确保使用数据库服务器的当前时间，并提供秒级精度
 * 符合需求4.3, 4.5的要求
 */
Timestamp
get_current_timestamp(void)
{
	TimestampTz now_tz;
	Timestamp	now;
	
	/* 获取当前时间戳（带时区），这是服务器的当前时间 */
	now_tz = GetCurrentTimestamp();
	
	/* 
	 * 将TimestampTz转换为Timestamp
	 * 这里我们使用PostgreSQL内置的转换函数
	 */
	now = DatumGetTimestamp(DirectFunctionCall1(timestamptz_timestamp, TimestampTzGetDatum(now_tz)));
	
	/* 截断到秒级精度，去除微秒部分 */
	now = (now / USECS_PER_SEC) * USECS_PER_SEC;
	
	return now;
}
/*
 * add_implicit_time_column
 *
 * 为指定的表添加隐含时间列
 * 在pg_implicit_columns系统表中记录隐含时间列信息
 */
void
add_implicit_time_column(Relation rel)
{
	Relation	implicit_rel;
	HeapTuple	tuple;
	Datum		values[Natts_pg_implicit_columns];
	bool		nulls[Natts_pg_implicit_columns];
	Oid			table_oid;
	AttrNumber	next_attnum;
	
	/* 检查输入参数 */
	if (!RelationIsValid(rel))
	{
		elog(ERROR, "add_implicit_time_column: 无效的关系描述符");
		return;
	}
		
	table_oid = RelationGetRelid(rel);
	
	/* 检查是否已经有隐含时间列 */
	if (table_has_implicit_time(table_oid))
	{
		elog(DEBUG1, "add_implicit_time_column: 表 %s 已经有隐含时间列", 
			 RelationGetRelationName(rel));
		return;
	}
	
	/* 计算下一个可用的属性编号 */
	next_attnum = RelationGetNumberOfAttributes(rel) + 1;
	
	/* 打开pg_implicit_columns系统表 */
	implicit_rel = table_open(ImplicitColumnsRelationId, RowExclusiveLock);
	
	/* 准备插入数据 */
	memset(nulls, false, sizeof(nulls));
	
	values[Anum_pg_implicit_columns_ic_relid - 1] = ObjectIdGetDatum(table_oid);
	values[Anum_pg_implicit_columns_ic_attname - 1] = CStringGetDatum(IMPLICIT_TIME_COLUMN_NAME);
	values[Anum_pg_implicit_columns_ic_attnum - 1] = Int16GetDatum(next_attnum);
	values[Anum_pg_implicit_columns_ic_atttypid - 1] = ObjectIdGetDatum(TIMESTAMPOID);
	values[Anum_pg_implicit_columns_ic_visible - 1] = BoolGetDatum(false);
	
	/* 创建元组并插入 */
	tuple = heap_form_tuple(RelationGetDescr(implicit_rel), values, nulls);
	CatalogTupleInsert(implicit_rel, tuple);
	
	/* 清理 */
	heap_freetuple(tuple);
	table_close(implicit_rel, RowExclusiveLock);
	
	/* 使缓存失效 */
	CacheInvalidateRelcache(rel);
	
	elog(DEBUG1, "add_implicit_time_column: 成功为表 %s (OID=%u) 添加隐含时间列", 
		 RelationGetRelationName(rel), table_oid);
}
/*
 * remove_implicit_time_column
 *
 * 从指定的表中删除隐含时间列
 * 从pg_implicit_columns系统表中删除相应记录
 */
void
remove_implicit_time_column(Relation rel)
{
	Relation	implicit_rel;
	ScanKeyData skey[2];
	SysScanDesc scan;
	HeapTuple	tuple;
	Oid			table_oid;
	bool		found = false;
	
	/* 检查输入参数 */
	if (!RelationIsValid(rel))
		elog(ERROR, "无效的关系描述符");
		
	table_oid = RelationGetRelid(rel);
	
	/* 检查表是否有隐含时间列 */
	if (!table_has_implicit_time(table_oid))
	{
		elog(WARNING, "表 %s 不包含隐含时间列", 
			 RelationGetRelationName(rel));
		return;
	}
	
	/* 打开pg_implicit_columns系统表 */
	implicit_rel = table_open(ImplicitColumnsRelationId, RowExclusiveLock);
	
	/* 设置扫描键：查找指定表的隐含时间列 */
	ScanKeyInit(&skey[0],
				Anum_pg_implicit_columns_ic_relid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(table_oid));
	ScanKeyInit(&skey[1],
				Anum_pg_implicit_columns_ic_attname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(IMPLICIT_TIME_COLUMN_NAME));
	
	/* 开始扫描 */
	scan = systable_beginscan(implicit_rel, ImplicitColumnsRelidAttnameIndexId,
							  true, NULL, 2, skey);
	
	/* 查找并删除隐含时间列记录 */
	while (HeapTupleIsValid(tuple = systable_getnext(scan)))
	{
		CatalogTupleDelete(implicit_rel, &tuple->t_self);
		found = true;
	}
	
	/* 清理 */
	systable_endscan(scan);
	table_close(implicit_rel, RowExclusiveLock);
	
	if (found)
	{
		/* 使缓存失效 */
		CacheInvalidateRelcache(rel);
		elog(DEBUG1, "remove_implicit_time_column: 成功从表 %s 删除隐含时间列", 
			 RelationGetRelationName(rel));
	}
	else
	{
		elog(WARNING, "remove_implicit_time_column: 在表 %s 中未找到隐含时间列记录", 
			 RelationGetRelationName(rel));
	}
}

/*
 * is_implicit_column
 *
 * 检查指定的列是否是隐含列
 */
bool
is_implicit_column(Oid table_oid, const char *column_name)
{
	if (!column_name)
		return false;
		
	/* 目前只支持时间列作为隐含列 */
	if (strcmp(column_name, IMPLICIT_TIME_COLUMN_NAME) == 0)
		return table_has_implicit_time(table_oid);
	
	return false;
}

/*
 * validate_implicit_column_type
 *
 * 验证指定的数据类型是否适合作为隐含列类型
 */
bool
validate_implicit_column_type(Oid type_oid)
{
	/* 目前只支持timestamp类型作为隐含时间列 */
	return (type_oid == TIMESTAMPOID || type_oid == TIMESTAMPTZOID);
}
/*
 * get_table_implicit_info
 *
 * 获取表的隐含列信息
 */
TableImplicitInfo *
get_table_implicit_info(Oid table_oid)
{
	TableImplicitInfo *info;
	Relation	implicit_rel;
	ScanKeyData skey[1];
	SysScanDesc scan;
	HeapTuple	tuple;
	
	/* 参数验证 */
	if (!OidIsValid(table_oid))
		return NULL;
	
	/* 分配内存 */
	info = (TableImplicitInfo *) palloc0(sizeof(TableImplicitInfo));
	info->table_oid = table_oid;
	info->has_implicit_time = false;
	info->time_attnum = InvalidAttrNumber;
	info->implicit_columns = NIL;
	info->num_implicit_cols = 0;
	
	/* 打开pg_implicit_columns系统表 */
	implicit_rel = table_open(ImplicitColumnsRelationId, AccessShareLock);
	
	/* 设置扫描键 */
	ScanKeyInit(&skey[0],
				Anum_pg_implicit_columns_ic_relid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(table_oid));
	
	/* 开始扫描 */
	scan = systable_beginscan(implicit_rel, ImplicitColumnsRelidAttnameIndexId,
							  true, NULL, 1, skey);
	
	/* 收集隐含列信息 */
	while (HeapTupleIsValid(tuple = systable_getnext(scan)))
	{
		Form_pg_implicit_columns form = (Form_pg_implicit_columns) GETSTRUCT(tuple);
		ImplicitColumn *col;
		
		/* 创建隐含列结构 */
		col = create_implicit_column(NameStr(form->ic_attname),
									 form->ic_atttypid,
									 form->ic_attnum);
		
		/* 添加到列表 */
		info->implicit_columns = lappend(info->implicit_columns, col);
		info->num_implicit_cols++;
		
		/* 检查是否是时间列 */
		if (strcmp(NameStr(form->ic_attname), IMPLICIT_TIME_COLUMN_NAME) == 0)
		{
			info->has_implicit_time = true;
			info->time_attnum = form->ic_attnum;
		}
	}
	
	/* 清理 */
	systable_endscan(scan);
	table_close(implicit_rel, AccessShareLock);
	
	return info;
}

/*
 * free_table_implicit_info
 *
 * 释放表隐含列信息结构
 */
void
free_table_implicit_info(TableImplicitInfo *info)
{
	ListCell *lc;
	
	if (!info)
		return;
	
	/* 释放隐含列列表 */
	foreach(lc, info->implicit_columns)
	{
		ImplicitColumn *col = (ImplicitColumn *) lfirst(lc);
		free_implicit_column(col);
	}
	
	list_free(info->implicit_columns);
	pfree(info);
}

/*
 * create_implicit_column
 *
 * 创建隐含列结构
 */
ImplicitColumn *
create_implicit_column(const char *name, Oid type_oid, AttrNumber attnum)
{
	ImplicitColumn *col;
	
	if (!name)
		return NULL;
	
	col = (ImplicitColumn *) palloc0(sizeof(ImplicitColumn));
	col->column_name = pstrdup(name);
	col->column_type = type_oid;
	col->attnum = attnum;
	col->is_active = true;
	col->created_time = GetCurrentTimestamp();
	
	return col;
}

/*
 * free_implicit_column
 *
 * 释放隐含列结构
 */
void
free_implicit_column(ImplicitColumn *col)
{
	if (!col)
		return;
	
	if (col->column_name)
		pfree(col->column_name);
	
	pfree(col);
}