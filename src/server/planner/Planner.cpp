#include "planner/Planner.hpp"
#include "access/table/AttrNumber.hpp"
#include "access/table/ITuple.hpp"
#include "cluster_state.hpp"
#include "db/catalog/OperatorInfo.hpp"
#include "db/catalog/TableId.hpp"
#include "db/catalog/TypeId.hpp"
#include "executor/Datum.hpp"
#include "executor/Oid.hpp"
#include "executor/VirtualTuple.hpp"
#include "executor/expr/AttributeExpressionNode.hpp"
#include "executor/expr/ConstantExpressionNode.hpp"
#include "executor/expr/FunctionExpressionNode.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include "executor/func/FunctionContext.hpp"
#include "executor/plan/DeleteNode.hpp"
#include "executor/plan/InsertNode.hpp"
#include "executor/plan/SeqScan.hpp"
#include "executor/plan/UpdateNode.hpp"
#include "sql/DeleteStatement.h"
#include "sql/Expr.h"
#include "sql/InsertStatement.h"
#include "sql/SQLStatement.h"
#include "sql/SelectStatement.h"
#include "sql/Table.h"
#include "sql/UpdateStatement.h"
#include "utils/DatumArray.hpp"
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

using namespace mi::planner;

static mi::db::catalog::OperatorStrategy map_op_strategy(hsql::OperatorType type) {
    switch (type) {
    case hsql::OperatorType::kOpEquals:
        return mi::db::catalog::OperatorStrategy::Equal;
    case hsql::OperatorType::kOpNotEquals:
        return mi::db::catalog::OperatorStrategy::NotEqual;
    case hsql::OperatorType::kOpLess:
        return mi::db::catalog::OperatorStrategy::Less;
    case hsql::OperatorType::kOpLessEq:
        return mi::db::catalog::OperatorStrategy::LessEq;
    case hsql::OperatorType::kOpGreater:
        return mi::db::catalog::OperatorStrategy::Greater;
    case hsql::OperatorType::kOpGreaterEq:
        return mi::db::catalog::OperatorStrategy::GreaterEq;
    default:
        throw std::runtime_error("Operator is not supported");
    }
}

static std::unique_ptr<mi::executor::IExpressionNode> parse_expr(hsql::Expr &expr) {
    if (expr.isType(hsql::ExprType::kExprOperator)) {
        auto left = parse_expr(*expr.expr);
        auto right = parse_expr(*expr.expr2);
        auto strategy = map_op_strategy(expr.opType);
        const auto info = mi::DatabaseGlobal->GetSchema()->FindOperatorInfo(
            // Пока только инты юзаю
            mi::schema::catalog::TypeId::Int32, mi::schema::catalog::TypeId::Int32, strategy);
        if (info == nullptr) {
            throw std::runtime_error("Could not find operator");
        }

        auto function = info->GetFunction();
        auto fctx = mi::executor::FunctionContext{function, true, 2};
        auto args = std::vector<std::unique_ptr<mi::executor::IExpressionNode>>{};
        args.emplace_back(std::move(left));
        args.emplace_back(std::move(right));
        return std::make_unique<mi::executor::FunctionExpressionNode>(std::move(fctx),
                                                                      std::move(args));
    } else if (expr.isType(hsql::ExprType::kExprLiteralInt)) {
        return std::make_unique<mi::executor::expr::ConstantExpressionNode>(mi::Datum{expr.ival});
    } else if (expr.isType(hsql::ExprType::kExprLiteralNull)) {
        return std::make_unique<mi::executor::expr::ConstantExpressionNode>(std::nullopt);
    } else if (expr.isType(hsql::ExprType::kExprColumnRef)) {
        auto &tableInfo = mi::DatabaseGlobal->GetSchema()->GetTableInfo(
            mi::schema::catalog::TableId::MainTableId);
        auto attno = mi::access::table::AttrNumber::Min();
        bool found = false;
        for (const auto &col : tableInfo.GetColumns()) {
            if (col.GetName() == expr.name) {
                found = true;
                break;
            }

            attno++;
        }
        if (!found) {
            throw std::runtime_error("could not find column: " + std::string{expr.name});
        }

        assert(attno <= tableInfo.GetDescriptor()->GetMaxAttrNumber());
        return std::make_unique<mi::executor::expr::AttributeExpressionNode>(attno);
    } else {
        throw std::runtime_error("expression is not supported: " + std::to_string(static_cast<int>(expr.type)));
    }
}

