#!/usr/bin/env python3
import os
import sys
import argparse

def generate_licenses_data(build_dir):
    licenses_dir = os.path.join(build_dir, "licenses")

    if not os.path.exists(licenses_dir):
        print(f"Error: Licenses directory not found at {licenses_dir}")
        sys.exit(1)

    license_content = ""
    package_count = 0

    # Collect all license files
    for root, dirs, files in os.walk(licenses_dir):
        for file in files:
            license_file = os.path.join(root, file)
            license_dir = os.path.dirname(license_file)
            package_name = os.path.basename(os.path.dirname(license_dir))

            try:
                with open(license_file, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read().strip()

                    # Add package header and content
                    license_content += f"--------------{package_name}--------------\n"
                    license_content += content + "\n\n"

                    package_count += 1
                    print(f"Info: Added license for {package_name} from {file}")

            except Exception as e:
                print(f"Warning: Could not read {license_file}: {e}")

    # Write as plain text file
    output_file = "assets/licenses.txt"
    os.makedirs(os.path.dirname(output_file), exist_ok=True)

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(license_content)

    print(f"Generated {output_file} with {package_count} licenses")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate licenses text file from Conan build directory')
    parser.add_argument('build_dir', help='Path to the build directory (e.g., Build-debug/arm64)')

    args = parser.parse_args()
    generate_licenses_data(args.build_dir)
