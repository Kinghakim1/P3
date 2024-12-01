# Test 1: Basic two-command pipeline (echo | grep)
echo "Test 1: Basic two-command pipeline"
echo "This is a test" | grep "test"

# Test 2: Pipeline with multiple commands (echo | grep | wc)
echo "Test 2: Pipeline with multiple commands"
echo "This is a test" | grep "test" | wc -l

# Test 3: Pipeline with invalid command (should produce error)
echo "Test 3: Pipeline with invalid command"
echo "This is a test" | nonexistent_command

# Test 4: Pipeline where the second command has no input (grep without input)
echo "Test 4: Pipeline where the second command has no input"
echo "This is a test" | grep "missing"

# Test 5: Combining echo with multiple pipelines (echo | grep | wc)
echo "Test 5: Combining echo with multiple pipelines"
echo "Hello from the test file" | grep "Hello" | wc -c

# Test 6: Combining pipeline with input/output redirection
echo "Test 6: Combining pipeline with input/output redirection"
echo "Hello from the test file" | grep "Hello" > output.txt
cat output.txt