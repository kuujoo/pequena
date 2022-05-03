#include "pequena/database/sqlite.h"
#include "sqlite/sqlite3.h"
#include <assert.h>
#include <algorithm>
#include <memory>
#include <assert.h>
#include <cctype>

using namespace peq;
using namespace peq::database;

namespace
{
	const std::string s_beginTransaction = "BEGIN TRANSACTION;";
	const std::string s_rollbackTransaction = "ROLLBACK;";
	const std::string s_commitTransaction = "COMMIT;";
}

Reflector::Reflector(const peq::database::SQLiteQueryResult* result) : _result(result)
{

}

SQLiteQueryResultRow::SQLiteQueryResultRow()
{
}

SQLiteQueryResultRow::SQLiteQueryResultRow(const SQLiteQueryResultRow& other)
{
	_values = other._values;
}

SQLiteQueryResultRow::SQLiteQueryResultRow(SQLiteQueryResultRow&& other) noexcept
{
	_values = std::move(other._values);
}

SQLiteQueryResultRow& SQLiteQueryResultRow::operator=(const SQLiteQueryResultRow& other)
{
	SQLiteQueryResultRow c(other);
	std::swap(*this, c);
	return *this;
}
SQLiteQueryResultRow& SQLiteQueryResultRow::operator=(SQLiteQueryResultRow&& other) noexcept
{
	_values = std::move(other._values);
	return *this;
}

void SQLiteQueryResultRow::add(const std::string& value)
{
	_values.push_back(value);
}

SQLiteQueryResult::SQLiteQueryResult()
{
}

SQLiteQueryResult::SQLiteQueryResult(const SQLiteQueryResult& other)
{
	_rows = other._rows;
	_columnIndices = other._columnIndices;
}

SQLiteQueryResult::SQLiteQueryResult(SQLiteQueryResult&& other) noexcept
{
	_rows = std::move(other._rows);
	_columnIndices = std::move(other._columnIndices);
}

SQLiteQueryResult& SQLiteQueryResult::operator=(const SQLiteQueryResult& other)
{
	SQLiteQueryResult c(other);
	std::swap(*this, c);
	return *this;
}

SQLiteQueryResult& SQLiteQueryResult::operator=(SQLiteQueryResult&& other) noexcept
{
	_rows = std::move(other._rows);
	_columnIndices = std::move(other._columnIndices);
	return *this;
}

const SQLiteQueryResultRow& SQLiteQueryResult::operator[](unsigned index) const
{
	return _rows[index];
}

unsigned SQLiteQueryResult::columnCount() const
{
	return _columnIndices.size();
}

unsigned SQLiteQueryResult::rowCount() const
{
	return _rows.size();
}


void SQLiteQueryResult::addRow(SQLiteQueryResultRow&& row)
{
	_rows.push_back(std::forward<SQLiteQueryResultRow>(row));
}

bool SQLiteQueryResult::empty() const
{
	return _rows.empty();
}

unsigned SQLiteQueryResult::columnIndex(const std::string& name) const
{
	auto idx = _columnIndices.find(name);
	if (idx == _columnIndices.end())
	{
		return SQLiteQueryResult::invalidColumn;
	}
	return idx->second;
}

void SQLiteQueryResult::addColumn(const std::string& column)
{
	_columnIndices[column] = _columnIndices.size();
}

SQLiteQuery::SQLiteQuery(SQLiteConnection* connection) : _connection(connection)
{
}

SQLiteQuery::SQLiteQuery(SQLiteConnection* connection, const std::string& query) : _connection(connection), _query(query)
{

}

SQLiteQuery::SQLiteQuery(SQLiteQuery&& connection) noexcept
{
	_connection = connection._connection;
	_error = connection._error;
	_query = connection._query;
	connection._connection = nullptr;
	connection._query = "";

}

SQLiteQuery& SQLiteQuery::operator=(SQLiteQuery&& connection) noexcept
{
	_connection = connection._connection;
	_error = connection._error;
	_query = connection._query;
	connection._connection = nullptr;
	connection._query = "";
	return *this;
}

void SQLiteQuery::set(const std::string& query)
{
	_query = query;
}

SQLiteQueryError SQLiteQuery::lastError() const
{
	return _error;
}


void SQLiteQuery::bind(const std::string& key, const std::string& value)
{
	auto idx = _query.find(key);
	if (idx == std::string::npos) return;
	_parameters.push_back(Parameter(idx, value));
}

void SQLiteQuery::bind(const std::string& key, int value)
{
	auto idx = _query.find(key);
	if (idx == std::string::npos) return;
	_parameters.push_back(Parameter(idx, value));
}

