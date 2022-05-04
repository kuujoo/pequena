#pragma once

#include "reflector.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <type_traits>
#include <functional>
#include <optional>
#include <assert.h>

class sqlite3_stmt;
class sqlite3;

namespace peq
{
	namespace database
	{
		class SQLiteConnection;
		class SQLiteQueryResult;

		class Reflector
		{
		public:
			Reflector(const peq::database::SQLiteQueryResult* result);
			template<typename T>
			T first();
			template<typename T>
			std::vector<T> all();
		private:
			const peq::database::SQLiteQueryResult* _result;
		};

		class SQLiteQueryResultRow
		{
		public:
			SQLiteQueryResultRow();
			SQLiteQueryResultRow(const SQLiteQueryResultRow&);
			SQLiteQueryResultRow(SQLiteQueryResultRow&&) noexcept;
			SQLiteQueryResultRow& operator=(const SQLiteQueryResultRow&);
			SQLiteQueryResultRow& operator=(SQLiteQueryResultRow&&) noexcept;

			template<typename T>
			T get(unsigned index) const;
			
		private:
			friend class SQLiteQuery;
			void add(const std::string& value);
			std::vector<std::string> _values;
		};


		template<>
		inline unsigned SQLiteQueryResultRow::get(unsigned index) const
		{
			return static_cast<unsigned>(std::stoul(_values[index]));
		}

		template<>
		inline int SQLiteQueryResultRow::get(unsigned index) const
		{
			return std::stoi(_values[index]);
		}

		template<>
		inline long SQLiteQueryResultRow::get(unsigned index) const
		{
			return std::stol(_values[index]);
		}

		template<>
		inline unsigned long SQLiteQueryResultRow::get(unsigned index) const
		{
			return std::stoul(_values[index]);
		}

		template<>
		inline float SQLiteQueryResultRow::get(unsigned index) const
		{
			return std::stof(_values[index]);
		}

		template<>
		inline double SQLiteQueryResultRow::get(unsigned index) const
		{
			return std::stod(_values[index]);
		}
		template<>
		inline int64_t SQLiteQueryResultRow::get(unsigned index) const
		{
			return std::stoll(_values[index]);
		}
		template<>
		inline uint64_t SQLiteQueryResultRow::get(unsigned index) const
		{
			return std::stoull(_values[index]);
		}
		template<>
		inline std::string SQLiteQueryResultRow::get(unsigned index) const
		{
			return _values[index];
		}

		class SQLiteQueryResult
		{
		public:
			static const unsigned invalidColumn = 99999U;

			SQLiteQueryResult();
			SQLiteQueryResult(const SQLiteQueryResult&);
			SQLiteQueryResult(SQLiteQueryResult&&) noexcept;
			SQLiteQueryResult& operator=(const SQLiteQueryResult&);
			SQLiteQueryResult& operator=(SQLiteQueryResult&&) noexcept;
			const SQLiteQueryResultRow& operator[](unsigned) const;

			unsigned columnCount() const;
			unsigned rowCount() const;
			unsigned columnIndex(const std::string& name) const;
			bool empty() const;

			std::vector<SQLiteQueryResultRow>::const_iterator begin() const noexcept
			{
				return _rows.begin();
			}
			std::vector<SQLiteQueryResultRow>::const_iterator end() const noexcept
			{
				return _rows.end();
			}
			std::vector<SQLiteQueryResultRow>::iterator begin() noexcept
			{
				return _rows.begin();
			}
			std::vector<SQLiteQueryResultRow>::iterator end() noexcept
			{
				return _rows.end();
			}

			template<typename T>
			T first();

			template<typename T>
			std::vector<T> all();
		private:
			friend class SQLiteQuery;
			void addColumn(const std::string& column);
			void addRow(SQLiteQueryResultRow&& row);
			std::vector<SQLiteQueryResultRow> _rows;
			std::unordered_map<std::string, unsigned> _columnIndices;
		};

