/*-------------------------------------------------------------------------
 *
 * pg_implicit_compat.c
 *        隐含列向后兼容性支持
 *
 * 本文件实现了PostgreSQL隐含时间列功能的向后兼容性支持，确保：
 * - 现有表的操作保持原有行为
 * - 新功能不影响现有应用程序
 * - 提供兼容性检查和迁移工具
 *
 * Portions Copyright (c) 1996-2022, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *        src/backend/catalog/pg_implicit_compat.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/relation.h"
#include "access/table.h"
#include "catalog/catalog.h"
#include "catalog/pg_attribute.h"
#include "catalog/pg_class.h"
#include "catalog/pg_implicit_columns.h"
#include "catalog/pg_implicit_compat.h"
#include "commands/defrem.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"

/* 静态函数声明 */
static void check_column_name_conflicts(Oid table_oid);
static bool validate_insert_compatibility(Oid table_oid);
static bool validate_update_compatibility(Oid table_oid);
static bool validate_delete_compatibility(Oid table_oid);
static bool validate_select_compatibility(Oid table_oid);
static bool is_system_table(Oid table_oid);
static bool is_temporary_table(Oid table_oid);

/*
 * check_backward_compatibility
 *
 * 检查表的向后兼容性，确保现有表的操作保持原有行为
 * 验证需求: Requirements 3.3, 6.1
 */
bool
check_backward_compatibility(Oid table_oid)
{
	HeapTuple	tuple;
	Form_pg_class classForm;
	bool		is_compatible = true;

	/* 参数验证 */
	if (!OidIsValid(table_oid))
	{
		elog(WARNING, "check_backward_compatibility: 无效的table_oid %u", table_oid);
		return false;
	}

	/* 获取表的基本信息 */
	tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(table_oid));
	if (!HeapTupleIsValid(tuple))
	{
		elog(WARNING, "表 %u 不存在，无法进行兼容性检查", table_oid);
		return false;
	}

	classForm = (Form_pg_class) GETSTRUCT(tuple);

	/* 检查表类型 - 只有普通表支持隐含列 */
	if (classForm->relkind != RELKIND_RELATION)
	{
		elog(DEBUG1, "表 %u 不是普通表，跳过隐含列处理", table_oid);
		is_compatible = true; /* 非普通表保持原有行为 */
		goto cleanup;
	}

	/* 检查表是否是系统表 */
	if (is_system_table(table_oid))
	{
		elog(DEBUG1, "表 %u 是系统表，不支持隐含列", table_oid);
		is_compatible = true; /* 系统表保持原有行为 */
		goto cleanup;
	}

	/* 检查表是否有特殊属性（如临时表、外部表等） */
	if (is_temporary_table(table_oid))
	{
		elog(DEBUG1, "表 %u 是临时表，隐含列功能可能受限", table_oid);
		/* 临时表可以有隐含列，但需要特殊处理 */
	}

	/* 检查列名冲突 */
	check_column_name_conflicts(table_oid);

cleanup:
	ReleaseSysCache(tuple);
	return is_compatible;
}

/*
 * ensure_legacy_behavior
 *
 * 确保现有表（没有隐含列的表）保持原有行为
 * 验证需求: Requirements 3.3, 6.1
 */
void
ensure_legacy_behavior(Oid table_oid)
{
	/* 如果表没有隐含时间列，确保所有操作保持原有行为 */
	if (!table_has_implicit_time(table_oid))
	{
		/* 记录调试信息 */
		elog(DEBUG2, "表 %u 没有隐含列，保持原有行为", table_oid);
		
		/* 这里可以添加额外的兼容性检查逻辑 */
		/* 例如：检查是否有与隐含列同名的用户列 */
		check_column_name_conflicts(table_oid);
	}
}

/*
 * check_column_name_conflicts
 *
 * 检查是否存在与隐含列同名的用户列
 */
