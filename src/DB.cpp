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

#include "DB.hpp"

#include <sqlite3.h>
#include <zlib.h>

#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

/**
 * @brief Binds single argument of a prepared statement.
 */
class Binder : public boost::static_visitor<>
{
public:
    /**
     * @brief Initializes the binder.
     *
     * @param ps  Handle to statement to initialize.
     * @param idx Index of the argument.
     */
    Binder(sqlite3_stmt *ps, int idx) : ps(ps), idx(idx)
    {
    }

public:
    /**
     * @brief Binds an integer argument.
     *
     * @param i The argument.
     */
    void operator()(int i)
    {
        errorValue = sqlite3_bind_int(ps, idx, i);
    }

    /**
     * @brief Binds a string argument.
     *
     * @param str The argument.
     */
    void operator()(const std::string &str)
    {
        errorValue = sqlite3_bind_text(ps, idx,
                                       str.c_str(), str.length(),
                                       SQLITE_STATIC);
    }

    /**
     * @brief Binds a vector of integers.
     *
     * @param vec The argument.
     */
    void operator()(const std::vector<int> &vec)
    {
        std::ostringstream oss;
        std::copy(vec.cbegin(), vec.cend(),
                  std::ostream_iterator<int>(oss, " "));
        const std::string str = oss.str();

        unsigned long compressedSize = compressBound(str.length());
        std::vector<unsigned char> blob(4U + compressedSize);

        // Serialize size in an endianness-independent way.
        blob[0] = str.length() >> 24;
        blob[1] = str.length() >> 16;
        blob[2] = str.length() >> 8;
        blob[3] = str.length();

        if (compress(&blob[4], &compressedSize,
                     reinterpret_cast<const unsigned char *>(str.data()),
                     str.size()) != Z_OK) {
            throw std::runtime_error("Failed to compress data");
        }
        blob.resize(4U + compressedSize);

        errorValue = sqlite3_bind_blob(ps, idx,
                                       blob.data(), blob.size(),
                                       SQLITE_TRANSIENT);
    }

public:
    const int &error = errorValue; //!< "Accessor" for error code.

private:
    sqlite3_stmt *const ps;     //!< Handle to statement to initialize.
    const int idx;              //!< Index of the argument.
    int errorValue = SQLITE_OK; //!< Error code.
};

}

DB::DB(const std::string &path)
{
    if (sqlite3_open(path.c_str(), &conn) != SQLITE_OK) {
        std::string error = std::string("Can't open database: ")
                          + sqlite3_errmsg(conn);
        sqlite3_close(conn);
        throw std::runtime_error(error);
    }
}

DB::~DB()
{
    sqlite3_close(conn);
}

void
DB::execute(const std::string &stmt, const std::vector<Binding> &binds)
{
    stmtPtr ps = prepare(stmt, binds);
    if (sqlite3_step(ps.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("Execute step failed: ") +
                                 sqlite3_errmsg(conn));
    }
}

DB::SingleRow
DB::queryOne(const std::string &stmt, const std::vector<Binding> &binds)
{
    stmtPtr ps = prepare(stmt, binds);
    return SingleRow(std::move(ps));
}

DB::Rows
DB::queryAll(const std::string &stmt, const std::vector<Binding> &binds)
{
    stmtPtr ps = prepare(stmt, binds);
    return Rows(std::move(ps));
}

