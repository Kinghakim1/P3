# Test 1: Output redirection to a file
echo "Test 1: Output redirection"
echo "This is a test" > output.txt
cat output.txt

# Test 2: Input redirection from a file
echo "Test 2: Input redirection"
echo "Hello from file" > input.txt
cat < input.txt

# Test 3: Redirecting output to a file and appending text
echo "Test 3: Output redirection with append"
echo "Appending this text" >> output.txt
cat output.txt

# Test 4: Redirecting both output and input
echo "Test 4: Redirecting both input and output"
echo "Hello world" > input.txt
cat < input.txt > output.txt
cat output.txt

# Test 5: Invalid input redirection (file doesn't exist)
echo "Test 5: Invalid input redirection"
cat < nonexistentfile.txt

# Test 6: Invalid output redirection (write permission error)
echo "Test 6: Invalid output redirection"
echo "Trying to write to a read-only file" > /etc/hosts