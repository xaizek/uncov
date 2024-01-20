// Copyright (C) 2016 xaizek <xaizek@posteo.net>
//
// This file is part of uncov.
//
// uncov is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// uncov is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with uncov.  If not, see <http://www.gnu.org/licenses/>.

#ifndef UNCOV_DB_HPP_
#define UNCOV_DB_HPP_

/**
 * @file DB.hpp
 *
 * @brief This unit provides basic facilities for interacting with a database.
 *
 * At the moment SQLite is the only supported database.
 */

#include <boost/range.hpp>
#include <boost/variant.hpp>

#include <cstdint>

#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "utils/integer_seq.hpp"

struct sqlite3;
struct sqlite3_stmt;

class Binding;
class Transaction;

/**
 * @brief Represents databaes connection.
 */
class DB
{
    class Row;
    class SingleRow;
    class RowIterator;
    class RowsData;
    class Rows;

    //! Type of smart handle to database statement, which is RAII-friendly.
    using stmtPtr = std::unique_ptr<sqlite3_stmt,
                                    std::function<void(sqlite3_stmt *)>>;
    //! Iterator facade base for the Rows class.
    using RowsBase = boost::iterator_range<RowIterator>;

public:
    /**
     * @brief Opens a database.
     *
     * @param path Path to the database.
     *
     * @throws std::runtime_error if database connection can't be opened.
     */
    explicit DB(const std::string &path);

    /**
     * @brief Closes database connection.
     */
    ~DB();

public:
    /**
     * @brief Performs a statement and discards result.
     *
     * @param stmt  Statement to be performed.
     * @param binds Bindings to be applied.
     *
     * @throws std::runtime_error on failure to make a statement or apply binds.
     */
    void execute(const std::string &stmt,
                 const std::vector<Binding> &binds = {});

    /**
     * @brief Queries single row as a result of performing a statement.
     *
     * @param stmt  Statement to be performed.
     * @param binds Bindings to be applied.
     *
     * @returns An object representing row, which is convertible to a tuple.
     *
     * @throws std::runtime_error on failure to make a statement or apply binds.
     */
    SingleRow queryOne(const std::string &stmt,
                       const std::vector<Binding> &binds = {});

    /**
     * @brief Queries multiple rows as a result of performing a statement.
     *
     * @param stmt  Statement to be performed.
     * @param binds Bindings to be applied.
     *
     * @returns Range-friendly object that generates tuples.
     *
     * @throws std::runtime_error on failure to make a statement or apply binds.
     */
    Rows queryAll(const std::string &stmt,
                  const std::vector<Binding> &binds = {});

    /**
     * @brief Retrieves id of the last inserted row.
     *
     * @returns The id.
     */
    std::int64_t getLastRowId();

    /**
     * @brief Starts a transaction.
     *
     * Usage:
     * @code
     * Transaction transaction = db.makeTransaction();
     * // db.execute(...);
     * // db.execute(...);
     * transaction.commit();
     * @endcode
     *
     * @returns RAII transaction object.
     */
    Transaction makeTransaction();

private:
    /**
     * @brief Builds a prepared statement.
     *
     * @param stmt  Statement to be performed.
     * @param binds Bindings to be applied.
     *
     * @returns Smart pointer to just created prepared statement.
     *
     * @throws std::runtime_error on failure to make a statement or apply binds.
     */
    stmtPtr prepare(const std::string &stmt, const std::vector<Binding> &binds);

private:
    sqlite3 *conn; //!< Connection to the database.
};

/**
 * @brief Wrapper for a single database row.
 */
