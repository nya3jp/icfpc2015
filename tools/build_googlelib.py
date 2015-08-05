
import argparse
import errno
import os
import subprocess


_GOOGLE_LIB_PATH = 'googlelib'
_DEPENDENT_PACKAGE_LIST = [
    'cmake',
    'g++',
    'automake',
    'libunwind8-dev',
]

_ROOT_PATH = os.path.abspath(os.path.normpath(
    os.path.join(os.path.dirname(__file__), '..')))


def _safe_makedirs(path):
    """Almost equivalent to mkdir -p."""
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise


def _install_deps():
    subprocess.check_call(
        ['sudo', 'apt-get', 'install'] + _DEPENDENT_PACKAGE_LIST)


def _build_gflags():
    gflags_path = os.path.join(_ROOT_PATH, _GOOGLE_LIB_PATH, 'gflags')
    _safe_makedirs(gflags_path)
    environ = os.environ.copy()
    environ['CXXFLAGS'] = '-fPIC'  # PIC build, as it is required by glog.
    subprocess.check_call(['cmake', '../../third_party/gflags'],
                          cwd=gflags_path,
                          env=environ)
    subprocess.check_call(['make'], cwd=gflags_path)


def _build_glog():
    glog_path = os.path.join(_ROOT_PATH, _GOOGLE_LIB_PATH, 'glog')
    _safe_makedirs(glog_path)
    subprocess.check_call(['../../third_party/glog/configure',
                           '--with-gflags=../gflags'],
                          cwd=glog_path)
    subprocess.check_call(['make'], cwd=glog_path)


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--skip-install-deps', action='store_true',
                        help='Do not run apt-get install for dependent '
                        'packages.')
    return parser.parse_args()


def main():
    parsed_args = _parse_args()
    if not parsed_args.skip_install_deps:
        _install_deps()
    _build_gflags()
    _build_glog()


if __name__ == '__main__':
    main()