std::unique_ptr<mi::executor::plan::IPlanNode> Planner::Plan(hsql::SQLStatement &statement) {
    assert(IsPlannableStatement(statement));

    if (statement.is(hsql::StatementType::kStmtSelect)) {
        hsql::SelectStatement &stmt = dynamic_cast<hsql::SelectStatement &>(statement);

        if (stmt.groupBy) {
            throw std::runtime_error("GROUP BY is not supported");
        }

        if (stmt.limit) {
            throw std::runtime_error("LIMIT is not supported");
        }

        if (stmt.order) {
            throw std::runtime_error("ORDER BY is not supported");
        }

        if (stmt.lockings) {
            throw std::runtime_error("LOCKING is not supported");
        }

        if (stmt.setOperations) {
            throw std::runtime_error("SET OPERATION is not supported");
        }

        if (stmt.selectDistinct) {
            throw std::runtime_error("DISTINCT is not supported");
        }

        switch (stmt.fromTable->type) {
        case hsql::TableRefType::kTableName:
            // ok, idfc there is only 1 table for now
            break;
        case hsql::TableRefType::kTableSelect:
            throw std::runtime_error("subSELECT is not supported");
        case hsql::TableRefType::kTableJoin:
            /* FALLTHROUGH */
        case hsql::TableRefType::kTableCrossProduct:
            throw std::runtime_error("JOIN is not supported");
        }

        auto table = DatabaseGlobal->OpenTable(schema::catalog::TableId::MainTableId);

        // SELECT predicate
        std::unique_ptr<executor::IExpressionNode> qual;
        if (stmt.whereClause) {
            qual = parse_expr(*stmt.whereClause);
        } else {
            qual = nullptr;
        }

        return std::make_unique<executor::plan::SeqScan>(table, std::move(qual));
    } else if (statement.is(hsql::StatementType::kStmtInsert)) {
        hsql::InsertStatement &stmt = dynamic_cast<hsql::InsertStatement &>(statement);
        const auto &info =
            DatabaseGlobal->GetSchema()->GetTableInfo(schema::catalog::TableId::MainTableId);
        const auto &desc = info.GetDescriptor();
        if (stmt.values->size() != desc->GetMaxAttrNumber()) {
            throw std::runtime_error(
                "INSERT attributes and table descriptor are not the same size");
        }
        auto arrays = DatumArray{};
        for (auto attno = access::table::AttrNumber::Min(); attno <= desc->GetMaxAttrNumber();
             ++attno) {
            const auto &attDesc = desc->Attributes()[attno.ToIndex()];
            if (!(attDesc.TypeId() != schema::catalog::TypeId::Int16 ||
                  attDesc.TypeId() != schema::catalog::TypeId::Int32 ||
                  attDesc.TypeId() != schema::catalog::TypeId::Int64)) {
                throw std::runtime_error("only int types are supported by planner for now");
            }

            auto value = stmt.values->at(attno.ToIndex());
            if (value->isType(hsql::ExprType::kExprLiteralInt)) {
                arrays.AddValue(Datum{value->ival});
            } else if (value->isType(hsql::ExprType::kExprLiteralNull)) {
                arrays.AddNull();
            } else {
                throw std::runtime_error("Expression in INSERT is not supported type");
            }
        }

        auto [values, isnull] = arrays.Decompose();
        auto tuple = std::make_unique<executor::VirtualTuple>(std::move(values), std::move(isnull));
        auto table = DatabaseGlobal->OpenTable(schema::catalog::TableId::MainTableId);
        auto vec = std::vector<std::unique_ptr<access::table::ITuple>>{};
        vec.emplace_back(std::move(tuple));
        return std::make_unique<executor::plan::InsertNode>(table, std::move(vec));
    } else if (statement.is(hsql::StatementType::kStmtUpdate)) {
        hsql::UpdateStatement &stmt = dynamic_cast<hsql::UpdateStatement &>(statement);
        auto table = DatabaseGlobal->OpenTable(schema::catalog::TableId::MainTableId);
        const auto &info = DatabaseGlobal->GetSchema()->GetTableInfo(schema::catalog::MainTableId);

        // attribute updates
        auto updates = std::vector<
            std::pair<access::table::AttrNumber, std::unique_ptr<executor::IExpressionNode>>>{};
        for (const auto update : *stmt.updates) {
            auto colname = std::string{update->column};
            auto colinfo = info.FindColumn(colname);
            if (!colinfo) {
                throw std::runtime_error("could not find column: " + colname);
            }

            auto expr = parse_expr(*update->value);
            updates.emplace_back(std::make_pair(colinfo->AttrNumber(), std::move(expr)));
        }

        // predicate
        auto qual = stmt.where ? parse_expr(*stmt.where) : nullptr;
        return std::make_unique<executor::plan::UpdateNode>(table, std::move(qual),
                                                            std::move(updates));
    } else if (statement.is(hsql::StatementType::kStmtDelete)) {
        hsql::DeleteStatement &stmt = dynamic_cast<hsql::DeleteStatement &>(statement);
        auto table = DatabaseGlobal->OpenTable(schema::catalog::TableId::MainTableId);
        auto qual = stmt.expr ? parse_expr(*stmt.expr) : nullptr;
        return std::make_unique<executor::plan::DeleteNode>(table, std::move(qual));
    } else if (statement.is(hsql::StatementType::kStmtTransaction)) {
        // TCL должен выполнятся отдельно (пока без всяких PlannedStmt)
        throw std::runtime_error("TCL must not reach here");
    } else {
        throw std::runtime_error("statement is not supported: " + std::to_string(static_cast<int>(statement.type())));
    }
}

bool Planner::IsPlannableStatement(hsql::SQLStatement &statement) {
    switch (statement.type()) {
    case hsql::kStmtSelect:
    case hsql::kStmtUpdate:
    case hsql::kStmtDelete:
    case hsql::kStmtInsert:
        return true;
    default:
        return false;
    }
}
