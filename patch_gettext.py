#!/usr/bin/env python
# encoding: utf-8
# Copyright (C) 2021 John Törnblom
"""
Replace literal strings in the DevolutionX source code with gettext macro calls.
"""

import ctypes.util
import fileinput
import glob

from clang import cindex
from clang.cindex import Index, Config, TokenKind


for libname in ['clang', 'clang-9', 'clang-10']:
    filename = ctypes.util.find_library(libname)
    if filename:
        Config.set_library_file(filename)
        break


def patch_literals(filename, positions):
    print('patching %d literals in %s' % (len(positions), filename))
    
    with open(filename, 'rb') as f:
        code = list(f.read())

    for pos, size in reversed(positions):
        code.insert(pos, ord('_'))
        code.insert(pos+1, ord('('))
        code.insert(pos+1+size+1, ord(')'))
        
    with open(filename, 'wb') as f:
        f.write(bytearray(code))


def main():
    with fileinput.input(files=glob.glob('*.ENG.txt')) as f:
        locale_literals = frozenset([line.strip() for line in f])

    cdb = cindex.CompilationDatabase.fromDirectory('build')
    for cmd in cdb.getAllCompileCommands():
        index = Index.create()
        tu = index.parse(None, list(cmd.arguments)).cursor

        positions = list()
        for tok in tu.get_tokens():
            # token is not from current file
            if tok.location.file.name != cmd.filename:
                continue

            # token is not a literal
            if tok.kind != TokenKind.LITERAL:
                continue

            # literal does not appear in PSX game
            if tok.spelling not in locale_literals:
                continue

            # skip a couple of common strings that does not need translation
            if tok.spelling in ['""', '"%i"', '"%d"', '" "']:
                continue

            pos = (tok.location.offset, len(tok.spelling))
            positions.append(pos)

        if positions:
            patch_literals(cmd.filename, positions)


if __name__ == '__main__':
    main()
