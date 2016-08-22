#!/usr/bin/python3

import os
import sys
import copy
import argparse
import subprocess

sys.dont_write_bytecode = True

from scenarios.rb            import ReviewBoard
from scenarios.testgenerator import TestGenerator
from scenarios.testreader    import TestReader
from scenarios.patch         import create_patch
from scenarios.ts            import Packages

arguments = {
    '--postreview': ('-p', {
            'action': 'store_true',
            'help': 'Create or update a review request of short list of scenarios',
        }),
    '--implement': ('-i', {
            'action': 'store_true',
            'help': 'Create an implementation template of new test',
        }),
    '--import-ts': (None, {
            'action': 'store_true',
            'help': 'Import implemented tests list to test scenarios document',
        }),

    '--author': ('-a', {
            'dest': 'author', 'default': None,
            'help': 'Author of the new test',
        }),
    '--document-id': ('-d', {
            'dest': 'doc_id',
            'help': 'Document ID (from doclist) which contains '\
                    'the current scenario',
        }),
    '--force': ('-f', {
            'dest': 'force', 'default': False, 'action': 'store_true',
            'help': 'Overwrite an existed file with the generated test',
        }),
    '--output': ('-o', {
            'dest': 'output',
            'help': 'The output filename to store the diff',
        }),
    '--reference': ('-r', {
            'dest': 'reference',
            'help': 'Any part of a test name to find it',
        }),
    '--repository': ('-r', {
            'dest': 'repository_id',
            'help': 'Review board repository ID or name',
        }),
    '--existing': ('-e', {
            'dest': 'existing',
            'help': 'Existing request ID to update',
        }),
    '--scenario-in-test': (None, {
            'dest': 'scenario_in_test', 'default': 0, 'type': int,
            'help': 'Declare the splitting level of a scenario in a code '\
                    '(0 - create a scenario in the file header; default=0)',
        }),
    '--scenario': ('-s', {
            'dest': 'scenario',
            'help': 'The name of file which contains '\
                    'the list of all scenarios',
        }),
    '--sorting': (None, {
            'dest': 'sorting', 'default': False, 'action': 'store_true',
            'help': 'Sort lists of groups and tests',
        }),
    '--test-suite': ('-t', {
            'dest': 'test_suite',
            'help': 'Local path to a test suite',
        }),
}

def add_argument(parent, argument, **opts):
    assert argument in arguments, argument
    arg, kwargs = arguments[argument]
    kwargs = copy.deepcopy(kwargs)
    kwargs.update(opts)
    if arg:
        args = (arg, argument)
    else:
        args = tuple([argument])
    parent.add_argument(*args, **kwargs)

def postreview():
    parser = argparse.ArgumentParser(
        description='Post a changeset to a Review Board server')
    add_argument(parser, '--postreview', required=True)
    group = parser.add_mutually_exclusive_group(required=True)
    add_argument(group, '--document-id')
    add_argument(group, '--scenario')
    add_argument(parser, '--test-suite', required=True)
    add_argument(parser, '--author')
    add_argument(parser, '--scenario-in-test')
    group = parser.add_mutually_exclusive_group(required=True)
    add_argument(group, '--output')
    add_argument(group, '--repository')
    add_argument(group, '--existing')
    args = parser.parse_args()

    if args.scenario:
        data = open(args.scenario).read()
    else:
        p = subprocess.Popen([
            "doclist", "-G", args.doc_id],
            stdout=subprocess.PIPE)
        data = p.stdout.read().decode('utf-8')

    patch = create_patch(args.test_suite, data=data, author=args.author,
            scenario_in_test=args.scenario_in_test)
    if args.output:
        open(args.output, 'w').write(patch)
        print('Patch saved in the file: ' + args.output)
    elif args.repository_id:
        rb = ReviewBoard()
        content = rb.create_review(args.repository_id)
        rb.update_diff(content.review_request.id, data=patch)
        print('Review request draft saved: ' + \
                content.review_request.absolute_url)
        print('Please check and publish this draft.')
    else:
        rb = ReviewBoard()
        content = rb.update_diff(args.existing, data=patch)
        revision = content.diff.revision
        content = rb.review(args.existing)
        rb.change_description(args.existing,
            "Link to the test scenario: [/r/%s/diff/1-%s/](%sdiff/1-%s/)" % (
                args.existing, revision,
                content.review_request.absolute_url, revision
            ))
        print('Review request draft updated: ' + \
                content.review_request.absolute_url)
        print('Please check and publish this draft.')

def implement():
    parser = argparse.ArgumentParser(description='Description.')
    add_argument(parser, '--implement',  required=True)
    add_argument(parser, '--scenario',   required=True)
    add_argument(parser, '--test-suite', required=True)
    add_argument(parser, '--reference',  required=True)
    add_argument(parser, '--author')
    add_argument(parser, '--scenario-in-test')
    add_argument(parser, '--force')
    args = parser.parse_args()

    reader = TestReader()
    reader.load_file(args.scenario)
    tests = []
    for key, test in sorted(reader.tests.items()):
        if args.reference not in key:
            continue
        if not hasattr(test.test, 'objective'):
            continue
        tests.append(key)

    if len(tests) == 0:
        print('Unknown test reference, please try again')
        if len(reader.tests.keys()) == 0:
            print('No available tests')
        else:
            print('List of available tests:')
            for i, test in enumerate(sorted(reader.tests.keys())):
                print('%i) %s' % (i + 1, test))
        sys.exit(-1)
    elif len(tests) > 1:
        print('Found few tests. Which test do you want to implement?')
        for i, test in enumerate(tests):
            print('%i) %s' % (i + 1, test))
        sys.exit(-1)
    else:
        test = reader.tests[tests[0]]
        filename = '%s.c' % os.path.join(args.test_suite, *test.get_location())
        if not args.force and os.path.exists(filename):
            print('The test already exists: %s' % filename)
            sys.exit(-1)
        open(filename, 'w').write(TestGenerator.generate(
                test, args.author, args.scenario_in_test))

        test.cut()
        open(args.scenario, 'w').write(reader.dump_full())

def import_ts():
    parser = argparse.ArgumentParser(
        description='Import implemented tests list to test scenarios document.')
    add_argument(parser, '--import-ts',  required=True)
    add_argument(parser, '--scenario',   required=True)
    add_argument(parser, '--test-suite', required=True)
    add_argument(parser, '--sorting')
    args = parser.parse_args()

    r = TestReader()
    packages = Packages(args.test_suite, sorting=args.sorting)
    if os.path.exists(args.scenario):
        r.load_file(args.scenario)
        packages.merge(r.context)
    else:
        r.import_context(packages.context)
    open(args.scenario, 'w').write(r.dump_full())

parser = argparse.ArgumentParser(description='Description.')
group = parser.add_mutually_exclusive_group(required=True)
add_argument(group, '--postreview')
add_argument(group, '--implement')
add_argument(group, '--import-ts')
args = parser.parse_args(sys.argv[1:2])

if   args.postreview: postreview()
elif args.implement:  implement()
elif args.import_ts:  import_ts()
