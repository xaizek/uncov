// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#ifndef UNCOV__COLORS_HPP__
#define UNCOV__COLORS_HPP__

//! Possible highlighting groups.
enum class ColorGroup
{
    Pre,           //!< Pre-formatted piece.

    AddedMark,     //!< Mark before a added line.
    RemovedMark,   //!< Mark before a removed line.
    RetainedMark,  //!< Mark before an unchanged line.

    Missed,        //!< Hits count of a missed line.
    SilentMissed,  //!< Hits count of a missed line that doesn't standout.
    Covered,       //!< Hits count of a covered line.
    SilentCovered, //!< Hits count of a covered line that doesn't standout.
    Irrelevant,    //!< Hits count of an irrelevant line.

    NoteMsg,       //!< Note message in diff output.
    ErrorMsg,      //!< Error message in output.
};

#endif // UNCOV__COLORS_HPP__
