#!/usr/bin/env python
"""\

This script replaces tab characters with spaces in source code files.

$LicenseInfo:firstyear=2024&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2024, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
"""

import argparse
import os

def convert_tabs_to_spaces(file_path, tab_stop):
    """Convert tabs in a file to spaces, considering tab stops."""
    with open(file_path, 'r') as file:
        lines = file.readlines()

    # Skip files with no tabs
    if not any('\t' in line for line in lines):
        return

    new_lines = []
    for line in lines:
        # Remove trailing spaces
        line = line.rstrip()
        new_line = ''
        column = 0  # Track the column index for calculating tab stops
        for char in line:
            if char == '\t':
                # Calculate spaces needed to reach the next tab stop
                spaces_needed = tab_stop - (column % tab_stop)
                new_line += ' ' * spaces_needed
                column += spaces_needed
            else:
                new_line += char
                column += 1

        new_lines.append(new_line + '\n')

    with open(file_path, 'w', newline='\n') as file:
        file.writelines(new_lines)

def process_directory(directory, extensions, tab_stop):
    """Recursively process files in directory, considering tab stops."""
    extensions = tuple(extensions)
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(extensions):
                file_path = os.path.join(root, file)
                print(f"Processing {file_path}")
                convert_tabs_to_spaces(file_path, tab_stop)

def main():
    parser = argparse.ArgumentParser(description='Convert tabs to spaces in files, considering tab stops.')
    parser.add_argument('-e', '--extensions', type=str, default='c,cpp,h,hpp,inl,py,glsl,cmake', help='Comma-separated list of file extensions to process (default: "c,cpp,h,hpp,inl,py,glsl,cmake")')
    parser.add_argument('-t', '--tabstop', type=int, default=4, help='Tab stop size (default: 4)')
    parser.add_argument('-d', '--directory', type=str, required=True, help='Directory to process')

    args = parser.parse_args()

    extensions = args.extensions.split(',')
    # Add a dot prefix to each extension if not present
    extensions = [ext if ext.startswith('.') else f".{ext}" for ext in extensions]

    process_directory(args.directory, extensions, args.tabstop)
    print("Processing completed.")

if __name__ == "__main__":
    main()
