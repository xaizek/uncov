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

#ifndef UNCOVER__DECORATION_HPP__
#define UNCOVER__DECORATION_HPP__

#include <functional>
#include <memory>
#include <iosfwd>
#include <type_traits>
#include <vector>

namespace decor {

using decorFunc = std::ostream & (*)(std::ostream &os);

class Decoration
{
public:
    Decoration() = default;
    Decoration(const Decoration &rhs);
    explicit Decoration(decorFunc decorator);
    Decoration(const Decoration &lhs, const Decoration &rhs);

    Decoration & operator=(Decoration &&rhs) = default;

public:
    std::ostream & decorate(std::ostream &os) const;

private:
    decorFunc decorator = nullptr;
    std::unique_ptr<Decoration> lhs;
    std::unique_ptr<Decoration> rhs;
};

inline Decoration
operator+(const Decoration &lhs, const Decoration &rhs)
{
    return Decoration(lhs, rhs);
}

inline std::ostream &
operator<<(std::ostream &os, const Decoration &d)
{
    return d.decorate(os);
}

class ScopedDecoration
{
public:
    ScopedDecoration(const Decoration &decoration,
                     std::function<void(std::ostream&)> app)
        : decoration(decoration)
    {
        apps.push_back(app);
    }
    ScopedDecoration(const ScopedDecoration &scoped,
                     std::function<void(std::ostream&)> app)
        : decoration(scoped.decoration), apps(scoped.apps)
    {
        apps.push_back(app);
    }

public:
    std::ostream & decorate(std::ostream &os) const;

private:
    const Decoration &decoration;
    std::vector<std::function<void(std::ostream&)>> apps;
};

template <typename T>
typename std::enable_if<std::is_function<T>::value, ScopedDecoration>::type
operator<<(const Decoration &d, const T &v)
{
    const T *val = v;
    return ScopedDecoration(d, [val](std::ostream &os) { os << val; });
}

template <typename T>
typename std::enable_if<!std::is_function<T>::value, ScopedDecoration>::type
operator<<(const Decoration &d, const T &val)
{
    return ScopedDecoration(d, [val](std::ostream &os) { os << val; });
}

template <typename T>
ScopedDecoration
operator<<(const ScopedDecoration &sd, const T &val)
{
    return ScopedDecoration(sd, [val](std::ostream &os) { os << val; });
}

inline std::ostream &
operator<<(std::ostream &os, const ScopedDecoration &sd)
{
    return sd.decorate(os);
}

extern const Decoration none;
extern const Decoration bold;
extern const Decoration inv;
extern const Decoration def;

extern const Decoration black_fg;
extern const Decoration red_fg;
extern const Decoration green_fg;
extern const Decoration yellow_fg;
extern const Decoration blue_fg;
extern const Decoration magenta_fg;
extern const Decoration cyan_fg;
extern const Decoration white_fg;

extern const Decoration black_bg;
extern const Decoration red_bg;
extern const Decoration green_bg;
extern const Decoration yellow_bg;
extern const Decoration blue_bg;
extern const Decoration magenta_bg;
extern const Decoration cyan_bg;
extern const Decoration white_bg;

}

#endif // UNCOVER__DECORATION_HPP__
