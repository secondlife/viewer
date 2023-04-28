import os

def change_line_endings(file_path):
    with open(file_path, 'rb') as f:
        content = f.read()

    if b'\r\n' in content:
        fixed_content = content.replace(b'\r\n', b'\n')
        with open(file_path, 'wb') as f:
            f.write(fixed_content)
        return True

    return False

def process_directory(path):
    for root, _, files in os.walk(path):
        for file in files:
            file_path = os.path.join(root, file)
            if change_line_endings(file_path):
                print(f"Fixed line endings in {file_path}")

if __name__ == "__main__":
    process_directory(".")
