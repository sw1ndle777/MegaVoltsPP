import os

# Get the current directory
current_directory = os.getcwd()

# List to store .h files
h_files = []

# Iterate through all files in the current directory
for filename in os.listdir(current_directory):
    # Check if the file has a .h extension
    if filename.endswith('.h'):
        h_files.append(filename)

# Sort the list of .h files in alphabetical order
h_files.sort()

# Print each file in the desired format
for h_file in h_files:
    print(f'#include "handlers/{h_file}"')