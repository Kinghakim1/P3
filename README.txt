Project III: My Shell Project

Authors:
NETID:dg1000
- David Guerrero-Mendoza 
NETID: Haa103
- Hakim Adam 


Overview

This project implements a simple shell called MyShell. The shell supports common features like built-in commands (cd, pwd, which, exit), wildcard expansion, redirection, and pipeline handling.

Testing Strategy

The testing strategy focuses on ensuring that all implemented features work as expected. This includes validating the shell’s ability to handle various scenarios in wildcard expansion, input/output redirection, and pipelines. A comprehensive test plan with multiple test cases was created to cover the following aspects:
	•	Wildcard Expansion: Ensuring that wildcards (*, etc.) work correctly by expanding file names and handling unmatched patterns.
	•	Redirection: Verifying that the shell handles input and output redirection, including appending, correctly.
	•	Pipelines: Testing single-command and multi-command pipelines to ensure correct data flow between processes.

Testing Approach

The tests are organized into three categories, corresponding to the major features:
	1.	Wildcard Expansion Tests
	2.	Redirection Handling Tests
	3.	Pipeline Handling Tests
	4.	Built-in Command Tests
	5.	myscript.sh Execution

Each category is designed to check the expected behavior of the shell for the specific feature, both for valid cases and edge cases.
Test Cases

1. Wildcard Expansion Test (wildcard_test.sh)

Scenario 1: Expanding a wildcard for files in the current directory

	•	Test: echo *.txt
	•	Expected Output: Lists all .txt files in the current directory.

Scenario 2: Expanding a wildcard for a specific pattern

	•	Test: echo file*.txt
	•	Expected Output: Lists .txt files starting with “file”.

Scenario 3: Wildcard with no matches

	•	Test: echo no_match*.txt
	•	Expected Output: Prints no_match*.txt since no files match.

Scenario 4: Expanding wildcard in a subdirectory

	•	Test: echo subdir/*.txt
	•	Expected Output: Expands to list .txt files inside the subdir.

Scenario 5: Wildcard expansion with hidden files

	•	Test: echo .*txt
	•	Expected Output: Lists hidden files that match the pattern.

Scenario 6: Wildcard expansion with multiple matching files

	•	Test: echo *.c
	•	Expected Output: Lists .c files in the current directory.

2. Redirection Test (redirection_test.sh)

Scenario 1: Output redirection to a file

	•	Test: echo "This is a test" > output.txt
	•	Expected Output: Output “This is a test” into output.txt, verify with cat output.txt.

Scenario 2: Input redirection from a file

	•	Test: cat < input.txt
	•	Expected Output: Display the contents of input.txt.

Scenario 3: Redirecting output and appending to a file

	•	Test: echo "New line" >> output.txt
	•	Expected Output: Append “New line” to output.txt, verify with cat output.txt.

Scenario 4: Combining both input and output redirection

	•	Test: cat < input.txt > output.txt
	•	Expected Output: Read from input.txt and write to output.txt.

Scenario 5: Invalid input redirection (file doesn’t exist)

	•	Test: cat < nonexistentfile.txt
	•	Expected Output: Print error message for missing input file.

Scenario 6: Invalid output redirection (write permission error)

	•	Test: echo "Trying to write" > /etc/hosts
	•	Expected Output: Print an error message due to lack of permissions.

3. Pipeline Test (pipeline_test.sh)

Scenario 1: Basic two-command pipeline

	•	Test: echo "This is a test" | grep "test"
	•	Expected Output: The grep command filters out the string “test”.

Scenario 2: Multi-command pipeline (echo | grep | wc)

	•	Test: echo "This is a test" | grep "test" | wc -l
	•	Expected Output: Outputs the count of lines matching “test”.

Scenario 3: Pipeline with invalid command (should produce error)

	•	Test: echo "This is a test" | nonexistent_command
	•	Expected Output: Error message stating the command is not found.

Scenario 4: Pipeline where the second command has no input

	•	Test: echo "This is a test" | grep "missing"
	•	Expected Output: Should output nothing since no matching strings exist.

Scenario 5: Combining echo with multiple pipelines

	•	Test: echo "Hello from the test file" | grep "Hello" | wc -c
	•	Expected Output: Counts the characters of “Hello” from the input.

Scenario 6: Combining pipeline with input/output redirection

	•	Test: echo "Hello from the test file" | grep "Hello" > output.txt
	•	Expected Output: Filters the input and redirects the output to output.txt.


Built-in Command Test (testcase1, testcase2, testcase3)


Scenario 1: cd with no arguments (should go to home directory)

	•	Test: cd
	•	Expected Output: Changes to the home directory.

Scenario 2: pwd (should print the current working directory)

	•	Test: pwd
	•	Expected Output: Prints the current working directory.

Scenario 3: which with a valid command (e.g., ls)

	•	Test: which ls
	•	Expected Output: Prints the path of the ls command (e.g., /usr/bin/ls).

Scenario 4: exit command with no arguments

	•	Test: exit
	•	Expected Output: Prints “Exiting my shell.” and exits the shell.

Scenario 5: exit command with arguments

	•	Test: exit Arg1 Arg2
	•	Expected Output: Prints Arg1 Arg2 followed by “Exiting my shell.” and exits the shell.    