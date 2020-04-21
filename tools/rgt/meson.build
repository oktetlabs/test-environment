# Copyright (C) 2018 OKTET Labs. All rights reserved.

# Common variable for rgt
package = 'rgt'
version = '0.4'
te_copyright = '# Copyright (C) 2003-2018 OKTET Labs. All rights reserved.'

# Common includes

inc = include_directories(
    'lib/tmpls',
    '.',
)
inc = [inc, te_include]

# Configure config.h and add define
c_args = ['-DHAVE_CONFIG_H']

cdata = configuration_data()
cdata.set_quoted('PACKAGE', package)
cdata.set_quoted('VERSION', version)
cdata.set('WITH_LIBXML', 1)

configure_file(output: 'config.h', configuration: cdata)

# Add dependencies

dep_glib = dependency('glib-2.0', version: '>= 2.4.8')

# Check for obstack feature

obstack_code = '''
    #include <stdlib.h>
    #include <obstack.h>
    int main ()
    {
        struct obstack mem;
        #define obstack_chunk_alloc malloc
        #define obstack_chunk_free free
        obstack_init (&mem);
        obstack_free (&mem, 0);
        return 0;
    }
'''

does_link = cc.links(
    obstack_code,
    name: 'obstack link check',
)

if not does_link
    error('Obstack feature is required to build RGT')
endif

# Add large logs support if required

if get_option('large-logs')
    c_args += ['-D_LARGEFILE64_SOURCE', '-D_FILE_OFFSET_BITS=64']
endif

# RGT uses many GNU extensions:
#  - obstack_printf
#  - program_invocation_short_name
# Force it here to be able to build the rest w/o _GNU_SOURCE.
c_args += '-D_GNU_SOURCE'

# Check libxml members

_xmlSAXHandler_members = [
    '_private',
    'endElementNs',
    'serror',
    'startElementNs',
]

foreach member: _xmlSAXHandler_members
    if not cc.has_member('struct _xmlSAXHandler', member,
            prefix: '#include <libxml/parser.h>', dependencies: [dep_libxml2])
        error('Member ' + member + ' of struct _xmlSAXHandler not found')
    endif
endforeach

# Find program that generates headers with templates

header_gen_script = find_program('gen_tmpls_h.sh', required: true)

# Process subdirectories

subdir('lib')
subdir('rgt-core')
subdir('rgt-dump')
subdir('rgt-idx')
subdir('rgt-log-bundle')
subdir('rgt-pdml2xml')
subdir('rgt-xml-merge')
subdir('rgt-format')
