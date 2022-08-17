-- SPDX-License-Identifier: Apache-2.0
-- Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.

-- Specify input and output paths:
OUTPUT_FILE = "doc/generated/rst/index.rst"
INPUT_FILE = "doc/generated/xml/index.xml"

-- Specific title
INDEX_TITLE = "Test Environment documetation"

-- If your documentation uses \verbatim directives for code snippets
-- you can convert those to reStructuredText C++ code-blocks:
VERBATIM_TO_CODE_BLOCK = "c"

-- Asterisks, pipes and trailing underscores have special meaning in
-- reStructuredText. If they appear in Doxy-comments anywhere except
-- for code-blocks, they must be escaped:
ESCAPE_ASTERISKS = true
ESCAPE_PIPES = true
ESCAPE_TRAILING_UNDERSCORES = true
