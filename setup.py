from distutils.core import setup, Extension


mumbi = Extension(
    'mumbi',
    libraries = ['mraa'],
    sources = ['src/mumbimodule.c',
               'src/mumbi.c'],
)

setup(
    name='mumbi',
    version='1.0',

    url='https://github.com/buckket/mumbi',

    author='buckket',
    author_email='buckket@cock.li',

    description='Control Mumbi m-FS300 remote power sockets',

    ext_modules = [mumbi],
)
