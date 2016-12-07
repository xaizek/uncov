#ifndef UNCOVER__ITEMTABLE_HPP__
#define UNCOVER__ITEMTABLE_HPP__

#include <iosfwd>
#include <string>
#include <vector>

/**
 * @brief String table formatter and printer.
 *
 * Format and sorting are configurable via constructor's parameters.
 */
class TablePrinter
{
    class Column;

public:
    /**
     * @brief Constructs the table formatter.
     *
     * @param fmt Format specification: <field>,<field>...
     * @param maxWidth Maximum allowed table width.
     */
    TablePrinter(const std::vector<std::string> &headings,
                 unsigned int maxWidth);
    /**
     * @brief To emit destructing code in corresponding source file.
     */
    ~TablePrinter();

public:
    /**
     * @brief Adds item to the table.
     *
     * @param item Row to add.
     *
     * @throws std::invalid_argument if item length doesn't match columns.
     */
    void append(const std::vector<std::string> &item);
    /**
     * @brief Prints table on standard output.
     *
     * @param os Output stream.
     */
    void print(std::ostream &os);

private:
    /**
     * @brief Populates columns with items' data.
     */
    void fillColumns();
    /**
     * @brief Ensures that columns fit into required width limit.
     *
     * @returns @c true on successful shrinking or @c false on failure.
     */
    bool adjustColumnsWidths();
    /**
     * @brief Print table heading.
     *
     * @param os Output stream.
     */
    void printTableHeader(std::ostream &os);
    /**
     * @brief Prints table lines.
     *
     * @param os Output stream.
     */
    void printTableRows(std::ostream &os);

private:
    /**
     * @brief Maximum allowed table width.
     */
    const unsigned int maxWidth;
    /**
     * @brief List of columns of the table.
     */
    std::vector<Column> cols;
    /**
     * @brief List of items to display.
     */
    std::vector<std::vector<std::string>> items;
};

#endif // UNCOVER__ITEMTABLE_HPP__
