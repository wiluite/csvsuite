#!/usr/bin/env python
# -*- coding: utf-8 -*-
# -*- mode: python -*-
# vi: set ft=python :

"""
Get locale infomation

Copyright (C) 2023 The C++ Plus Project.
This file is part of the build-aux library.

The build-aux library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

The build-aux library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public
License along with the build-aux library; see the file COPYING.
If not, see <https://www.gnu.org/licenses/>.
"""

__all__ = ["windows_getlocale", "posix_getlocale", "main"]
__author__ = "ChenPi11, The C++ Plus Project"
__license__ = "GPL-3.0-or-later"
__version__ = "0.0.1"
__maintainer__ = "ChenPi11"
__doc__ = \
"""
Get locale infomation
"""

import platform
import os
import sys

def windows_getlocale():
    """Get locale info on Windows.

    Returns:
        str: Locale info.
    """
    LOCALE_NAME_MAX_LENGTH = 85
    import ctypes
    buffer = ctypes.create_unicode_buffer("", LOCALE_NAME_MAX_LENGTH)
    ctypes.windll.kernel32.GetUserDefaultLocaleName(buffer, LOCALE_NAME_MAX_LENGTH)
    return buffer.value.replace("-", "_")

def posix_getlocale():
    """Get locale info on POSIX.

    Returns:
        str: Locale info.
    """
    locale = os.getenv("LANG")
    if locale is None:
        locale = os.getenv("LANGUAGE")
    if locale is None:
        locale = os.getenv("LC_ALL")
    if locale is None:
        locale = os.getenv("LC_CTYPE")
    if locale is None:
        locale = os.getenv("LC_MESSAGES")
    if locale is None:
        locale = "en_US.UTF-8"
    return locale.split(".")[0]

def main():
    """Main entry.
    """
    if platform.uname().system == "Windows":
        try:
            print(windows_getlocale())
        except:
            print(posix_getlocale())
    else:
        print(posix_getlocale())
    return 0

if __name__ == "__main__":
    sys.exit(main())
