import os
import re
import sys
import platform
import subprocess
import unittest
import codecs

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = ['-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=' + extdir,
                      '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable,
                      '-DKAHYPAR_PYTHON_INTERFACE=ON']

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(
            env.get('CXXFLAGS', ''), self.distribution.get_version())

        if env.get('KAHYPAR_USE_MINIMAL_BOOST', 'OFF').upper() == 'ON':
            cmake_args += ['-DKAHYPAR_PYTHON_INTERFACE=ON', 
                           '-DKAHYPAR_USE_MINIMAL_BOOST=ON']
            env['CXXFLAGS'] += ' -fPIC'

        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(
            ['cmake', ext.sourcedir] + cmake_args,
            cwd=self.build_temp, env=env)
        subprocess.check_call(
            ['cmake', '--build', '.', '--target', 'kahypar_python'] + build_args,
            cwd=self.build_temp)

def test_suite():
    test_loader = unittest.TestLoader()
    test_suite = test_loader.discover('python/tests', pattern='test_*.py')
    return test_suite

with codecs.open('README.md', "r", encoding='utf-8') as fh:
    long_description = fh.read()

if __name__ == '__main__':
    setup(
        name='kahypar',
        version='1.3.3',
        description='Python Inferface for the Karlsruhe Hypergraph Partitioning Framework (KaHyPar)',
        long_description=long_description,
        long_description_content_type="text/markdown",
        url="https://www.kahypar.org",
        author='Sebastian Schlag',
        author_email='kahypar@sebastianschlag.de',
        ext_modules=[CMakeExtension('kahypar')],
        cmdclass=dict(build_ext=CMakeBuild),
        zip_safe=False,
        # use MANIFEST.in for extra source files
        include_package_data=True,
        # find test suite
        test_suite='setup.test_suite',
        classifiers=[
            "Development Status :: 5 - Production/Stable",
            "Topic :: Scientific/Engineering",
            "Topic :: Software Development :: Libraries :: Python Modules",
            "License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)"
        ],
    )
