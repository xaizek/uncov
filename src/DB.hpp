// Copyright (C) 2016 xaizek <xaizek@openmailbox.org>
//
// This file is part of uncov.
//
// uncov is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// uncov is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with uncov.  If not, see <http://www.gnu.org/licenses/>.

#ifndef UNCOVER__DB_HPP__
#define UNCOVER__DB_HPP__

#include <boost/range.hpp>
#include <boost/variant.hpp>

#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

class Binding;
class Transaction;

class DB
{
    class Row;
    class RowWrapper;
    class RowIterator;
    class RowsData;
    class Rows;

    using stmtPtr = std::unique_ptr<sqlite3_stmt,
                                    std::function<void(sqlite3_stmt *)>>;
    using RowsBase = boost::iterator_range<RowIterator>;

public:
    explicit DB(const std::string &path);

    ~DB();

public:
    void execute(const std::string &stmt,
                 const std::vector<Binding> &binds = {});
    RowWrapper queryOne(const std::string &stmt,
                        const std::vector<Binding> &binds = {});
    Rows queryAll(const std::string &stmt,
                  const std::vector<Binding> &binds = {});
    std::int64_t getLastRowId();
    Transaction makeTransaction();

private:
    stmtPtr prepare(const std::string &stmt, const std::vector<Binding> &binds);

private:
    sqlite3 *conn;
};

class DB::Row
{
    // Type marker for overload resolution.
    template <typename T> struct Marker {};

public:
    Row(sqlite3_stmt *ps) : ps(ps)
    {
    }

public:
    template <typename... Types>
    operator std::tuple<Types...>()
    {
        const int nCols = getCountCount();
        if (nCols != sizeof...(Types)) {
            throw std::runtime_error {
                "Expected " + std::to_string(nCols) + " columns, got " +
                std::to_string(sizeof...(Types))
            };
        }

        return makeTuple<Types...>(std::index_sequence_for<Types...>());
    }

private:
    template <typename... Types, std::size_t... Is>
    std::tuple<Types...> makeTuple(std::index_sequence<Is...>)
    {
        return std::make_tuple(makeTupleItem(Is, Marker<Types>())...);
    }

    int getCountCount() const;

    std::string makeTupleItem(std::size_t idx, Marker<std::string>);
    int makeTupleItem(std::size_t idx, Marker<int>);
    std::vector<int> makeTupleItem(std::size_t idx, Marker<std::vector<int>>);

private:
    sqlite3_stmt *ps;
};

class DB::RowWrapper : public Row
{
public:
    RowWrapper(stmtPtr ps);

private:
    stmtPtr ps;
};

class DB::RowIterator
  : public boost::iterator_facade<RowIterator, Row, std::input_iterator_tag>
{
    friend class boost::iterator_core_access;

public:
    RowIterator() : ps(nullptr), row(ps)
    {
    }

    RowIterator(sqlite3_stmt *ps) : ps(ps), row(ps)
    {
        increment();
    }

private:
    void increment();

    bool equal(const RowIterator &rhs) const
    {
        return ps == rhs.ps;
    }

    Row & dereference() const
    {
        return row;
    }

private:
    sqlite3_stmt *ps;
    mutable Row row;
};

class DB::RowsData
{
protected:
    RowsData(stmtPtr ps) : ps(std::move(ps))
    {
    }

protected:
    stmtPtr ps;
};

class DB::Rows : private RowsData, public RowsBase
{
public:
    explicit Rows(stmtPtr ps)
        : RowsData(std::move(ps)),
          RowsBase(RowIterator(RowsData::ps.get()), RowIterator())
    {
    }
};

class Transaction
{
public:
    Transaction(sqlite3 *conn);
    Transaction(const Transaction &rhs) = delete;
    Transaction(Transaction &&rhs) = default;
    Transaction & operator=(const Transaction &rhs) = delete;
    Transaction & operator=(Transaction &&rhs) = default;
    ~Transaction();

public:
    void commit();

private:
    sqlite3 *const conn;
    bool committed;
};

class Binding
{
public:
    explicit Binding(std::string name) : name(std::move(name))
    {
    }

public:
    Binding & operator=(const std::string &val) &&
    {
        value = val;
        return *this;
    }

    Binding & operator=(int val) &&
    {
        value = val;
        return *this;
    }

    Binding & operator=(const std::vector<int> &val) &&
    {
        value = val;
        return *this;
    }

public:
    const std::string & getName() const
    {
        return name;
    }

    const boost::variant<std::string, int, std::vector<int>> & getValue() const
    {
        return value;
    }

private:
    const std::string name;
    boost::variant<std::string, int, std::vector<int>> value;
};

inline Binding operator "" _b(const char name[], std::size_t len)
{
    return Binding(std::string(name, len));
}

#endif // UNCOVER__DB_HPP__
