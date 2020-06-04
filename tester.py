#!/usr/bin/env python3

'''
TODO: Verify that return code is non-error, in all cases (their code shouldn't
 crash)
TODO: Decide what to do about dot
TODO: Make sure they know that when they print an error, needs to start with
'Error'
'''

import os
import re
import subprocess
import sys
import collections
import signal

TestResult = collections.namedtuple('TestResult', ['output', 'error'])


def get_student_output(executable_command, test_path) -> str:
    try:
        student_process = subprocess.run(
            executable_command,
            stdin=open(test_path, 'r'),
            stderr=subprocess.STDOUT,
            stdout=subprocess.PIPE,
            timeout=10)
        if (student_process.returncode == -signal.SIGSEGV):
            return "Segmentation fault"
        else:
            return student_process.stdout.decode('utf-8')
    except subprocess.TimeoutExpired:
        return "Timed out"


def get_correct_output(test_path) -> str:
    '''Gets correct output.'''
    correct_output = open(test_path, 'r')
    return correct_output.read()


def clean_output(output: str) -> str:
    '''Clean up output as much as possible to allow the student output and the
    correct output to be compared.'''
    result = output

    # If any line starts off with an error phrase, truncate the line just to
    # include the error. This is for test matching purposes.
    result = re.sub('^Syntax error.*$', 'Syntax error',
                    result,
                    flags=re.MULTILINE | re.IGNORECASE)
    result = re.sub('^Evaluation error.*$',
                    'Evaluation error',
                    result,
                    flags=re.MULTILINE | re.IGNORECASE)

    # Sequences of one or more whitespace
    result = re.sub('\\s+', ' ', result)

    # Parens followed by whitespace should just remove whitespace
    result = re.sub('\(\\s+', '(', result)

    # Whitespace followed by right paren should just remove whitespace
    result = re.sub('\\s+\)', ')', result)

    # Trailing zeros after a decimal at end, i.e. 1.0000 -> 1
    result = re.sub('\\.0+$', '', result)

    # Trailing decimal places after non-zero values,
    # followed by whitespace i.e. 1.32000 -> 1.32
    result = re.sub('(\\.)([1-9]+)(0*)\\s+', '\\1\\2 ', result)

    # Trailing decimal places after non-zero values, at end,
    # i.e. 1.32000 -> 1.32
    result = re.sub('(\\.)([1-9]+)(0*)$', '\\1\\2', result)

    result = (result
              .lstrip()   # Whitespace at beginning
              .rstrip())  # Whitespace at end

    return result


def run_tests_with_valgrind(executable_command, test_path) -> TestResult:
    '''Run again with valgrind (just student version) and look for errors)'''
    valgrind_command = ['valgrind',
                        '--leak-check=full',
                        '--show-leak-kinds=all',
                        '--error-exitcode=99']
    valgrind_command.append(executable_command)

    try:
        process = subprocess.run(
            valgrind_command,
            stdin=open(test_path, 'r'),
            stderr=subprocess.STDOUT,
            stdout=subprocess.PIPE,
            timeout=10)
        return TestResult(process.stdout.decode('utf-8'),
                          process.returncode in (99, -signal.SIGSEGV))
    except subprocess.TimeoutExpired:
        return TestResult("Timed out", True)


def main() -> None:

    error_encountered = False
    executable_command = "./interpreter"

    if len(sys.argv) == 1:
        test_dir = "tests"
    else:
        test_dir = sys.argv[1]

    test_names = [test_name.split('.')[0]
                  for test_name in sorted(os.listdir(test_dir))
                  if test_name.split('.')[1] == 'scm']

    for test_name in test_names:
        print('------Test', test_name, '------')

        test_input_path = os.path.join(test_dir, test_name + ".scm")
        test_output_path = os.path.join(test_dir, test_name + ".output")
        student_output = get_student_output(executable_command,
                                            test_input_path)
        student_output = clean_output(student_output)

        correct_output = get_correct_output(test_output_path)
        correct_output = clean_output(correct_output)

        if student_output != correct_output:
            error_encountered = True
            print("---OUTPUT INCORRECT---")
            print('Correct output:')
            print(correct_output)
            print('Student output:')
            print(student_output)
        else:
            print("---OUTPUT CORRECT---")

        valgrind_test_results = run_tests_with_valgrind(
            executable_command,
            test_input_path)

        if valgrind_test_results.error:
            error_encountered = True
            print('---VALGRIND ERROR---')
            print('Valgrind test results')
            print(valgrind_test_results.output)
        else:
            print('---VALGRIND NO ERROR---')

    if error_encountered:
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == '__main__':
    main()