DB::stmtPtr
DB::prepare(const std::string &stmt, const std::vector<Binding> &binds)
{
    sqlite3_stmt *rawPs;
    const int error = sqlite3_prepare_v2(conn, stmt.c_str(),
                                         stmt.length() + 1U, &rawPs,
                                         nullptr);

    if (error != SQLITE_OK) {
        throw std::runtime_error(std::string("Execute prepare failed: ") +
                                 sqlite3_errmsg(conn));
    }

    stmtPtr ps(rawPs, sqlite3_finalize);
    rawPs = nullptr;

    for (const Binding &bind : binds) {
        const int idx =
            sqlite3_bind_parameter_index(ps.get(), bind.getName().c_str());
        if (idx == 0) {
            throw std::runtime_error("No such binding: " + bind.getName());
        }

        Binder doBind(ps.get(), idx);
        boost::apply_visitor(doBind, bind.getValue());
        if (doBind.error != SQLITE_OK) {
            throw std::runtime_error("Failed to set binding of " +
                                     bind.getName() + " " +
                                     sqlite3_errmsg(conn));
        }
    }

    return ps;
}

std::int64_t
DB::getLastRowId()
{
    return { sqlite3_last_insert_rowid(conn) };
}

int
DB::Row::getColumnCount() const
{
    return sqlite3_column_count(ps);
}

Transaction
DB::makeTransaction()
{
    return Transaction(conn);
}

std::string
DB::Row::makeTupleItem(std::size_t idx, Marker<std::string>)
{
    if (sqlite3_column_type(ps, idx) != SQLITE_TEXT) {
        throw std::runtime_error("Expected text type of column.");
    }
    return reinterpret_cast<const char *>(sqlite3_column_text(ps, idx));
}

int
DB::Row::makeTupleItem(std::size_t idx, Marker<int>)
{
    if (sqlite3_column_type(ps, idx) != SQLITE_INTEGER) {
        throw std::runtime_error("Expected integer type of column.");
    }
    return sqlite3_column_int(ps, idx);
}

std::vector<int>
DB::Row::makeTupleItem(std::size_t idx, Marker<std::vector<int>>)
{
    if (sqlite3_column_type(ps, idx) != SQLITE_BLOB) {
        throw std::runtime_error("Expected blob type of column.");
    }

    auto b = static_cast<const unsigned char *>(sqlite3_column_blob(ps, idx));
    std::vector<unsigned char> blob(b, b + sqlite3_column_bytes(ps, idx));
    unsigned long strSize =
        (blob[0] << 24) + (blob[1] << 16) + (blob[2] << 8) + blob[3];

    std::string str(strSize, '\0');
    if (uncompress(reinterpret_cast<unsigned char *>(&str[0]), &strSize,
                   &blob[4], blob.size() - 4U) != Z_OK) {
        throw std::runtime_error("Failed to uncompress data");
    }

    std::istringstream is(str);
    std::vector<int> vec;
    for (int i; is >> i; ) {
        vec.push_back(i);
    }
    return vec;
}

DB::SingleRow::SingleRow(stmtPtr ps) : Row(ps.get()), ps(std::move(ps))
{
    if (sqlite3_step(this->ps.get()) != SQLITE_ROW) {
        throw std::runtime_error("Failed to read single row");
    }
}

void
DB::RowIterator::increment()
{
    if (sqlite3_step(ps) != SQLITE_ROW) {
        *this = RowIterator();
    }
}

Transaction::Transaction(sqlite3 *conn) : conn(conn), committed(false)
{
    char *errMsg;
    if (sqlite3_exec(conn, "BEGIN TRANSACTION", nullptr, nullptr,
                     &errMsg) != 0) {
        std::string error = errMsg;
        sqlite3_free(errMsg);
        throw std::runtime_error("Failed to start transaction: " + error);
    }
}

void
Transaction::commit()
{
    if (committed) {
        throw std::logic_error("An attempt to commit a transaction twice");
    }

    char *errMsg;
    if (sqlite3_exec(conn, "END TRANSACTION", nullptr, nullptr, &errMsg) != 0) {
        std::string error = errMsg;
        sqlite3_free(errMsg);
        throw std::runtime_error("Failed to commit transaction: " + error);
    }
    committed = true;
}

Transaction::~Transaction()
{
    if (!committed) {
        (void)sqlite3_exec(conn, "ROLLBACK", nullptr, nullptr, nullptr);
    }
}