void SQLiteQuery::bind(const std::string& key, int64_t value)
{
	auto idx = _query.find(key);
	if (idx == std::string::npos) return;
	_parameters.push_back(Parameter(idx, value));
}

void SQLiteQuery::bind(const std::string& key, double value)
{
	auto idx = _query.find(key);
	if (idx == std::string::npos) return;
	_parameters.push_back(Parameter(idx, value));
}


bool SQLiteQuery::executeNoResult()
{
	if (!_parameters.empty())
	{
		std::sort(_parameters.begin(), _parameters.end(), [](const Parameter& v0, const Parameter& v1)->bool {
			return v0.idx < v1.idx;
		});
	}

	if (_connection->queryNoResult(_query, _parameters))
	{
		_error = SQLiteQueryError();
		return true;
	}
	else
	{
		_error = SQLiteQueryError(_connection->lastErrorText());
		return false;
	}
}

std::optional<SQLiteQueryResult> SQLiteQuery::execute()
{
	if (!_parameters.empty())
	{
		std::sort(_parameters.begin(), _parameters.end(), [](const Parameter& v0, const Parameter& v1)->bool {
			return v0.idx < v1.idx;
		});
	}

	SQLiteQueryResult result;
	auto rowFunc = [&result](int argc, const char** values, const char** colNames)->int {
		if (result.columnCount() != argc)
		{
			for (int i = 0; i < argc; i++)
			{
				result.addColumn(colNames[i]);
			}
		}
		SQLiteQueryResultRow row;
		for (int i = 0; i < argc; i++)
		{
			if (values[i] == nullptr)
			{
				row.add("");
				continue;
			}
			row.add(values[i]);
		}
		result.addRow(std::move(row));
		return 0;
	};

	if (_connection->query(_query, _parameters, rowFunc))
	{
		_error = SQLiteQueryError();
		return result;
	}
	else
	{
		_error = SQLiteQueryError(_connection->lastErrorText());
		return std::optional<SQLiteQueryResult>();
	}
}

SQLiteTransaction::SQLiteTransaction(SQLiteConnection* connection) : _connection(connection), _transactionOpen(false)
{
}

SQLiteTransaction::SQLiteTransaction(SQLiteTransaction&& connection) noexcept
{
	_connection = connection._connection;
	connection._connection = nullptr;
	_transactionOpen = connection._transactionOpen;
}

SQLiteTransaction& SQLiteTransaction::operator=(SQLiteTransaction&& connection) noexcept
{
	_connection = connection._connection;
	connection._connection = nullptr;
	_transactionOpen = connection._transactionOpen;
	return *this;
}

bool SQLiteTransaction::begin()
{
	if (_transactionOpen) return false;

	SQLiteQuery q(_connection, s_beginTransaction);
	_transactionOpen = true;
	return q.executeNoResult();
}

bool SQLiteTransaction::commit()
{
	if (!_transactionOpen) return false;

	SQLiteQuery q(_connection, s_commitTransaction);
	_connection = nullptr;
	_transactionOpen = false;
	return q.executeNoResult();
}

bool SQLiteTransaction::rollback()
{
	if (!_transactionOpen) return false;

	SQLiteQuery q(_connection, s_rollbackTransaction);
	_connection = nullptr;
	_transactionOpen = false;
	return q.executeNoResult();
}

// sqlite things

SQLiteConnection::SQLiteConnection(const std::string& db) : _db(nullptr), _dbName(db)
{
}

SQLiteConnection::~SQLiteConnection()
{
	close();
}

SQLiteConnection::SQLiteConnection(SQLiteConnection&& other) noexcept
{
	_dbName = std::move(other._dbName);
	_db = other._db;
	other._db = nullptr;
}

SQLiteConnection& SQLiteConnection::operator=(SQLiteConnection&& other) noexcept
{
	_dbName = std::move(other._dbName);
	_db = other._db;
	other._db = nullptr;
	return *this;
}

bool SQLiteConnection::open()
{
	if (_db != nullptr)
	{
		assert(0);
		close();
	}
	auto r = sqlite3_open(_dbName.c_str(), &_db);
	return r == SQLITE_OK;
}

bool SQLiteConnection::close()
{
	auto rt = sqlite3_close(_db);
	_db = nullptr;
	return rt == SQLITE_OK;
}

