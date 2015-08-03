import __main__
import datetime
import logging
import logging.handlers
import os
import socket
import sys

import gflags

FLAGS = gflags.FLAGS

_LOGLEVELS = (
    'none', 'debug', 'info', 'warn', 'warning', 'error', 'fatal', 'critical')

gflags.DEFINE_enum(
    'logtostderr', 'warning',
    _LOGLEVELS,
    'Log to stderr.')
gflags.DEFINE_enum(
    'logtosyslog', 'info',
    _LOGLEVELS,
    'Log to syslog.')

_DATE_FORMAT = '%Y-%m-%d %H:%M:%S'
_LOG_BASE_FORMAT = '%(levelname)s [%(filename)s:%(lineno)d] %(message)s'

# Set in run()
appname = None


class ExtendedFormatter(logging.Formatter):
  def format(self, record):
    record.appname = appname
    return super(ExtendedFormatter, self).format(record)


class NullHandler(logging.Handler):
  def handle(self, record):
    pass

  def emit(self, record):
    pass

  def createLock(self):
    self.lock = None


def setup():
  global appname
  appname = os.path.splitext(os.path.basename(__main__.__file__))[0]

  root = logging.getLogger()
  root.setLevel(logging.DEBUG)

  # Avoid basicConfig() is called.
  root.addHandler(NullHandler())

  if FLAGS.logtostderr != 'none':
    handler = logging.StreamHandler(sys.stderr)
    formatter = ExtendedFormatter(
        fmt='%(asctime)-15s ' + _LOG_BASE_FORMAT,
        datefmt=_DATE_FORMAT)
    handler.setFormatter(formatter)
    handler.setLevel(getattr(logging, FLAGS.logtostderr.upper()))
    root.addHandler(handler)

  if FLAGS.logtosyslog != 'none':
    handler = logging.handlers.SysLogHandler('/dev/log', facility='local0')
    formatter = ExtendedFormatter(
        fmt='%(appname)s[%(process)d]: ' + _LOG_BASE_FORMAT,
        datefmt=_DATE_FORMAT)
    handler.setFormatter(formatter)
    handler.setLevel(getattr(logging, FLAGS.logtosyslog.upper()))
    root.addHandler(handler)
