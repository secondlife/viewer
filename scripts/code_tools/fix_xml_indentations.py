import os
import sys
import glob
import io
import xml.etree.ElementTree as ET

def parse_xml_file(file_path):
    try:
        tree = ET.parse(file_path)
        return tree
    except ET.ParseError as e:
        print(f"Error parsing XML file {file_path}: {e}")
        return None

def indent(elem, level=0, indent_text=False, indent_tab=False):
    indent_string = "\t" if indent_tab else "  "
    i = "\n" + level * indent_string
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + indent_string
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
        for elem in elem:
            indent(elem, level + 1, indent_text, indent_tab)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i
        if indent_text and elem.text and not elem.text.isspace():
            elem.text = "\n" + (level + 1) * indent_string + elem.text.strip() + "\n" + level * indent_string

def save_xml(tree, file_path, indent_text=False, indent_tab=False, rm_space=False):
    if tree is not None:
        root = tree.getroot()
        indent(root, indent_text=indent_text, indent_tab=indent_tab)
        xml_string = ET.tostring(root, encoding='unicode')
        if rm_space:
            xml_string = xml_string.replace(' />', '/>')
        try:
            with io.open(file_path, 'wb') as file:
                file.write('<?xml version="1.0" encoding="utf-8" standalone="yes"?>\n'.encode('utf-8'))
                file.write(xml_string.encode('utf-8'))
        except IOError as e:
            print(f"Error saving file {file_path}: {e}")

def process_directory(directory_path, indent_text=False, indent_tab=False, rm_space=False):
    if not os.path.isdir(directory_path):
        print(f"Directory not found: {directory_path}")
        return

    xml_files = glob.glob(os.path.join(directory_path, "*.xml"))
    if not xml_files:
        print(f"No XML files found in directory: {directory_path}")
        return

    for file_path in xml_files:
        tree = parse_xml_file(file_path)
        if tree is not None:
            save_xml(tree, file_path, indent_text, indent_tab, rm_space)


if __name__ == "__main__":
    if len(sys.argv) < 2 or '--help' in sys.argv:
        print("XML Formatter Script")
        print("This script formats XML files in a given directory with options for indentation and space removal.")
        print("\nUsage:")
        print("  python script_name.py <path/to/directory> [options]")
        print("\nOptions:")
        print("  --indent-text    Indents text within XML tags.")
        print("  --indent-tab     Uses tabs instead of spaces for indentation.")
        print("  --rm-space       Removes spaces in self-closing tags.")
        print("\nCommon Usage:")
        print("  To format XML files with text indentation, tab indentation, and removal of spaces in self-closing tags:")
        print("  python script_name.py /path/to/xmls --indent-text --indent-tab --rm-space")
        sys.exit(1)

    directory_path = sys.argv[1]
    indent_text = '--indent-text' in sys.argv
    indent_tab = '--indent-tab' in sys.argv
    rm_space = '--rm-space' in sys.argv
    process_directory(directory_path, indent_text, indent_tab, rm_space)
