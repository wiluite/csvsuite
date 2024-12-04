#!/usr/bin/env python
# -*- coding: utf-8 -*-
# -*- mode: python -*-
# vi: set ft=python :


"""
C++ Plus NLS Util

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

__all__ = ["readall", "writeall", "load_file", "list_messages", "get_message", "main"]
__author__ = "LYF511, ChenPi11, The C++ Plus Project"
__license__ = "GPL-3.0-or-later"
__version__ = "0.0.1"
__maintainer__ = "ChenPi11"
__doc__ = \
"""
C++ Plus NLS Util
Read, get and replace messages in a cppp language map file.
"""

import sys

def readall(file_path):
    """Read all of the str data in a file and return it.

    Args:
        file_path (str): The file's path.

    Returns:
        str: Data in the file.
    """

    with open(file_path, "r", encoding="UTF-8") as file:
        file.seek(0)
        return file.read()


def writeall(file_path, data):
    """Write all of the data into a file.

    Args:
        file_path (str): The file's path.
        data (str): Data.
    """

    with open(file_path, "w", encoding="UTF-8") as file:
        file.seek(0)
        file.write(data)
        file.flush()


def load_file(path, domain):
    """Load a language map file and return the language map.

    Args:
        path (str): Language map file.
        domain (str): The domain you want to save.

    Returns:
        dict[dict[str, str]]: Result language map.
    """

    line_count = 0
    loaded_count = 0
    status = 0  # 0: close, 1: key, 2: content
    key_buffer = ""
    content_buffer = ""

    # Result map.
    messages = {}

    with open(path, "r", encoding="UTF-8") as file:
        lines = file.readlines()
        for line in lines:
            line_count += 1
            line = line.replace("\n", "")

            if line or status:
                if (
                    line.startswith("#") and not status
                ):  # Skip notes when it is not in text area.
                    continue
                if line == "'''":
                    if status < 2:
                        status += 1
                    else:
                        status = 0
                        messages.setdefault(domain, {})[key_buffer] = content_buffer
                        key_buffer = ""
                        content_buffer = ""
                        loaded_count += 1
                elif status:  # Contents
                    if status == 1:  # Key
                        key_buffer += ("\n" + line) if key_buffer else line
                    else:  # Content
                        content_buffer += ("\n" + line) if content_buffer else line
                else:  # Syntax error?
                    sys.stderr.write(
                        "WARNING: " + path + ":" + str(line_count) +
                            " : Syntax error in this line.\n"
                        )

    if status:
        sys.stderr.write("WARNING: A token never closed.\n")

    sys.stderr.write(path + ": Loaded " + str(loaded_count) + " messages.\n")
    return messages

def list_messages(messages, domain):
    """List messages in a language map.

    Args:
        messages (dict): The language map.
        domain (str): Domain.

    Returns:
        list[str]: Result list.
    """

    if domain not in messages:
        return []

    return list(messages[domain].keys())


def get_message(messages, key, domain, default_value=""):
    """Get a message in the language map.

    Args:
        messages (dict): The language map.
        key (str): Messgae key.
        domain (str): Domain.
        default_value (str, optional): Default value if get failed. Defaults to "".

    Raises:
        ValueError: If domain or key invalid.

    Returns:
        str: Message data.
    """

    if domain not in messages:
        raise ValueError("Invalid domain '" + domain + "'")

    domain_unit = messages[domain]
    if key not in domain_unit:
        if not default_value:
            raise ValueError("Invalid key '" + key + "'")
        return default_value
    return domain_unit[key]


# Main domain, we only used the domain named 'main' for replace files.
MAIN_DOMAIN = "main"


def main(argv):
    """C++ Plus NLS Util main entry.

    Args:
        argv (list): Arguments.

    Returns:
        int: Return value.
    """

    # Usage: nls_util.py input-file output-file language-map-file
    if len(argv) < 4:
        sys.stderr.write("Usage: " + argv[0] + " input-file output-file language-map-file\n")
        return 1

    # Messages pool.
    messages = {}

    input_file_path = argv[1]
    output_file_path = argv[2]
    langmap_file_path = argv[3]

    # Prepare for replace.
    messages = load_file(langmap_file_path, MAIN_DOMAIN)
    msgs = list_messages(messages, MAIN_DOMAIN)
    data = readall(input_file_path)
    replaced_count = 0

    # Replace key to message.
    for msg in msgs:
        data = data.replace(msg, get_message(messages, msg, MAIN_DOMAIN, msg))
        replaced_count += 1

    # Write data.
    writeall(output_file_path, data)

    sys.stderr.write(
        output_file_path + ": Successfully replaced " + str(replaced_count) + " words.\n"
        )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
