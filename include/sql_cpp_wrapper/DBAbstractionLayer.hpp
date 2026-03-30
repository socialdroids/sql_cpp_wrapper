#ifndef INCLUDE_INCLUDE_DBABSTRACTIONLAYER_HPP_
#define INCLUDE_INCLUDE_DBABSTRACTIONLAYER_HPP_

#include <any>
#include <cppdb/driver_manager.h>
#include <cppdb/errors.h>
#include <cppdb/frontend.h>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

// INFO:
// short -> bool
// Não existe fetch(bool arg) no cppdb
// ---
// short = int16_t
// int = int32_t
// using SQLData = std::variant<std::tm, std::string, int16_t, int32_t, float,
//                              double, int64_t, std::vector<uint8_t>>;
// using SQLConditionMap = std::unordered_map<std::string, SQLData>;
typedef std::variant<std::tm, std::string, int16_t, int32_t, float, double,
                     int64_t, std::vector<uint8_t>>
    SQLData;
typedef std::unordered_map<std::string, SQLData> SQLConditionMap;

enum class SQLDataType
{
    Int,      // int
    DateTime, // std::tm
    String,   // std::string
    Short,    // short
    Blob,     // std::vector<uint8_t>
    //
    SmallInt,    // int16_t
    Integer,     // int32_t
    BigInt,      // int64_t
    Boolean,     // short
    Real,        // float
    Double,      // double
    Time,        // std::string
    TimestampTZ, // std::tm
    Text,        // std::string
};

inline std::ostream& operator<<(std::ostream& os, const SQLData& v)
{
    std::visit(
        [&os](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::vector<uint8_t>>)
            {
                os << "[BLOB data: " << arg.size() << " bytes]";
            }
            else
            {
                os << arg;
            }
        },
        v);
    return os;
}

class SQLSchema
{
public:
    SQLSchema(const std::string _table) : tableName(_table)
    {
    }

    ~SQLSchema()
    {
    }

    enum class InsertBehavior : bool
    {
        kUse,
        kIgnore
    };

    enum class UpdateBehavior : bool
    {
        kUse,
        kIgnore
    };

    typedef struct Column
    {
        std::string name;
        InsertBehavior insert;
        UpdateBehavior update;
        SQLDataType type;
    } Column_t;

    const std::string& table() const
    {
        return tableName;
    };

    void addColumn(std::string const& _name, SQLDataType _type,
                   InsertBehavior _insert, UpdateBehavior _update)
    {
        columns.push_back({_name, _insert, _update, _type});
    }

    const std::vector<Column_t>& getColumns() const
    {
        return columns;
    }

    std::vector<std::string> getColumnsNames() const
    {
        std::vector<std::string> out;
        for (Column_t const& col : columns)
        {
            out.push_back(col.name);
        }

        return out;
    }

    void updateColumn(std::string const& _col, InsertBehavior _insert,
                      UpdateBehavior _update)
    {
        for (Column_t& col : columns)
        {
            if (col.name == _col)
            {
                col.insert = _insert;
                col.update = _update;
            }
        }
    }

private:
    std::string tableName;
    std::vector<Column_t> columns;
};

class SQLEntry
{
public:
    void setValue(const std::string& columnName, SQLData value)
    {
        data[columnName] = value;
    }

    SQLData getValue(const std::string& columnName) const
    {
        auto it = data.find(columnName);
        if (it != data.end())
        {
            return it->second;
        }
        throw std::invalid_argument("Undefined column: " + columnName);
    }

    bool hasColumn(const std::string& columnName) const
    {
        return data.find(columnName) != data.end();
    }

    bool toBool(const std::string& columnName)
    {
        return std::get<std::string>(this->getValue(columnName)).find("t") !=
               std::string::npos;
    }

private:
    std::unordered_map<std::string, SQLData> data;
};

class SQLManager
{
public:
    SQLManager()
    {
        cppdb::driver_manager::instance().add_search_path(".");
        currentSession = std::make_unique<cppdb::session>();
    }
    ~SQLManager()
    {
    }