class DB::Row
{
    /**
     * @brief Type marker for overload resolution.
     *
     * @tparam T Marked type.
     */
    template <typename T> struct Marker {};

public:
    /**
     * @brief Initializes row from database statement.
     *
     * @param ps @copybrief ps
     */
    Row(sqlite3_stmt *ps) : ps(ps)
    {
    }

public:
    /**
     * @brief Retrieves contents of the row as a tuple.
     *
     * @tparam Types Column types.
     *
     * @returns Row columns.
     *
     * @throws std::runtime_error if tuple size or types don't match.
     */
    template <typename... Types>
    operator std::tuple<Types...>()
    {
        const int nCols = getColumnCount();
        if (nCols != sizeof...(Types)) {
            throw std::runtime_error {
                "Expected " + std::to_string(nCols) + " columns, got " +
                std::to_string(sizeof...(Types))
            };
        }

        return makeTuple<Types...>(index_sequence_for<Types...>());
    }

private:
    /**
     * @brief Does the actual job of converting columns into a tuple.
     *
     * @tparam Types Column types.
     * @tparam Is    Index sequence that numbers columns.
     *
     * @param is "Container" for integer sequence.
     *
     * @returns Row columns.
     *
     * @throws std::runtime_error if tuple types don't match.
     */
    template <typename... Types, std::size_t... Is>
    std::tuple<Types...> makeTuple(integer_sequence<Is...> is)
    {
        static_cast<void>(is);
        return std::make_tuple(makeTupleItem(Is, Marker<Types>())...);
    }

    /**
     * @brief Retrieves number of columns in the row.
     *
     * @returns The number.
     */
    int getColumnCount() const;

    /**
     * @brief Reads contents of a column as a string.
     *
     * @param idx    Index of the column.
     * @param marker Overload resolution marker.
     *
     * @returns The string.
     */
    std::string makeTupleItem(std::size_t idx, Marker<std::string> marker);

    /**
     * @brief Reads contents of a column as an integer.
     *
     * @param idx    Index of the column.
     * @param marker Overload resolution marker.
     *
     * @returns The integer.
     */
    int makeTupleItem(std::size_t idx, Marker<int> marker);

    /**
     * @brief Reads contents of a column as a vector of integers.
     *
     * @param idx    Index of the column.
     * @param marker Overload resolution marker.
     *
     * @returns The vector of integers.
     */
    std::vector<int> makeTupleItem(std::size_t idx,
                                   Marker<std::vector<int>> marker);

private:
    sqlite3_stmt *ps; //!< Handle to the database statement.
};

/**
 * @brief A wrapper for reading single row from database statement.
 */
class DB::SingleRow : public Row
{
public:
    /**
     * @brief Takes ownership of the argument and reads single row.
     *
     * @param ps @copybrief ps
     */
    explicit SingleRow(stmtPtr ps);

private:
    stmtPtr ps; //!< Smart handle to database statement.
};

/**
 * @brief Table row iterator implementation.
 */
class DB::RowIterator
  : public boost::iterator_facade<RowIterator, Row, std::input_iterator_tag>
{
    friend class boost::iterator_core_access;

public:
    /**
     * @brief Initializes empty row iterator (end-iterator).
     */
    RowIterator() : ps(nullptr), row(ps)
    {
    }

    /**
     * @brief Initializes non-empty row iterator (begin-iterator).
     *
     * @param ps @copybrief ps
     */
    RowIterator(sqlite3_stmt *ps) : ps(ps), row(ps)
    {
        increment();
    }

private:
    /**
     * @brief Advances iterator position to the next row.
     */
    void increment();

    /**
     * @brief Compares the iterator against another one for equality.
     *
     * @param rhs Iterator to compare against.
     *
     * @returns @c true when equal, @c false otherwise.
     */
    bool equal(const RowIterator &rhs) const
    {
        return ps == rhs.ps;
    }

    /**
     * @brief Dereferences iterator.
     *
     * @returns Row object that allows accessing columns.
     */
    Row & dereference() const
    {
        return row;
    }

private:
    sqlite3_stmt *ps; //!< Handle to the database statement.
    mutable Row row;  //!< Row iterator object.
};

/**
 * @brief Data class for base-from-member idiom for Rows.
 */
class DB::RowsData
{
protected:
    /**
     * @brief Just moves argument into a member.
     *
     * @param ps @copybrief ps
     */
    RowsData(stmtPtr ps) : ps(std::move(ps))
    {
    }

protected:
    stmtPtr ps; //!< Smart handle to database statement.
};

