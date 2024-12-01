# Test 1: Expanding a wildcard for files in the current directory
echo "Test 1: Expanding wildcard *"
echo *.txt

# Test 2: Expanding a wildcard for a specific pattern
echo "Test 2: Expanding wildcard with a specific pattern"
echo file*.txt

# Test 3: Wildcard with no matches (should print as is)
echo "Test 3: Wildcard with no matches"
echo no_match*.txt

# Test 4: Expanding wildcard in a subdirectory
echo "Test 4: Expanding wildcard in a subdirectory"
echo subdir/*.txt

# Test 5: Wildcard expansion with hidden files (files starting with a dot)
echo "Test 5: Wildcard expansion with hidden files"
echo .*txt

# Test 6: Wildcard expansion with a specific folder
echo "Test 6: Expanding wildcard in a specific folder"
echo folder/*.* 

# Test 7: Wildcard expansion with multiple matching files
echo "Test 7: Expanding multiple matching files"
echo *.c

# Test 8: Wildcard expansion with no file found (should return the pattern itself)
echo "Test 8: Wildcard expansion with no files"
echo nofiles*