class SQLiteStatement
{
public:
	SQLiteStatement(sqlite3* db) : _db(db), _statement(nullptr){
	}
	~SQLiteStatement()
	{
		sqlite3_finalize(_statement);
	}
	bool prepare(const char* query, const char** tail)
	{
		if (_statement) return false;

		if (sqlite3_prepare_v2(_db, query, -1, &_statement, tail) != SQLITE_OK)
		{
			sqlite3_finalize(_statement);
			_statement = nullptr;
			return false;
		}
		else
		{
			return true;
		}
	}
	int parameterCount() const
	{
		return sqlite3_bind_parameter_count(_statement);;
	}
	bool bind(int id, const Parameter& param)
	{
		if (id >= parameterCount())
		{
			return false;
		}

		auto& it = param;
		switch (it.type)
		{
			case Parameter::Type::String:
			{
				if (sqlite3_bind_text(_statement, id + 1, it.svalue.c_str(), it.svalue.size(), nullptr) != SQLITE_OK)
				{
					return false;
				}
				break;
			}
			case Parameter::Type::Int:
			{
				if (sqlite3_bind_int(_statement, id + 1, it.ivalue) != SQLITE_OK)
				{
					return false;
				}
				break;
			}
			case Parameter::Type::Int64:
			{
				if (sqlite3_bind_int64(_statement, id + 1, it.i64value) != SQLITE_OK)
				{
					return false;
				}
				break;
			}
			case Parameter::Type::Double:
			{
				if (sqlite3_bind_double(_statement, id + 1, it.dvalue) != SQLITE_OK)
				{
					return false;
				}
				break;
			}
		}
		return true;
	}
	bool executeNoResult()
	{
		return sqlite3_step(_statement) == SQLITE_DONE;
	}
	bool execute(std::function<int(int, const char** colValues, const char** colNames)> rowFunc)
	{
		auto st = sqlite3_step(_statement);
		auto cols = sqlite3_column_count(_statement);
		const char* values[SQLiteConnection::maxQueryColumns] = { nullptr };
		const char* names[SQLiteConnection::maxQueryColumns] = { nullptr };

		while (st == SQLITE_ROW)
		{
			for (int i = 0; i < cols; i++)
			{
				names[i] = sqlite3_column_name(_statement, i);
				values[i] = (const char*)sqlite3_column_text(_statement, i);
			}

			rowFunc(cols, values, names);
			st = sqlite3_step(_statement);
		}
		return st == SQLITE_DONE;
	}
private:
	sqlite3* _db;
	sqlite3_stmt* _statement;
};

bool SQLiteConnection::query(const std::string& query, const std::vector<Parameter>& parameters, std::function<int(int, const char** colValues, const char** colNames)> rowFunc)
{
	if (query.empty()) return false;

	auto queryPtr = query.c_str();
	auto querySize = query.size();
	const char* queryTail = queryPtr;

	int parameterOffset = 0;

	while (std::isspace(*queryPtr)) {
		queryPtr++;
	}

	while (*queryPtr)
	{
		SQLiteStatement statement(_db);

		if (!statement.prepare(queryPtr, &queryTail))
		{
			return false;
		}

		auto parameterCount = statement.parameterCount();
		for (int i = 0; i < parameterCount; i++)
		{
			if (i >= parameters.size())
			{
				return false;
			}

			if (!statement.bind(i, parameters[parameterOffset]))
			{
				return false;
			}
			parameterOffset++;
		}


		if (!statement.execute(rowFunc))
		{
			return false;
		}

		queryPtr = queryTail;
		while (std::isspace(*queryPtr)) {
			queryPtr++;
		}
	}

	return true;
}

bool SQLiteConnection::queryNoResult(const std::string& query, const std::vector<Parameter>& parameters)
{
	if (query.empty()) return false;

	auto queryPtr = query.c_str();
	auto querySize = query.size();
	const char* queryTail = queryPtr;

	int parameterOffset = 0;
	while (std::isspace(*queryPtr)) {
		queryPtr++;
	}
	while (*queryPtr)
	{
		SQLiteStatement statement(_db);

		if (!statement.prepare(queryPtr, &queryTail))
		{
			return false;
		}

		auto parameterCount = statement.parameterCount();
		for (int i = 0; i < parameterCount; i++)
		{
			if (!statement.bind(i, parameters[parameterOffset]))
			{
				return false;
			}
			parameterOffset++;
		}

		
		if (!statement.executeNoResult())
		{
			return false;
		}

		queryPtr = queryTail;
		while (std::isspace(*queryPtr)) {
			queryPtr++;
		}
	}

	return true;
}

std::string SQLiteConnection::lastErrorText() const
{
	return std::string(sqlite3_errmsg(_db));;
}
