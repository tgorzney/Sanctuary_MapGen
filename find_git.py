import os
import zlib
import sys

def read_blob(hash_str):
    path = os.path.join('.git', 'objects', hash_str[:2], hash_str[2:])
    if not os.path.exists(path): return None
    with open(path, 'rb') as f:
        data = zlib.decompress(f.read())
    return data

def find_file_in_tree(tree_hash, target_path):
    parts = target_path.split('/')
    current_tree = tree_hash
    for part in parts:
        data = read_blob(current_tree)
        if not data: return None
        
        idx = data.find(b'\0') + 1
        found = False
        while idx < len(data):
            space = data.find(b' ', idx)
            if space == -1: break
            null = data.find(b'\0', space)
            if null == -1: break
            
            name = data[space+1:null].decode('ascii')
            hash_val = data[null+1:null+21].hex()
            
            if name == part:
                current_tree = hash_val
                found = True
                break
            idx = null + 21
        if not found: return None
    return current_tree

blob_hash = find_file_in_tree('2600bf4f34e6f8b3cef76644e0e65d8c5f0c6b9d', 'core/gen/Gen_Mask_Slope.cpp')
if blob_hash:
    file_data = read_blob(blob_hash)
    idx = file_data.find(b'\0') + 1
    text = file_data[idx:].decode('utf-8')
    lines = text.split('\n')
    for i, line in enumerate(lines):
        if 'dx' in line and 'dy' in line:
            start = max(0, i-5)
            end = min(len(lines), i+15)
            print('\n'.join(lines[start:end]))
            break