    bool init(std::string _dbname, std::string _user, std::string _pwd,
              std::string _host = "localhost")
    {
        std::stringstream config;
        config << "postgresql:dbname=" << _dbname << ";user=" << _user
               << ";password=" << _pwd << ";host=" << _host;

        try
        {
            currentSession->open(config.str());
        }
        catch (cppdb::cppdb_error const& e)
        {
            std::cerr << "Connection Error: " << e.what() << std::endl;
        }

        return currentSession->is_open();
    }

    bool insert(SQLSchema* _schema, SQLEntry* _entry)
    {
        std::string columnsStr = "";
        std::string valuesStr = "";
        std::vector<SQLData> bindValues;

        bool first = true;
        for (const auto& col : _schema->getColumns())
        {
            // Insere apenas se o schema permite e se o usuário forneceu o dado
            if (col.insert == SQLSchema::InsertBehavior::kUse &&
                _entry->hasColumn(col.name))
            {
                if (!first)
                {
                    columnsStr += ", ";
                    valuesStr += ", ";
                }
                columnsStr += col.name;
                valuesStr += "?";
                bindValues.push_back(_entry->getValue(col.name));
                first = false;
            }
        }

        if (columnsStr.empty())
        {
            std::cerr << "Insert aborted: No valid columns to insert.\n";
            return false;
        }

        std::string statement = "INSERT INTO " + _schema->table() + " (" +
                                columnsStr + ") VALUES (" + valuesStr + ")";
        prepare(statement);

        for (const auto& val : bindValues)
        {
            if (!bindAny(val))
            {
                std::cerr << "Failed to bind value to insert statement!\n";
                return false;
            }
        }

        int affected = run();
        if (affected <= 0)
        {
            std::cerr << "Insert failed!\n";
            return false;
        }

        return affected > 0;
    }

    bool update(SQLSchema* _schema, SQLEntry* _entry,
                const std::unordered_map<std::string, SQLData>& conditions)
    {
        if (conditions.empty())
        {
            std::cerr << "Update aborted: conditions map is empty (prevents "
                         "accidental full table update).\n";
            return false;
        }

        std::string statement = "UPDATE " + _schema->table() + " SET ";
        std::vector<SQLData> bindValues;

        // Build SET
        bool firstSet = true;
        for (const auto& col : _schema->getColumns())
        {
            // Atualiza apenas se a coluna permite update e o SQLEntry contém
            // esse dado
            if (col.update == SQLSchema::UpdateBehavior::kUse &&
                _entry->hasColumn(col.name))
            {
                if (!firstSet)
                {
                    statement += ", ";
                }
                statement += col.name + "=?";
                bindValues.push_back(_entry->getValue(col.name));
                firstSet = false;
            }
        }

        // Build WHERE
        statement += " WHERE ";
        bool firstWhere = true;
        for (const auto& [colName, val] : conditions)
        {
            if (!firstWhere)
            {
                statement += " AND ";
            }
            statement += colName + "=?";
            bindValues.push_back(val);
            firstWhere = false;
        }

        prepare(statement);
        for (const SQLData& val : bindValues)
        {
            if (!bindAny(val))
            {
                std::cerr << "Failed to bind value to update statement!\n";
                return false;
            }
        }
        int affected = run();
        if (affected == 0)
        {
            std::cerr << "Update skipped: Entry not found or conditions didn't "
                         "match.\n";
            return false;
        }

        return affected > 0;
    }