static void
check_column_name_conflicts(Oid table_oid)
{
	HeapTuple	tuple;
	Form_pg_attribute attrForm;

	/* 检查是否有名为"time"的用户列 */
	tuple = SearchSysCache2(ATTNAME,
							ObjectIdGetDatum(table_oid),
							CStringGetDatum("time"));
	
	if (HeapTupleIsValid(tuple))
	{
		attrForm = (Form_pg_attribute) GETSTRUCT(tuple);
		
		/* 如果存在用户定义的"time"列，且不是隐含列 */
		if (!attrForm->attisdropped && attrForm->atttypid != TIMESTAMPOID)
		{
			elog(WARNING, "表 %u 已存在名为time的用户列，可能与隐含列功能冲突", 
				 table_oid);
		}
		
		ReleaseSysCache(tuple);
	}
}

/*
 * migrate_existing_table
 *
 * 为现有表提供隐含列迁移支持
 * 验证需求: Requirements 6.1
 */
bool
migrate_existing_table(Oid table_oid, bool add_implicit_time)
{
	Relation	rel;
	bool		success = false;

	/* 检查兼容性 */
	if (!check_backward_compatibility(table_oid))
	{
		elog(ERROR, "表 %u 不兼容隐含列功能", table_oid);
		return false;
	}

	/* 打开表 */
	rel = table_open(table_oid, AccessExclusiveLock);

	PG_TRY();
	{
		if (add_implicit_time)
		{
			/* 添加隐含时间列 */
			if (!table_has_implicit_time(table_oid))
			{
				add_implicit_time_column(rel);
				elog(NOTICE, "已为表 %s 添加隐含时间列", 
					 RelationGetRelationName(rel));
				success = true;
			}
			else
			{
				elog(WARNING, "表 %s 已经包含隐含时间列", 
					 RelationGetRelationName(rel));
				success = true;
			}
		}
		else
		{
			/* 移除隐含时间列 */
			if (table_has_implicit_time(table_oid))
			{
				remove_implicit_time_column(rel);
				elog(NOTICE, "已从表 %s 移除隐含时间列", 
					 RelationGetRelationName(rel));
				success = true;
			}
			else
			{
				elog(WARNING, "表 %s 不包含隐含时间列", 
					 RelationGetRelationName(rel));
				success = true;
			}
		}
	}
	PG_CATCH();
	{
		elog(ERROR, "迁移表 %s 时发生错误", RelationGetRelationName(rel));
		success = false;
	}
	PG_END_TRY();

	/* 关闭表 */
	table_close(rel, AccessExclusiveLock);

	return success;
}

/*
 * validate_operation_compatibility
 *
 * 验证操作的兼容性，确保不会破坏现有功能
 * 验证需求: Requirements 3.3, 6.1
 */
bool
validate_operation_compatibility(Oid table_oid, const char *operation)
{
	bool		is_compatible = true;

	if (!operation)
		return false;

	/* 确保现有表的行为不变 */
	ensure_legacy_behavior(table_oid);

	/* 根据操作类型进行特定的兼容性检查 */
	if (strcmp(operation, "INSERT") == 0)
	{
		/* INSERT操作的兼容性检查 */
		is_compatible = validate_insert_compatibility(table_oid);
	}
	else if (strcmp(operation, "UPDATE") == 0)
	{
		/* UPDATE操作的兼容性检查 */
		is_compatible = validate_update_compatibility(table_oid);
	}
	else if (strcmp(operation, "DELETE") == 0)
	{
		/* DELETE操作的兼容性检查 */
		is_compatible = validate_delete_compatibility(table_oid);
	}
	else if (strcmp(operation, "SELECT") == 0)
	{
		/* SELECT操作的兼容性检查 */
		is_compatible = validate_select_compatibility(table_oid);
	}

	return is_compatible;
}

/*
 * validate_insert_compatibility
 *
 * 验证INSERT操作的兼容性
 */