/**
 * @brief Iterator range for rows.
 */
class DB::Rows : private RowsData, public RowsBase
{
public:
    /**
     * @brief Initializes the range.
     *
     * @param ps Smart handle to database statement.
     */
    explicit Rows(stmtPtr ps)
        : RowsData(std::move(ps)),
          RowsBase(RowIterator(RowsData::ps.get()), RowIterator())
    {
    }
};

/**
 * @brief RAII class for managing transactions.
 *
 * Transaction is started in the constructor and automatically rolled back in
 * the destructor unless commit() was called.
 */
class Transaction
{
public:
    /**
     * Starts the transaction.
     *
     * @param conn @copybrief conn
     */
    Transaction(sqlite3 *conn);
    //! Not copyable.
    Transaction(const Transaction &rhs) = delete;
    //! Moveable.
    Transaction(Transaction &&rhs) = default;
    //! Not copy-assignable.
    Transaction & operator=(const Transaction &rhs) = delete;
    //! Move-assignable.
    Transaction & operator=(Transaction &&rhs) = default;
    /**
     * Rolls back the transaction if commit() wasn't called.
     */
    ~Transaction();

public:
    /**
     * @brief Commits the transaction.
     *
     * @throws std::logic_error if transaction has already been committed.
     * @throws std::runtime_error if transaction doesn't commit.
     */
    void commit();

private:
    sqlite3 *conn;  //!< Connection on which transaction is performed.
    bool committed; //!< Whether transaction has been committed.
};

/**
 * @brief A name-value pair that represents a binding for prepared statements.
 */
class Binding
{
    friend class BlankBinding;

    /**
     * @brief Initializes the binding.
     *
     * Only BlankBinding can do this.
     *
     * @param name  @copybrief name
     * @param value @copybrief value
     */
    Binding(std::string name,
            boost::variant<std::string, int, std::vector<int>> value)
        : name(std::move(name)), value(value)
    {
    }

public:
    /**
     * @brief Retrieves name of the binding.
     *
     * @returns The name.
     */
    const std::string & getName() const
    {
        return name;
    }

    /**
     * @brief Retrieves value of the binding.
     *
     * @returns The value.
     */
    const boost::variant<std::string, int, std::vector<int>> & getValue() const
    {
        return value;
    }

private:
    //! Name of the argument that is being bound.
    const std::string name;
    //! Value that is bound.
    const boost::variant<std::string, int, std::vector<int>> value;
};

/**
 * @brief A temporary object which is a result of user-defined literal _b.
 */
class BlankBinding
{
    friend BlankBinding operator ""_b(const char name[], std::size_t len);

    /**
     * @brief Initializes blank binding with a name.
     *
     * @param name @copybrief name
     */
    explicit BlankBinding(std::string name) : name(std::move(name))
    {
    }

public:
    /**
     * @brief Completes binding with a string.
     *
     * @param val Value for the binding.
     *
     * @returns Fully initialized binding.
     */
    Binding operator=(std::string val) &&
    {
        return Binding(name, std::move(val));
    }

    /**
     * @brief Completes binding with an integer.
     *
     * @param val Value for the binding.
     *
     * @returns Fully initialized binding.
     */
    Binding operator=(int val) &&
    {
        return Binding(name, val);
    }

    /**
     * @brief Completes binding with vector of integers.
     *
     * @param val Value for the binding.
     *
     * @returns Fully initialized binding.
     */
    Binding operator=(std::vector<int> val) &&
    {
        return Binding(name, std::move(val));
    }

private:
    const std::string name; //!< Name of the argument that is being bound.
};

/**
 * @brief Partially creates a binding that holds its name only.
 *
 * @param name Name of the binding.
 * @param len  Length of the name of the binding.
 *
 * @returns Half-formed binding which should be completed with assignment.
 */
inline BlankBinding
operator ""_b(const char name[], std::size_t len)
{
    return BlankBinding(std::string(name, len));
}

#endif // UNCOV_DB_HPP_
