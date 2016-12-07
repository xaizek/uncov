#include "DB.hpp"

#include <sqlite3.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

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

DB::RowWrapper
DB::queryOne(const std::string &stmt, const std::vector<Binding> &binds)
{
    stmtPtr ps = prepare(stmt, binds);
    return RowWrapper(std::move(ps));
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

    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> ps {
        rawPs, sqlite3_finalize
    };
    rawPs = nullptr;

    class Binder : public boost::static_visitor<>
    {
    public:
        Binder(sqlite3_stmt *ps, int idx) : ps(ps), idx(idx)
        {
        }

    public:
        void operator()(int i) const
        {
            error = sqlite3_bind_int(ps, idx, i);
        }

        void operator()(const std::string &str) const
        {
            // TODO: try compressing the string here.
            error = sqlite3_bind_text(ps, idx,
                                      str.c_str(), str.length(),
                                      SQLITE_STATIC);
        }

    public:
        mutable int error;

    private:
        sqlite3_stmt *const ps;
        const int idx;
    };

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
DB::Row::getCountCount() const
{
    return sqlite3_column_count(ps);
}

Transaction
DB::makeTransaction()
{
    return Transaction(conn);
}

std::string
DB::Row::makeTupleItem(std::size_t idx, std::string *)
{
    // TODO: try compressing the string here.
    if (sqlite3_column_type(ps, idx) != SQLITE_TEXT) {
        throw std::runtime_error("Expected text type of column.");
    }
    return reinterpret_cast<const char *>(sqlite3_column_text(ps, idx));
}

int
DB::Row::makeTupleItem(std::size_t idx, int *)
{
    if (sqlite3_column_type(ps, idx) != SQLITE_INTEGER) {
        throw std::runtime_error("Expected integer type of column.");
    }
    return sqlite3_column_int(ps, idx);
}

DB::RowWrapper::RowWrapper(stmtPtr ps) : Row(ps.get()), ps(std::move(ps))
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
