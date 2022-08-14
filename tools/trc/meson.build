# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

# C flags of project trc
c_args = []

# Common include directories for trc
inc = include_directories(
    'lib',
    '.',
)

# Common dependencies for trc
dep_openssl = dependency('openssl')
dep_common = declare_dependency(
    dependencies: [
        dep_libxml2,
        dep_popt,
        dep_openssl,
        dep_threads,
        dep_lib_trc,
        dep_lib_tools,
        dep_lib_logic_expr,
        dep_lib_logger_file,
        dep_lib_logger_core,
    ],
    include_directories: [inc, te_include]
)

# Create config header
cdata = configuration_data()
cdata.set_quoted('PACKAGE_STRING', 'trc')

configure_file(
    output: 'trc_config.h',
    configuration: cdata,
)


# Set C flags and LD flags and define HAVE_LIBPERL if libperl is present
perl_c_args = []
perl_ld_args = []
perl = find_program('perl', required: false)

if perl.found()
    ret = run_command([perl, '-MExtUtils::Embed', '-e', 'ccopts'])
    if ret.returncode() != 0
        error('perl: Failed to get cflags')
    endif
    foreach arg: ret.stdout().strip().split(' ')
        if (arg.startswith('-D') or arg.startswith('-I'))
            perl_c_args += arg
        endif
    endforeach

    ret = run_command([perl, '-MExtUtils::Embed', '-e', 'ldopts'])
    if ret.returncode() != 0
        error('perl: Failed to get ldflags')
    endif

    foreach arg: ret.stdout().strip().split(' ')
        if (arg.startswith('-l') or arg.startswith('-L'))
            perl_ld_args += arg
        endif
    endforeach

    perl_code = '''
        #include <EXTERN.h>
        #include <perl.h>
        int main(int argc, char **argv, char **env) {
            perl_alloc();
            return 0;
        }
    '''

    does_link = cc.links(
        perl_code,
        args: perl_c_args + perl_ld_args,
        name: 'perl link check',
    )
    if (does_link)
        perl_c_args += '-DHAVE_LIBPERL'
    else
        warning('Perl library is found but can not be used')
        perl_c_args = []
        perl_ld_args = []
    endif
endif

dep_perl = declare_dependency(
    compile_args: perl_c_args,
    link_args: perl_ld_args,
)

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

if cc.has_type('xmlError', prefix: '#include <libxml/xmlerror.h>',
        dependencies: [dep_libxml2])
    c_args += '-DHAVE_XMLERROR'
endif

# Check provided options
with_options = [
    'log-urls',
    'popups',
    'spoilers',
    'hidden-stats',
]
foreach option: with_options
    if (get_option('trc-' + option) == true)
        c_args += ('-DWITH_' + option.to_upper().underscorify())
    endif
endforeach

# Process subdirectories
subdir('lib')
subdir('trc-diff')
subdir('trc-log')
subdir('trc-report')
subdir('trc-key')
subdir('trc-update')

# Install scripts
install_data(
    [
        'te_trc.sh',
        'te_trc_mv',
        'te_trc_night_update',
        'te_trc_search',
        'te_trc_update',
        'te_trc_update_wrapper',
    ],
    install_dir: get_option('bindir'),
)
