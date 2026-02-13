#!/usr/bin/env python3
"""\
@file gen_emoji_characters.py

$LicenseInfo:firstyear=2025&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2025, Linden Research, Inc.

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
import json
import os
import glob

parser = argparse.ArgumentParser(description='Generate per-language emoji character list')
parser.add_argument('root_path', help='Path to the root directory containing subfolders with JSON files.')
parser.add_argument('output_dir', help='Path to the output directory where generated XML files will be stored.')
args = parser.parse_args()

ROOT_PATH = args.root_path
OUTPUT_DIR = args.output_dir

print(ROOT_PATH)
print(OUTPUT_DIR)

# Constants
# Note: There is no support for Turkish (TR) characters currently
#       si SL Viewer short-code support will be limited to English
ALLOWED_FOLDERS = {'da', 'de', 'en', 'es', 'fr', 'it', 'ja', 'pl', 'pt', 'ru', 'tr', 'zh'}

# Create output directory if it don't exist
output_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), OUTPUT_DIR)
os.makedirs(output_path, exist_ok=True)

def process_folder(data_file, cldr_file, emojibase_file, messages_file, output_file, folder_name):
    # Read and parse JSON files with specified encoding
    with open(data_file, 'r', encoding='utf-8') as f1:
        data = json.load(f1)

    with open(cldr_file, 'r', encoding='utf-8') as f2:
        cldr = json.load(f2)

    if os.path.isfile(emojibase_file):
        with open(emojibase_file, 'r', encoding='utf-8') as f3:
            emojibase = json.load(f3)
    else:
        emojibase = None

    with open(messages_file, 'r', encoding='utf-8') as f4:
        messages = json.load(f4)

    # Generate the desired output
    output = []

    groups = messages['groups']
    subgroups = messages['subgroups']

    for item in data:
        hexcode = item['hexcode']
        short_code = emojibase.get(hexcode) if emojibase else None
        if not short_code:
            short_code = cldr.get(hexcode)

        if '-' in hexcode:
            # Log unsupported multi-character emojis to the console
            #print(f"Unsupported multi-character emoji '{hexcode}' with shortcode ':{short_code}:' in folder {folder_name}")
            continue

        if not short_code:
            print(f"Error: Shortcode not found for hexcode '{hexcode}' in folder {folder_name}")
            continue

        # Convert hexcode to Unicode character
        character = chr(int(hexcode, 16))

        # Get categories from groups and subgroups if they exist
        group = next((g['message'] for g in groups if item.get('group') is not None and g['order'] == item['group']), '')
        subgroup = next((sg['message'] for sg in subgroups if item.get('subgroup') is not None and sg['order'] == item['subgroup']), '')

        # The ampersand character is illegal in an XML file outside
        # of the CDATA section. They appear frequently in the groups/subgroup
        # tags - "smileys & emotions" for example - so we must replace them
        group=group.replace(" & ", " &amp; ")
        subgroup=subgroup.replace(" & ", " &amp; ")

        xml_shortcodes = ("".join([f'\t\t\t\t<string>:{code}:</string>\n' for code in short_code]) if isinstance(short_code, list) else f'\t\t\t\t<string>:{short_code}:</string>\n')

        map_element = (
            '\t\t<map>\n'
            '\t\t\t<key>Character</key>\n'
            f'\t\t\t<string>{character}</string>\n'
            '\t\t\t<key>ShortCodes</key>\n'
            '\t\t\t<array>\n'
            f'{xml_shortcodes}'
            '\t\t\t</array>\n'
            '\t\t\t<key>Categories</key>\n'
            '\t\t\t<array>\n' +
            (f'\t\t\t\t<string>{group}</string>\n' if group else '') +
            (f'\t\t\t\t<string>{subgroup}</string>\n' if subgroup else '') +
            '\t\t\t</array>\n'
            '\t\t</map>'
        )
        output.append(map_element)

    # Header
    xml_header = '<?xml version="1.0" ?>\n<llsd xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="llsd.xsd">\n'

    # Warning against editing manually at the top of each file
    # Ideally, this would be right at the top but it seems like
    # comments can only appear after the initial <?xml> tag.
    xml_header += '\t<!--\n'
    xml_header += '\tNOTE: changes made to this file locally will be overwritten\n'
    xml_header += '\twhen CMake is invoked during autobuild.\n'
    xml_header += '\tTo modify these files, update the 3p package here:\n'
    xml_header += '\thttps://github.com/secondlife/3p-emoji-shortcodes\n'
    xml_header += '\tand add the resulting artifact to autobuild.xml\n'
    xml_header += '\t-->\n'
    xml_header += '\t<array>\n'

    # Footer
    xml_footer = '\t</array>\n</llsd>'

    output_str = xml_header + '\n'.join(output) + xml_footer

    # Write the output to a file with specified encoding
    with open(output_file, 'w', encoding='utf-8') as outfile:
        outfile.write(output_str)

# Add allowed folder names
allowed_folders = {'da', 'de', 'en', 'es', 'fr', 'it', 'ja', 'pl', 'pt', 'ru', 'tr', 'zh'}

# Process each subfolder
for subfolder in glob.glob(os.path.join(ROOT_PATH, '*')):
    if os.path.isdir(subfolder):
        folder_name = os.path.basename(subfolder)

        # Skip processing if folder_name not in allowed_folders
        if folder_name not in ALLOWED_FOLDERS:
            continue

        print(f"Processing folder: {folder_name}")

        data_file = os.path.join(subfolder, 'data.raw.json')
        cldr_file = os.path.join(subfolder, 'shortcodes', 'cldr.raw.json')
        emojibase_file = os.path.join(subfolder, 'shortcodes', 'emojibase.raw.json')
        messages_file = os.path.join(subfolder, 'messages.raw.json')

        # Create output subfolder if it doesn't exist
        output_subfolder = os.path.join(output_path, folder_name)
        os.makedirs(output_subfolder, exist_ok=True)

        output_file = os.path.join(output_subfolder, 'emoji_characters.xml')

        if os.path.isfile(data_file) and os.path.isfile(cldr_file) and os.path.isfile(messages_file):
            process_folder(data_file, cldr_file, emojibase_file, messages_file, output_file, folder_name)
