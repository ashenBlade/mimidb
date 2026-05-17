#include "db/Schema.hpp"
#include "db/catalog/OperatorInfo.hpp"
#include "db/catalog/TableInfo.hpp"
#include "executor/Oid.hpp"
#include <stdexcept>
#include <tuple>

using namespace mi::db;

Schema::Schema(std::unordered_map<Oid, std::unique_ptr<catalog::TableInfo>> &&tables,
    std::unordered_map<Oid, std::unique_ptr<catalog::TypeInfo>> &&types,
    std::unordered_map<Oid, std::unique_ptr<catalog::OperatorInfo>> &&operators)
    : _tables(std::move(tables)), _types(std::move(types)), _operators(std::move(operators)), _signatureToOperatorIdx() {
    
    // build indexes
    for (const auto &[id, opinfo] : this->_operators) {
        auto key = std::make_tuple(opinfo->LeftType(), opinfo->RightType(), opinfo->Strategy());
        this->_signatureToOperatorIdx.emplace(key, opinfo.get());
    }
};

template <class T>
static const T &map_get_entry(mi::Oid id, const std::unordered_map<mi::Oid, std::unique_ptr<T>> &map) {
    auto it = map.find(id);
    if (it != map.end()) {
        return *it->second.get();
    }

    throw std::runtime_error("could not find entry with provided id");
}

const catalog::TableInfo &Schema::GetTableInfo(Oid tableId) const {
    return map_get_entry(tableId, this->_tables);
}

const catalog::TypeInfo &Schema::GetTypeInfo(Oid typeId) const {
    return map_get_entry(typeId, this->_types);
}

const catalog::OperatorInfo *Schema::FindOperatorInfo(Oid leftType, Oid rightType, catalog::OperatorStrategy strategy) const {
    auto key = std::make_tuple(leftType, rightType, strategy);
    auto it = this->_signatureToOperatorIdx.find(key);
    if (it == this->_signatureToOperatorIdx.end()) {
        return nullptr;
    }

    return it->second;
}
