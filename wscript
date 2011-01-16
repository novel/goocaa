# vim:ft=python
top = '.'
out = 'build'

def set_options(opt):
    opt.tool_options('compiler_cc')

    opt.add_option('--with-debug', action='store_true', default=False, help='enable additional debugging')

def configure(conf):
    import Options

    if Options.options.with_debug:
        conf.env.append_unique('CFLAGS', ['-Wall -g'])

    conf.check_tool('compiler_cc')

    conf.check_cfg(atleast_pkgconfig_version='0.0.0')
    conf.check_cfg(package='libxml-2.0', args='--cflags --libs', uselib_store='LIBXML', mandatory=True)
    conf.check_cfg(package='glib-2.0', args='--cflags --libs', uselib_store='GLIB', mandatory=True)
    conf.check_cfg(package='libcurl', args='--cflags --libs', uselib_store='CURL', mandatory=True)

def build(bld):
    t = bld(
            features = ['cc', 'cprogram'],
            source = ['main.c', 'google.c', 'cache.c'],
            target = 'goocaa',
            install_path = '${PREFIX}/bin',
            uselib=['NEON', 'LIBXML', 'GLIB', 'CURL'])
