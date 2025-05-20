#!/bin/bash

# Create a temporary directory for packaging
TEMP_DIR="temp_package"
mkdir -p "$TEMP_DIR/Code"

# Clean the Code directory
cd Code
make clean
cd ..

# Copy source files to temporary directory
cp Code/*.c Code/*.h Code/*.l Code/*.y Code/Makefile "$TEMP_DIR/Code/"

# Make the project in the temporary directory
cd "$TEMP_DIR/Code"
make
cd ../..

# Copy the generated parser and other required files
cp "$TEMP_DIR/Code/parser" "$TEMP_DIR/"
cp README "$TEMP_DIR/"
cp report.pdf "$TEMP_DIR/"

# Create the zip file
cd "$TEMP_DIR"
zip -r "../贺然_221240014.zip" .
cd ..

# Clean up
rm -rf "$TEMP_DIR"

echo "Package created successfully: 贺然_221240014.zip" 