    bool remove(SQLSchema* _schema,
                const std::unordered_map<std::string, SQLData>& conditions)
    {
        if (conditions.empty())
        {
            std::cerr << "Remove aborted: conditions map is empty (prevents "
                         "accidental full table drop).\n";
            return false;
        }

        std::string statement = "DELETE FROM " + _schema->table() + " WHERE ";
        std::vector<SQLData> bindValues;

        bool first = true;
        for (const auto& [colName, val] : conditions)
        {
            if (!first)
                statement += " AND ";
            statement += colName + "=?";
            bindValues.push_back(val);
            first = false;
        }

        prepare(statement);
        for (const auto& val : bindValues)
        {
            if (!bindAny(val))
            {
                std::cerr << "Failed to bind value to delete statement!\n";
                return false;
            }
        }

        int affected = run();
        if (affected == 0)
        {
            std::cerr << "Delete skipped: Entry not found or conditions didn't "
                         "match.\n";
            return false;
        }

        return affected > 0;
    }

    std::vector<SQLEntry> get(SQLSchema* _schema,
                              const std::vector<std::string>& columnsToSelect,
                              const SQLConditionMap& conditions)
    {
        std::string selectCols = "";

        // Build SELECT
        if (columnsToSelect.empty())
        {
            selectCols = "*";
        }
        else
        {
            bool firstCol = true;
            for (const auto& c : columnsToSelect)
            {
                if (!firstCol)
                    selectCols += ", ";
                selectCols += c;
                firstCol = false;
            }
        }

        std::string statement =
            "SELECT " + selectCols + " FROM " + _schema->table();
        std::vector<SQLData> bindValues;

        // Build WHERE
        if (!conditions.empty())
        {
            statement += " WHERE ";
            bool firstCond = true;
            for (const auto& [colName, val] : conditions)
            {
                if (!firstCond)
                    statement += " AND ";
                statement += colName + "=?";
                bindValues.push_back(val);
                firstCond = false;
            }
        }

        return customQuery(statement, bindValues, _schema);
    }

    std::vector<SQLEntry> getAllEntries(SQLSchema* _schema)
    {
        return get(_schema, {}, {});
    }

    cppdb::statement& lastStatement()
    {
        return stat;
    }

    /**
     * @brief Realiza uma query customizável
     *
     * Exemplo:
     * @code
     *  std::vector<SQLEntry> cEntry = dbManager.customQuery(
     *      "SELECT ssid,password FROM wifi_profiles WHERE ssid=?",
     *      {std::string("SD")}, &schema);
     *  if (not cEntry.empty())
     *  {
     *      std::cout << "\tSSID: " << cEntry.at(0).getValue("ssid")
     *                << ", PWD: " << cEntry.at(0).getValue("password") << "\n";
     *  }
     * @endcode
     *
     * @param _queryStr SQL query.
     * @param _params Parâmetros para os placeholders.
     * @param _schema
     * @return Array com os resultados da query.
     */
    std::vector<SQLEntry> customQuery(const std::string& _queryStr,
                                      const std::vector<SQLData>& _params,
                                      SQLSchema* _schema)
    {
        std::vector<SQLEntry> entries;
        prepare(_queryStr);

        for (const SQLData& param : _params)
        {
            if (!bindAny(param))
            {
                std::cerr << "Failed to bind parameter to custom query!\n";
                return entries;
            }
        }

        cppdb::result res;
        try
        {
            res = stat.query();
        }
        catch (cppdb::cppdb_error const& e)
        {
            std::cerr << "Custom Query ERROR: " << e.what() << std::endl;
            stat.reset();
            return entries;
        }

        // Verify returned cols
        int num_cols = res.cols();
        std::vector<std::string> returned_cols;
        for (int i = 0; i < num_cols; ++i)
        {
            returned_cols.push_back(res.name(i));
        }

        // Map types
        std::unordered_map<std::string, SQLDataType> typeMap;
        for (const auto& col : _schema->getColumns())
        {
            typeMap[col.name] = col.type;
        }

        while (res.next())
        {
            SQLEntry data;
            for (const std::string& colName : returned_cols)
            {
                auto it = typeMap.find(colName);
                if (it == typeMap.end())
                {
                    std::cerr << "Warning: Returned column '" << colName
                              << "' is not defined in Schema.\n";
                    continue;
                }

                if (res.is_null(colName))
                {
                    std::cerr << "Column: " << colName << " is Null\n";
                    continue;
                }

                try
                {
                    switch (it->second)
                    {
                    case SQLDataType::Int:
                        data.setValue(colName, res.get<int>(colName));
                        break;
                    case SQLDataType::TimestampTZ:
                    case SQLDataType::DateTime:
                        data.setValue(colName, res.get<std::tm>(colName));
                        break;
                    case SQLDataType::Boolean:
                    case SQLDataType::Time:
                    case SQLDataType::String:
                    case SQLDataType::Text:
                        data.setValue(colName, res.get<std::string>(colName));
                        break;
                    case SQLDataType::Short:
                        data.setValue(colName, res.get<short>(colName));
                        break;
                    case SQLDataType::Blob: {
                        std::string blobStr = res.get<std::string>(colName);
                        std::vector<uint8_t> blobData(blobStr.begin(),
                                                      blobStr.end());
                        data.setValue(colName, blobData);
                        break;
                    }
                    case SQLDataType::SmallInt:
                        data.setValue(colName, res.get<int16_t>(colName));
                        break;
                    case SQLDataType::Integer:
                        data.setValue(colName, res.get<int32_t>(colName));
                        break;
                    case SQLDataType::BigInt:
                        data.setValue(colName, res.get<int64_t>(colName));
                        break;
                    case SQLDataType::Real:
                        data.setValue(colName, res.get<float>(colName));
                        break;
                    case SQLDataType::Double:
                        data.setValue(colName, res.get<double>(colName));
                        break;
                    }
                }
                catch (const cppdb::bad_value_cast& e)
                {
                    std::cerr << "Column conversion error for " << colName
                              << ": " << e.what()
                              << "\nValue: " << res.get<std::string>(colName);
                    // for (uint8_t u : res.get<std::ostream>>(colName))
                    // {
                    //     std::cerr << u << ", ";
                    // }
                    std::cerr << "\n";
                }
            }
            entries.push_back(data);
        }

        stat.reset();
        return entries;
    }

private:
    std::unique_ptr<cppdb::session> currentSession;
    cppdb::statement stat;