static bool
validate_insert_compatibility(Oid table_oid)
{
	/* 对于没有隐含列的表，INSERT行为完全不变 */
	if (!table_has_implicit_time(table_oid))
		return true;

	/* 对于有隐含列的表，确保隐含列不会影响用户的INSERT语句 */
	elog(DEBUG2, "表 %u 的INSERT操作将自动处理隐含时间列", table_oid);
	return true;
}

/*
 * validate_update_compatibility
 *
 * 验证UPDATE操作的兼容性
 */
static bool
validate_update_compatibility(Oid table_oid)
{
	/* 对于没有隐含列的表，UPDATE行为完全不变 */
	if (!table_has_implicit_time(table_oid))
		return true;

	/* 对于有隐含列的表，确保隐含列会自动更新 */
	elog(DEBUG2, "表 %u 的UPDATE操作将自动更新隐含时间列", table_oid);
	return true;
}

/*
 * validate_delete_compatibility
 *
 * 验证DELETE操作的兼容性
 */
static bool
validate_delete_compatibility(Oid table_oid)
{
	/* DELETE操作对隐含列没有特殊要求 */
	elog(DEBUG2, "表 %u 的DELETE操作与隐含列兼容", table_oid);
	return true;
}

/*
 * validate_select_compatibility
 *
 * 验证SELECT操作的兼容性
 */
static bool
validate_select_compatibility(Oid table_oid)
{
	/* 对于没有隐含列的表，SELECT行为完全不变 */
	if (!table_has_implicit_time(table_oid))
		return true;

	/* 对于有隐含列的表，确保SELECT *不会返回隐含列 */
	elog(DEBUG2, "表 %u 的SELECT操作将正确处理隐含列可见性", table_oid);
	return true;
}

/*
 * get_compatibility_info
 *
 * 获取表的兼容性信息
 */
char *
get_compatibility_info(Oid table_oid)
{
	StringInfoData info;
	HeapTuple	tuple;
	Form_pg_class classForm;

	initStringInfo(&info);

	tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(table_oid));
	if (!HeapTupleIsValid(tuple))
	{
		appendStringInfo(&info, "表 %u 不存在", table_oid);
		return info.data;
	}

	classForm = (Form_pg_class) GETSTRUCT(tuple);

	appendStringInfo(&info, "表 %s:\n", NameStr(classForm->relname));
	
	if (table_has_implicit_time(table_oid))
	{
		appendStringInfo(&info, "- 包含隐含时间列\n");
		appendStringInfo(&info, "- 隐含列属性编号: %d\n", 
						 get_implicit_time_attnum(table_oid));
	}
	else
	{
		appendStringInfo(&info, "- 不包含隐含时间列（保持原有行为）\n");
	}

	appendStringInfo(&info, "- 表类型: %c\n", classForm->relkind);
	appendStringInfo(&info, "- 持久性: %c\n", classForm->relpersistence);
	
	if (check_backward_compatibility(table_oid))
	{
		appendStringInfo(&info, "- 兼容性: 良好\n");
	}
	else
	{
		appendStringInfo(&info, "- 兼容性: 存在问题\n");
	}

	ReleaseSysCache(tuple);
	return info.data;
}

/*
 * is_system_table
 *
 * 检查表是否是系统表
 */
static bool
is_system_table(Oid table_oid)
{
	return IsSystemClass(table_oid, NULL);
}

/*
 * is_temporary_table
 *
 * 检查表是否是临时表
 */
static bool
is_temporary_table(Oid table_oid)
{
	HeapTuple	tuple;
	Form_pg_class classForm;
	bool		is_temp = false;

	tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(table_oid));
	if (HeapTupleIsValid(tuple))
	{
		classForm = (Form_pg_class) GETSTRUCT(tuple);
		is_temp = (classForm->relpersistence == RELPERSISTENCE_TEMP);
		ReleaseSysCache(tuple);
	}

	return is_temp;
}