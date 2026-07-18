#!/usr/bin/env python3
"""
Decoder Script for Session Changes

This script reads the encoded file and extracts all files to their proper locations.
It also handles deleted files by removing them from the filesystem.

Usage:
    python decode_files.py [encoded_file]

Default encoded file: session_changes_encoded.txt
"""

import os
import sys
from pathlib import Path


def decode_files(encoded_file: str = "bundle.txt"):
    """
    Decode the bundled file and create all individual files.
    Also processes any deleted files manifest and removes those files.
    
    Args:
        encoded_file: Path to the encoded file with all content
    """
    if not os.path.exists(encoded_file):
        print(f"Error: Encoded file '{encoded_file}' not found!")
        sys.exit(1)
    
    print(f"Reading from: {encoded_file}")
    
    with open(encoded_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Check for deleted files manifest at the beginning
    deleted_files = []
    deleted_manifest_start = "=" * 10 + "DELETED_FILES_MANIFEST\n"
    deleted_manifest_end = "=" * 10 + "END_DELETED_FILES_MANIFEST\n"
    
    if content.startswith(deleted_manifest_start):
        # Extract deleted files manifest
        end_idx = content.find(deleted_manifest_end)
        if end_idx != -1:
            manifest_content = content[len(deleted_manifest_start):end_idx]
            deleted_files = [line.strip() for line in manifest_content.split('\n') if line.strip()]
            # Remove manifest from content
            content = content[end_idx + len(deleted_manifest_end):]
            print(f"\nFound {len(deleted_files)} file(s) marked for deletion")
    
    # Process deleted files first
    files_deleted = 0
    if deleted_files:
        print("\n--- Processing Deletions ---")
        for file_path in deleted_files:
            # Normalize path separators
            file_path = file_path.replace('\\', '/')
            
            if os.path.exists(file_path):
                try:
                    os.remove(file_path)
                    print(f"[DELETED] {file_path}")
                    files_deleted += 1
                    
                    # Clean up empty parent directories
                    parent_dir = os.path.dirname(file_path)
                    while parent_dir:
                        try:
                            # Only remove if directory is empty
                            if os.path.isdir(parent_dir) and not os.listdir(parent_dir):
                                os.rmdir(parent_dir)
                                print(f"[REMOVED EMPTY DIR] {parent_dir}")
                                parent_dir = os.path.dirname(parent_dir)
                            else:
                                break
                        except:
                            break
                            
                except Exception as e:
                    print(f"[ERROR] Failed to delete {file_path}: {e}")
            else:
                print(f"[SKIP] Already deleted: {file_path}")
    
    # Split by the separator pattern (10 equals + ./ + filename)
    # Handle both "\n==========./" and "==========./" at the start
    separator = "=" * 10 + "./"
    
    # Normalize content to ensure consistent splitting
    if content.startswith(separator):
        # Add a newline before the first separator for consistent splitting
        content = "\n" + content
    
    # Now split by newline + separator
    parts = content.split("\n" + separator)
    
    files_created = 0
    
    if len(parts) > 1:
        print("\n--- Processing File Updates ---")
    
    for i, part in enumerate(parts):
        if not part.strip():
            continue
        
        # The first part is before any file marker, skip it
        if i == 0:
            continue
        
        # The part should start with the file path (without the ./ since we split on it)
        lines = part.split('\n', 1)
        
        if len(lines) < 1:
            continue
        
        file_path = lines[0].strip()
        file_content = lines[1] if len(lines) > 1 else ""
        
        # File path is already clean (no leading ./)
        if not file_path:
            continue
        
        # Create directory structure
        file_dir = os.path.dirname(file_path)
        if file_dir:
            os.makedirs(file_dir, exist_ok=True)
        
        # Write the file
        try:
            with open(file_path, 'w', encoding='utf-8', newline='\n') as f:
                # Remove leading/trailing empty lines from content but preserve internal structure
                f.write(file_content.rstrip('\n') + '\n')
            
            print(f"[OK] Created: {file_path}")
            files_created += 1
        except Exception as e:
            print(f"[ERROR] Error creating {file_path}: {e}")
    
    # Summary
    print("\n" + "=" * 50)
    if files_deleted > 0:
        print(f"✓ Deleted {files_deleted} file(s)")
    if files_created > 0:
        print(f"✓ Created/Updated {files_created} file(s)")
    print("=" * 50)


def main():
    if len(sys.argv) > 1:
        encoded_file = sys.argv[1]
    else:
        encoded_file = "session_changes_encoded.txt"
    
    decode_files(encoded_file)


if __name__ == "__main__":
    main()
