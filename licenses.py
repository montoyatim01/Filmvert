#!/usr/bin/env python3
import os
import sys
import argparse

def generate_licenses_header(build_dir):
    licenses_dir = os.path.join(build_dir, "licenses")

    if not os.path.exists(licenses_dir):
        print(f"Error: Licenses directory not found at {licenses_dir}")
        sys.exit(1)

    header_content = '''#pragma once
#include <string>
#include <unordered_map>
extern std::string licText;
namespace licenses {
    const std::unordered_map<std::string, std::string> all_licenses = {
'''

    # Use os.walk to get all files recursively, regardless of extension
    for root, dirs, files in os.walk(licenses_dir):
        for file in files:
            license_file = os.path.join(root, file)

            # Get the directory containing the license file, then go up one more level
            license_dir = os.path.dirname(license_file)
            package_name = os.path.basename(os.path.dirname(license_dir))

            try:
                with open(license_file, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read().replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
                    header_content += f'        {{"{package_name}", "{content}"}},\n'
            except Exception as e:
                print(f"Warning: Could not read {license_file}: {e}")

    header_content += '''    };
}
'''

    with open("src/licenses.h", "w") as f:
        f.write(header_content)

    print(f"Generated src/licenses.h from licenses in {licenses_dir}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate licenses header from Conan build directory')
    parser.add_argument('build_dir', help='Path to the build directory (e.g., Build-debug/arm64)')

    args = parser.parse_args()
    generate_licenses_header(args.build_dir)