    void prepare(std::string const& _statement)
    {
        // std::cout << "\t-- Preparing <" << _statement << ">\n";
        if (!stat.empty())
        {
            stat.reset();
        }
        stat = currentSession->create_statement(_statement);
    }

    bool bindAny(SQLData const& _value)
    {
        try
        {
            std::visit(
                [this](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::vector<uint8_t>>)
                    {
                        std::string blobStr(arg.begin(), arg.end());
                        this->stat.bind(blobStr);
                    }
                    else
                    {
                        this->stat.bind(arg);
                    }
                },
                _value);
            return true;
        }
        catch (const cppdb::cppdb_error& e)
        {
            std::cerr << "Bind error: " << e.what() << "\n";
            return false;
        }
    }

    cppdb::result runQuery(SQLData _arg)
    {
        SQLData dummy = 0;
        bindAny(_arg);
        return runQuery();
    }

    cppdb::result runQuery()
    {
        try
        {
            cppdb::result res = stat.query();
            // std::cout << "ID: " << stat.last_insert_id() << "\n";
            // std::cout << "Affected rows: " << stat.affected() << "\n";

            stat.reset();
            return res;
        }
        catch (cppdb::cppdb_error const& e)
        {
            std::cerr << "Statement Query ERROR: " << e.what() << std::endl;
            return cppdb::result();
        }
    }

    int run()
    {
        try
        {
            stat.exec();
            int affectedRows = stat.affected();
            return affectedRows;
        }
        catch (cppdb::cppdb_error const& e)
        {
            std::cerr << "Statement Run ERROR: " << e.what() << std::endl;
            return -1;
        }
    }
};

using namespace std;
class DBInterface
{
public:
    DBInterface(SQLManager* _manager, const string& _table)
        : db(_manager), sc(_table)
    {
    }

protected:
    SQLManager* db;
    SQLSchema sc;
};

#endif // INCLUDE_INCLUDE_DBABSTRACTIONLAYER_HPP_
