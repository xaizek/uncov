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

#ifndef UNCOVER__INTEGRATION_HPP__
#define UNCOVER__INTEGRATION_HPP__

#include <sys/types.h>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream_buffer.hpp>

#include <string>
#include <utility>

class RedirectToPager
{
    template <typename T>
    using stream_buffer = boost::iostreams::stream_buffer<T>;
    using file_descriptor_sink = boost::iostreams::file_descriptor_sink;

    /**
     * @brief Custom stream buffer that spawns pager for large outputs only.
     *
     * Collect up to <terminal height> lines.  If buffer is closed with this
     * limit not reached, it prints lines on std::cout.  If we hit the limit in
     * the process of output, it opens a pager and feeds it all collected output
     * and everything that comes next.
     */
    class ScreenPageBuffer
    {
    public:
        using char_type = char;
        using category = boost::iostreams::sink_tag;

    public:
        ScreenPageBuffer(unsigned int screenHeight,
                         stream_buffer<file_descriptor_sink> *out);
        ~ScreenPageBuffer();

    public:
        std::streamsize write(const char s[], std::streamsize n);

    private:
        bool put(char c);
        void openPager();

    private:
        bool redirectToPager = false;
        unsigned int nLines = 0U;
        unsigned int screenHeight;
        std::string buffer;

        /**
         * @brief Pointer to buffer stored in RedirectToPager.
         *
         * This is not by value, because ScreenPageBuffer needs to be copyable.
         */
        stream_buffer<file_descriptor_sink> *out;
        pid_t pid;
    };

public:
    RedirectToPager();
    ~RedirectToPager();

private:
    stream_buffer<file_descriptor_sink> out;
    stream_buffer<ScreenPageBuffer> screenPageBuffer;
    std::streambuf *rdbuf;
};

/**
 * @brief Retrieves terminal width and height in characters.
 *
 * @returns Pair of actual terminal width and height, or maximum possible values
 *          of the type.
 */
std::pair<unsigned int, unsigned int> getTerminalSize();

#endif // UNCOVER__INTEGRATION_HPP__
