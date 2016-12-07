#include "TablePrinter.hpp"

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include "decoration.hpp"

/**
 * @brief Helper class that represents single column of a table.
 */
class TablePrinter::Column
{
public:
    /**
     * @brief Constructs empty column.
     *
     * @param idx Index of the column.
     * @param heading Key of the column.
     */
    Column(int idx, std::string heading, bool alignLeft)
        : idx(idx), alignLeft(alignLeft), heading(std::move(heading)),
          width(this->heading.size())
    {
    }

public:
    /**
     * @brief Retrieves index of the column.
     *
     * @returns The index.
     */
    int getIdx() const
    {
        return idx;
    }

    /**
     * @brief Returns whether this column should be aligned to the left.
     *
     * @returns @true if so, @false otherwise.
     */
    bool leftAlign() const
    {
        return alignLeft;
    }

    /**
     * @brief Retrieves heading of the column.
     *
     * @returns The heading.
     */
    const std::string getHeading() const
    {
        return truncate(heading);
    }

    /**
     * @brief Adds the value to the column.
     *
     * @param val Value to add.
     */
    void append(std::string val)
    {
        width = std::max(width, static_cast<unsigned int>(val.size()));
        values.emplace_back(std::move(val));
    }

    /**
     * @brief Retrieves widths of the column.
     *
     * @returns The width.
     */
    unsigned int getWidth() const
    {
        return width;
    }

    /**
     * @brief Reduces width of the column by @p by positions.
     *
     * @param by Amount by which to adjust column width.
     */
    void reduceWidthBy(unsigned int by)
    {
        width -= std::min(width, by);
    }

    /**
     * @brief Retrieves printable value of the column by index.
     *
     * The value can be truncated to fit limited width, which is indicated by
     * trailing ellipsis.
     *
     * @param i Index of the value (no range checks are performed).
     *
     * @returns The value.
     */
    std::string operator[](unsigned int i) const
    {
        return truncate(values[i]);
    }

private:
    /**
     * @brief Truncates a string with ellipsis to fit into column width.
     *
     * @param s The string to truncate.
     *
     * @returns Truncated string, which is the same as @p s if it already fits.
     */
    std::string truncate(const std::string &s) const
    {
        if (s.length() <= width) {
            return s;
        }
        if (width <= 3U) {
            return std::string("...").substr(0U, width);
        }
        return s.substr(0U, width - 3U) + "...";
    }

private:
    /**
     * @brief Index of the column.
     */
    const int idx;
    /**
     * @brief Whether this column should be aligned to the left.
     */
    bool alignLeft;
    /**
     * @brief Title of the column for printing.
     */
    const std::string heading;
    /**
     * @brief Width of the column.
     */
    unsigned int width;
    /**
     * @brief Contents of the column.
     */
    std::vector<std::string> values;
};

static const std::string gap = "  ";

TablePrinter::TablePrinter(const std::vector<std::string> &headings,
                           unsigned int maxWidth) : maxWidth(maxWidth)
{
    for (unsigned int i = 0U; i < headings.size(); ++i) {
        std::string heading = headings[i];

        bool left = false;
        if (!heading.empty() && heading.front() == '-') {
            heading = heading.substr(1U);
            left = true;
        }

        boost::algorithm::to_upper(heading);
        boost::algorithm::trim_left_if(heading, boost::is_from_range('_', '_'));
        cols.emplace_back(i, std::move(heading), left);
    }
}

TablePrinter::~TablePrinter()
{
}

void
TablePrinter::append(const std::vector<std::string> &item)
{
    if (item.size() != cols.size()) {
        throw std::invalid_argument("Invalid item added to the table.");
    }
    items.emplace_back(item);
}

void
TablePrinter::print(std::ostream &os)
{
    fillColumns();

    if (!adjustColumnsWidths()) {
        // Available width is not enough to display table.
        return;
    }

    printTableHeader(os);
    printTableRows(os);
}

void
TablePrinter::fillColumns()
{
    for (const std::vector<std::string> &item : items) {
        for (Column &col : cols) {
            col.append(item[col.getIdx()]);
        }
    }
}

bool
TablePrinter::adjustColumnsWidths()
{
    // The code below assumes that there is at least one column.
    if (cols.empty()) {
        return false;
    }

    // Calculate real width of the table.
    unsigned int realWidth = 0U;
    for (Column &col : cols) {
        realWidth += col.getWidth();
    }
    realWidth += gap.length()*(cols.size() - 1U);

    // Make ordering of columns that goes from widest to narrowest.
    std::vector<std::reference_wrapper<Column>> sorted {
        cols.begin(), cols.end()
    };
    std::sort(sorted.begin(), sorted.end(),
              [](const Column &a, const Column &b) {
                  return a.getWidth() >= b.getWidth();
              });

    // Repeatedly reduce columns until we reach target width.
    // At each iteration: reduce width of (at most all, but not necessarily) the
    // widest columns by making them at most as wide as the narrower columns
    // that directly follow them.
    while (realWidth > maxWidth) {
        unsigned int toReduce = realWidth - maxWidth;

        // Make list of the widest columns as well as figure out by which amount
        // we can adjust the width (difference between column widths).
        std::vector<std::reference_wrapper<Column>> widest;
        unsigned int maxAdj = static_cast<Column&>(sorted.front()).getWidth();
        for (Column &col : sorted) {
            const unsigned int w = col.getWidth();
            if (w != maxAdj) {
                maxAdj -= w;
                break;
            }
            widest.push_back(col);
        }

        // Reversed order of visiting to ensure that ordering invariant is
        // intact: last visited element can be reduced by smaller amount, which
        // will leave it the biggest.  Actually it doesn't matter because we
        // reach target width at the same time, still it might matter later.
        for (Column &col : boost::adaptors::reverse(widest)) {
            const unsigned int by = std::min(maxAdj, toReduce);
            col.reduceWidthBy(by);
            toReduce -= by;
        }

        // We could exhaust possibilities to reduce column width and all that's
        // left is padding between columns.
        if (maxAdj == 0) {
            break;
        }

        // Update current width of the table.
        realWidth = maxWidth + toReduce;
    }

    return realWidth <= maxWidth;
}

void
TablePrinter::printTableHeader(std::ostream &os)
{
    for (Column &col : cols) {
        os << (decor::white_fg + decor::black_bg + decor::bold + decor::inv
           << (col.leftAlign() ? std::left : std::right)
           << std::setw(col.getWidth()) << col.getHeading());

        if (&col != &cols.back()) {
            os << gap;
        }
    }
    os << '\n';
}

void
TablePrinter::printTableRows(std::ostream &os)
{
    for (unsigned int i = 0, n = items.size(); i < n; ++i) {
        for (Column &col : cols) {
            os << (col.leftAlign() ? std::left : std::right);
            os << std::setw(col.getWidth()) << col[i];
            if (&col != &cols.back()) {
                os << gap;
            }
        }
        os << '\n';
    }
}