		template<typename T>
		T Reflector::first()
		{
			T r;
			r.peq_reflectorFromDbRow(_result, 0);
			return r;
		}
		template<typename T>
		std::vector<T> Reflector::all()
		{
			std::vector<T> results;
			for (unsigned i = 0; i < _result->rowCount(); i++)
			{
				T r;
				r.peq_reflectorFromDbRow(_result, i);
				results.push_back(std::move(r));
			}

			return results;
		}


		template<typename T>
		T SQLiteQueryResult::first()
		{
			Reflector reflector(this);
			return reflector.first<T>();
		}

		template<typename T>
		std::vector<T> SQLiteQueryResult::all()
		{
			Reflector reflector(this);
			return reflector.all<T>();
		}

		struct SQLiteQueryError
		{
			SQLiteQueryError() {}
			SQLiteQueryError(const std::string& text) : text(text) {}
			std::string text;
		};


		struct Parameter
		{
			enum class Type
			{
				String,
				Int,
				Int64,
				Double
			};
			Parameter(unsigned idx, const std::string& value) : type(Type::String), idx(idx), svalue(value) {}
			Parameter(unsigned idx, int value) : type(Type::Int), idx(idx), ivalue(value) {}
			Parameter(unsigned idx, int64_t value) : type(Type::Int64), idx(idx), i64value(value) {}
			Parameter(unsigned idx, double value) : type(Type::Double), idx(idx), dvalue(value) {}
			Parameter(unsigned idx, float value) : type(Type::Double), idx(idx), dvalue(value) {}
			~Parameter() {}
			Type type;
			unsigned int idx;
			std::string svalue;
			int64_t i64value;
			double dvalue;
			int ivalue;
		};

		class SQLiteQuery
		{
		public:
			SQLiteQuery(SQLiteConnection* connection);
			SQLiteQuery(SQLiteConnection* connection, const std::string& query);
			SQLiteQuery(SQLiteQuery&& connection) noexcept;
			SQLiteQuery& operator=(SQLiteQuery&& connection) noexcept;
			SQLiteQuery& operator=(const SQLiteQuery&) = delete;
			SQLiteQuery(const SQLiteQuery&) = delete;
			void set(const std::string& query);
			std::optional<SQLiteQueryResult> execute();
			bool executeNoResult();
			SQLiteQueryError lastError() const;
			// Prepared statement
			void bind(const std::string& key, const std::string& value);
			void bind(const std::string& key, int value);
			void bind(const std::string& key, int64_t value);
			void bind(const std::string& key, double value);
		private:
			std::vector<Parameter> _parameters;
			std::string _query;
			SQLiteConnection* _connection;
			SQLiteQueryError _error;
		};

		class SQLiteTransaction
		{
		public:
			SQLiteTransaction(SQLiteConnection* connection);
			SQLiteTransaction(SQLiteTransaction&& connection) noexcept;
			SQLiteTransaction& operator=(SQLiteTransaction&& connection) noexcept;
			SQLiteTransaction& operator=(const SQLiteTransaction&) = delete;
			SQLiteTransaction(const SQLiteTransaction&) = delete;
			bool begin();
			bool commit();
			bool rollback();
		private:
			SQLiteConnection* _connection;
			bool _transactionOpen;
		};

		class SQLiteConnection
		{
		public:
			static const unsigned maxQueryColumns = 32U;
			SQLiteConnection(const std::string& db);
			~SQLiteConnection();

			SQLiteConnection(SQLiteConnection&&) noexcept;
			SQLiteConnection& operator=(SQLiteConnection&&) noexcept;

			SQLiteConnection(const SQLiteConnection&) = delete;
			SQLiteConnection& operator=(const SQLiteConnection&) = delete;

			bool open();
			bool close();
		protected:
			bool query(const std::string& query, const std::vector<Parameter>& parameters, std::function<int(int, const char**, const char**)> rowFunc);//argc, argv, colname
			bool queryNoResult(const std::string& query, const std::vector<Parameter>& parameters);
			std::string lastErrorText() const;
			std::string _dbName;
			sqlite3* _db;
			friend class SQLiteQuery;
			friend class SQLiteTransaction;
		};
	}
}
