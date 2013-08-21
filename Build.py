#!/usr/bin/env python3
# -*- mode: python -*-
# -*- encoding: utf8 -*-

"""Build engine."""

import hashlib
import logging
import multiprocessing
import optparse
import os
import pickle
import queue
import re
import shutil
import subprocess
import sys
import threading

# Add the root directory to the Python path if necessary:
__path = os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))
if __path not in sys.path:
  sys.path.append(__path)

from base import base
from base import cli
from base import command

import BuildGraph as build_graph
import Misc as misc

# ------------------------------------------------------------------------------

class Error(Exception):
  """Base class for errors in this module."""


class SourceNotFoundError(Error):
  """Source file not found."""


# ------------------------------------------------------------------------------

FLAGS = base.FLAGS

FLAGS.AddBoolean(
  'verbose',
  default=False,
  help='Print verbose logging messages during the run.'
)

FLAGS.AddBoolean(
  'verbose_command',
  default=False,
  help='Print each command-line being run.'
)

FLAGS.AddInteger(
  'ncpu',
  default=multiprocessing.cpu_count(),
  help='Number of CPU to distribute the work.'
)

FLAGS.AddBoolean(
  'use_distcc',
  default=False,
  help='Whether to use distcc.'
)

FLAGS.AddString(
  'base_dir',
  default=os.getcwd(),
  help='Base directory to build from.'
)

# ------------------------------------------------------------------------------


# Marker for a parameter's default value.
# To use when None is a valid parameter's value that does not mean "default".
Default = object()


# ------------------------------------------------------------------------------

def FindGCC():
  lpaths = [
    '/usr/bin/g++',
    '/usr/bin/g++-4.6.1',
    '/Library/gcc/4.6.2/bin/g++',
  ]
  for path in lpaths:
    if not os.path.exists(path):
      continue
    version_line = base.ShellCommandOutput('%s --version' % path).split('\n')[0]
    version = version_line.split()[-1]
    nums = version.split('.')
    major = int(nums[0])
    minor = int(nums[1])
    if major < 4:
      continue
    if minor < 6:
      continue
    return path
  raise KeyError('G++ binary')

# ------------------------------------------------------------------------------


def ParseIncludedHeaders(source_path):
  """Returns the list of included headers in the given source files."""
  source = open(source_path)
  try:
    content = source.read()
    return re.findall('#include ["](.*)["]', content)
  finally:
    source.close()


def GetIncludedHeadersClosure(source_apath, includes=None):
  """Returns the transitive list of included sources in the given source file."""
  includes = includes or set()
  if os.path.exists(source_path):
    includes.add(source_path)

    for include in sorted(frozenset(ParseIncludedHeaders(source_path))):
      if include in includes: continue
      GetIncludedHeadersClosure(include, includes)

  return includes

# ------------------------------------------------------------------------------

class Path(object):
  """Representation of a project path.

  A project path is a relative path: 'path/to/file.ext'.
  It is relative to some directory within the project root:
  A source, intermediate or target directory.

  It maps to an absolute path in the source directory or in the target
  directory.
  """

  def __init__(self, env, ppath):
    """Constructs a project path.

    Args:
      env: Project environment.
      ppath: Project path (must be relative).
    """
    self._env = env
    self._ppath = ppath
    assert not os.path.isabs(self._ppath), (
      'Invalid project path: %r' % self._ppath)
    src_path = os.path.join(self.env.source_dir, self._ppath)
    if os.path.exists(src_path):
      self._apath = src_path
    else:
      self._apath = os.path.join(self.env.output_dir, source)

  @property
  def env(self):
    """Returns the project environment."""
    return self._env

  @property
  def pdir(self):
    """Returns the project directory of this path."""
    return os.path.dirname(self._ppath)

  @property
  def name(self):
    """Returns the path file name."""
    return os.path.basename(self._ppath)

  @property
  def extension(self):
    """Returns the extension of the path name."""
    return self.name.split('.')[-1]

  @property
  def ppath(self):
    """Returns this path's relative to the project ."""
    return self._ppath

  @property
  def apath(self):
    """Returns the absolute path for this project path."""
    return self._apath

  def AssertExists(self):
    assert os.path.exists(self._apath), (
      'Cannot file project path %s (%s)' % (self._ppath, self._apath))

  def __str__(self):
    return 'Path(ppath=%s)' % self._ppath

  def __repr__(self):
    return 'Path(ppath=%r, apath=%r)' % (self._ppath, self._apath)

# ------------------------------------------------------------------------------

class Metadata(object):
  def __init__(self, file_path):
    self._file_path = file_path

    # Map: key = frozenset((source-path,checksum)...)
    #   → value = frozenset((target-path,checksum)...)
    self._metadata = {}

  def Clear(self):
    self._metadata = {}

  def ReadFromFile(self):
    if not os.path.exists(self._file_path):
      return
    f = open(self._file_path, 'rb')
    try:
      self._metadata = pickle.load(f)
    finally:
      f.close()

  def WriteToFile(self):
    f = open(self._file_path, 'wb')
    try:
      pickle.dump(self._metadata, f)
    finally:
      f.close()

  @staticmethod
  def _ChecksumAll(paths):
    for path in paths:
      if not os.path.exists(path):
        yield path, None
      else:
        yield (path, misc.ChecksumFile(path))

  def IsUpToDate(self, sources, targets):
    sources = frozenset(Metadata._ChecksumAll(sources))
    if sources not in self._metadata:
      return False
    targets = frozenset(Metadata._ChecksumAll(targets))
    return self._metadata[sources] == targets

  def Record(self, sources, targets):
    sources = frozenset(Metadata._ChecksumAll(sources))
    targets = frozenset(Metadata._ChecksumAll(targets))
    for path, checksum in targets:
      assert checksum
    self._metadata[sources] = targets


# ------------------------------------------------------------------------------


class ConfigVars(object):
  """Holds a set of configuration variables."""

  def __init__(self):
    self.__dict__['_vars'] = {}

  def __getitem__(self, key):
    return self._vars.get(key)

  def __setitem__(self, key, value):
    self._vars[key] = value

  def __getattr__(self, key):
    return self._vars.get(key)

  def __setattr__(self, key, value):
    logging.info('Setting config var: %r = %r', key, value)
    self._vars[key] = value

  def __str__(self):
    return ('ConfigVars(%s)'
            % ','.join(map(lambda kv: '%r: %r' % kv, self._vars.items())))


# ------------------------------------------------------------------------------


class Environment(object):
  """Build environment."""

  DEFAULT_OUTPUT_DIR = 'build'
  DEFAULT_SOURCE_DIR = 'src'
  METADATA_FILE = '.metadata'

  def __init__(
      self,
      base_dir,
      output_dir=None,
      source_dir=None,
      metadata_file=None
  ):
    """Initializes the build environment.

    Args:
      base_dir: Absolute base directory path.
      output_dir: Output directory, relative to base_dir.
      source_dir: Source directory, relative to base_dir.
      metadata_file: Metadata file name, relative to base_dir.
    """
    output_dir = output_dir or Environment.DEFAULT_OUTPUT_DIR
    source_dir = source_dir or Environment.DEFAULT_SOURCE_DIR
    metadata_file = metadata_file or Environment.METADATA_FILE
    self._base_dir = base_dir
    assert os.path.exists(self._base_dir), (
      'Base directory does not exist: %s' % base_dir)
    self._output_dir = os.path.join(self._base_dir, output_dir)
    misc.MakeDirectories(self._output_dir)
    self._source_dir = os.path.join(self._base_dir, source_dir)
    assert os.path.exists(self._source_dir), (
      'Source directory does not exist: %s' % self._source_dir)

    self._metadata_file = os.path.join(self._base_dir, metadata_file)
    self._metadata = Metadata(self._metadata_file)
    self._metadata.ReadFromFile()

    logging.info('Base directory: %s', self._base_dir)
    logging.info('Source directory: %s', self._source_dir)
    logging.info('Output directory: %s', self._output_dir)
    logging.info('Metadata file: %s', self._metadata_file)

    # ----------------------------------------------------------------------

    self._vars = ConfigVars()

    self.vars.CXX_COMPILER = FindGCC()
    #CXX_COMPILER = '/usr/bin/clang'
    logging.info('Using gcc: %s', self.vars.CXX_COMPILER)

    self.vars.CXX_COMPILER_COMMAND = [self.vars.CXX_COMPILER]
    if FLAGS.use_distcc:
      self.vars.CXX_COMPILER_COMMAND.insert(0, 'distcc')
    self.vars.CXX_FLAGS = [ '-Wall', '-g', '-fPIC', '-std=c++0x' ]

    self.vars.AR = '/usr/bin/ar'
    self.vars.RANLIB = '/usr/bin/ranlib'

    self.vars.LINKER = self.vars.CXX_COMPILER
    self.vars.LINK_OPTIONS = ['-g', '-std=c++0x', '-lstdc++', '-lboost_regex']

    self.vars.PROTO_COMPILER = '/usr/bin/protoc'
    self.vars.PROTO_COMPILER_FLAGS = []

    # --------------------------------------
    # External libraries

    self.vars.PKG_CONFIG = 'pkg-config'
    self.vars.GTEST_CONFIG = 'gtest-config'

    # Google flags
    GFLAGS_CXX_FLAGS = (
        self.ShellOutput('{PKG_CONFIG} --cflags libgflags')).split()
    GFLAGS_LINK_OPTIONS = (
        self.ShellOutput('{PKG_CONFIG} --libs libgflags')).split()
    self.vars.CXX_FLAGS.extend(GFLAGS_CXX_FLAGS)
    self.vars.LINK_OPTIONS.extend(GFLAGS_LINK_OPTIONS)
    # self.vars.CXX_FLAGS += ['-I/R/gflags/current/include/']
    # self.vars.LINK_OPTIONS += ['-L/R/gflags/current/lib/', '-lgflags']

    # Google logging
    GLOG_CXX_FLAGS = (
        self.ShellOutput('{PKG_CONFIG} --cflags libglog')).split()
    GLOG_LINK_OPTIONS = (
        self.ShellOutput('{PKG_CONFIG} --libs libglog')).split()
    self.vars.CXX_FLAGS.extend(GLOG_CXX_FLAGS)
    self.vars.LINK_OPTIONS.extend(GLOG_LINK_OPTIONS)
    # self.vars.CXX_FLAGS += ['-I/R/glog/current/include/']
    # self.vars.LINK_OPTIONS += ['-L/R/glog/current/lib/', '-lglog']

    # Protobuf
    self.vars.PROTOBUF_CXX_FLAGS = (
        self.ShellOutput('{PKG_CONFIG} --cflags protobuf').split())
    self.vars.PROTOBUF_LINK_OPTIONS = (
        self.ShellOutput('{PKG_CONFIG} --libs protobuf').split())
    self.vars.CXX_FLAGS.extend(self.vars.PROTOBUF_CXX_FLAGS)
    self.vars.LINK_OPTIONS.extend(self.vars.PROTOBUF_LINK_OPTIONS)

    # Google unit-tests
    self.vars.GTEST_CXX_FLAGS = (
        self.ShellOutput('{GTEST_CONFIG} --cppflags --cxxflags').split())
    self.vars.GTEST_LINK_OPTIONS = (
        self.ShellOutput('{GTEST_CONFIG} --ldflags --libs').split())
    self.vars.CXX_FLAGS.extend(self.vars.GTEST_CXX_FLAGS)
    #LINK_OPTIONS.extend(GTEST_LINK_OPTIONS)  # Added for test binaries only

    # GMP / MPFR
    # GMP_LINK_OPTIONS = ['-lgmp', '-lgmpxx', '-lmpfr']
    # LINK_OPTIONS.extend(GMP_LINK_OPTIONS)
    self.vars.CXX_FLAGS += ['-I/R/mpfr/current/include/']
    self.vars.LINK_OPTIONS += ['-L/R/mpfr/current/lib/', '-lmpfr']

    self.vars.CXX_FLAGS += ['-I/R/gmp/current/include/']
    self.vars.LINK_OPTIONS += ['-L/R/gmp/current/lib/', '-lgmp', '-lgmpxx']

  @property
  def vars(self):
    return self._vars

  def ShellOutput(self, command):
    return base.ShellCommandOutput(command.format(**self.vars._vars))

  @property
  def base_dir(self):
    return self._base_dir

  @property
  def output_dir(self):
    return self._output_dir

  @property
  def source_dir(self):
    return self._source_dir

  def Exec(self, command_args):
    """Executes a command-line.

    Args:
      command_args: Command-line as a list of string.
    """
    cmd = command.Command(*command_args)
    if cmd.exit_code != 0:
      pretty_command = ' \\\n  '.join(command_args)
      logging.error('Command failed:\n%s\n%s', cmd.output_text, cmd.error_text)
      raise Exception('Command-line failure:\n%s' % pretty_command)

  def Clean(self):
    shutil.rmtree(self._output_dir, ignore_errors=True)
    self._metadata.Clear()
    self._metadata.WriteToFile()

  # def FindPFiles(self, file_name_regex):
  #   for apath in misc.FindFiles(file_name_regex, base_dir=self._source_dir):
  #     yield misc.StripPrefix(apath, self._source_dir)[1:]  # Remove leading /
  #   for apath in misc.FindFiles(file_name_regex, base_dir=self._output_dir):
  #     yield misc.StripPrefix(apath, self._output_dir)[1:]  # Remove leading /

  def GetSourceAbsolutePath(self, source):
    """Converts a path into an absolute file path.

    Args:
      source: Relative path.
    Returns:
      Absolute path of the source file in the source or output directory.
    """
    src_path = os.path.join(self._source_dir, source)
    if os.path.exists(src_path):
      return src_path
    out_path = os.path.join(self._output_dir, source)
    if os.path.exists(out_path):
      return out_path
    raise SourceNotFoundError('Cannot find source %s' % source)

  def GetCCIncludesClosure(self, source_ppath, closure=None):
    """Assumes C++ includes are project paths."""
    closure = closure or set()
    try:
      source_apath = self.GetSourceAbsolutePath(source_ppath)
      closure.add(source_ppath)

      for sub_include in frozenset(ParseIncludedHeaders(source_apath)):
        if sub_include not in closure:
          self.GetCCIncludesClosure(sub_include, closure)
    except SourceNotFoundError:
      pass

    return closure

  def IsUpToDate(self, sources, targets):
    return self._metadata.IsUpToDate(
      [self.GetSourceAbsolutePath(source) for source in sources],
      [os.path.join(self._output_dir, target) for target in targets])

  def RecordMetadata(self, sources, targets):
    self._metadata.Record(
      [self.GetSourceAbsolutePath(source) for source in sources],
      [os.path.join(self._output_dir, target) for target in targets])

  def CompileProto(self, source):
    """Compiles a protocol buffer definition into C++/Python/Java code.

    Args:
      source: .proto file Path.
    """
    proto_source = Path(self, source)
    assert proto_source.extension == 'proto', (
      'Expecting a .proto file path, got: %s' % source)
    proto_source.AssertExists()

    misc.MakeDirectories(self._output_dir)
    command = (
      [ self.vars.PROTO_COMPILER,
        '--proto_path=%s' % self._source_dir,
        '--proto_path=%s' % self._output_dir,
        '--cpp_out=%s' % self._output_dir,
        # '--java_out=%s' % os.path.join(self._output_dir, 'java')
        # '--python_out%s' % os.path.join(self._output_dir, 'python')
        ]
      + self.vars.PROTO_COMPILER_FLAGS
      + [ proto_source.apath ])
    logging.info('Compiling proto: %s', source)
    self.Exec(command)

  def CompileCC(self, source_ppath):
    """Compiles a C++ source file into an object file.

    Args:
      source_ppath: .cc file project path.
    """
    source_apath = self.GetSourceAbsolutePath(source_ppath)
    object_ppath = misc.SwitchSuffix(source_ppath, '.cc', '.o')
    object_apath = os.path.join(self._output_dir, object_ppath)

    source_ppaths = set([source_ppath])
    source_ppaths.update(self.GetCCIncludesClosure(source_ppath))
    target_ppaths = [ object_ppath ]
    if self.IsUpToDate(source_ppaths, target_ppaths):
      logging.info('%s -> %s is up to date', source_ppath, object_ppath)
      return

    misc.MakeDirectoriesFor(object_apath)
    command = (
      self.vars.CXX_COMPILER_COMMAND
      + self.vars.CXX_FLAGS
      + [ '-I' + self._source_dir,
          '-I' + self._output_dir,
          '-c', source_apath,
          '-o', object_apath,
          ])
    logging.info('Compiling C++: %s', source_ppath)
    self.Exec(command)

    self.RecordMetadata(source_ppaths, target_ppaths)

  def BuildLibrary(
      self,
      library_ppath,
      object_ppaths
  ):
    """Builds a static library.

    Args:
      library_path: The library target project path.
      object_paths: The objects project paths.
    """
    library_apath = os.path.join(self._output_dir, library_ppath)
    misc.MakeDirectoriesFor(library_apath)
    object_apaths = [os.path.join(self._output_dir, ppath)
                     for ppath in object_ppaths]
    if self.IsUpToDate(object_ppaths, [library_ppath]):
      logging.info('%s is up to date', library_ppath)
      return
    for object_apath in object_apaths:
      assert os.path.exists(object_apath), (
        'Cannot find object: %s' % object_apath)
    command = ([self.vars.AR, 'rc', library_apath]
               + object_apaths)
    logging.info('Building library: %s', library_ppath)
    self.Exec(command)

    assert os.path.exists(library_apath)
    command = [self.vars.RANLIB, library_apath]
    self.Exec(command)

    self.RecordMetadata(object_ppaths, [library_ppath])

  def LinkProgram(
      self,
      program_ppath,
      object_ppaths,
      library_ppaths,
      link_flags=Default
  ):
    """Links a C++ binary."""
    if link_flags is Default: link_flags = self.vars.LINK_OPTIONS
    program_apath = os.path.join(self._output_dir, program_ppath)
    misc.MakeDirectoriesFor(program_apath)
    object_apaths = [self.GetSourceAbsolutePath(ppath)
                     for ppath in object_ppaths]
    library_apaths = [self.GetSourceAbsolutePath(ppath)
                      for ppath in library_ppaths]

    # Check if program is already up-to-date
    source_ppaths = list(object_ppaths) + library_ppaths
    target_ppaths = [ program_ppath ]
    if self.IsUpToDate(source_ppaths, target_ppaths):
      logging.info('Program %s is up to date', program_ppath)
      return

    for object_apath in object_apaths:
      assert os.path.exists(object_apath), (
        'Cannot find object %s' % object_apath)
    for library_apath in library_apaths:
      assert os.path.exists(library_apath), (
        'Cannot find library %s' % library_apath)
    command = (
      [ self.vars.LINKER ]
      + [ '-o', program_apath ]
      + link_flags
      + library_apaths
      + object_apaths
      + library_apaths
      + link_flags
    )
    logging.info('Linking binary: %s', program_ppath)
    self.Exec(command)

    self.RecordMetadata(source_ppaths, target_ppaths)

  def LinkTest(
      self,
      test_ppath,
      object_ppaths,
      library_ppaths
  ):
    """Links a C++ test."""
    # Add test libraries
    test_link_flags = list(self.vars.LINK_OPTIONS)
    test_link_flags.extend(self.vars.GTEST_LINK_OPTIONS)
    self.LinkProgram(
        program_ppath=test_ppath,
        object_ppaths=object_ppaths,
        library_ppaths=library_ppaths,
        link_flags=test_link_flags,
    )

# ------------------------------------------------------------------------------

class Worker(object):
  """Worker processing tasks from a queue in a separate thread.

  The thread exits when the queue is empty.
  """

  def __init__(self, queue, process):
    self._queue = queue
    self._process = process
    self._thread = threading.Thread(target=self._Run)
    self._thread.start()

  def Join(self):
    """Waits until the worker exits."""
    self._thread.join()

  def _Run(self):
    try:
      while True:
        job = self._queue.get_nowait()
        self._process(job)
    except queue.Empty:
      logging.info('Worker done')


def WorkerPool(queue, process, nworkers):
  """Processes a queue of tasks by a set of workers.

  Args:
    queue: A queue of tasks.
    process: Procedure invoked to process a task.
    nworkers: Number of concurrent workers.
  """
  workers = [Worker(queue, process) for _ in range(nworkers)]
  for worker in workers:
    worker.Join()

# ------------------------------------------------------------------------------


class Action(cli.Action):
  def __init__(self):
    super(Action, self).__init__()
    self._env = Environment(base_dir=FLAGS.base_dir)

  @property
  def env(self):
    return self._env


class Clean(Action):
  def Run(self, args):
    logging.info('Cleaning the build targets')
    self.env.Clean()


class Build(Action):
  def Run(self, args):
    try:
      target_graph = build_graph.TargetGraph()
      config_path = os.path.join(FLAGS.base_dir, 'BuildConfig.py')
      build_graph.ParseConfig(target_graph, config_path)
      target_graph.Dump()

      q = queue.Queue()
      for target_name in target_graph.TopologicalSort():
        target = target_graph._nodes[target_name]
        q.put_nowait(target)

      def BuildTarget(target):
        logging.info('Building %s', target.name)
        target.Build(self.env)

      WorkerPool(q, BuildTarget, FLAGS.ncpu + 1)

    finally:
      self.env._metadata.WriteToFile()

# ------------------------------------------------------------------------------


FLAGS.AddString(
  'do',
  default=None,
  help='Optional explicit action to run. One of clean, build.'
)

def Main(args):
  logging.info('Base directory: %s', FLAGS.base_dir)
  logging.info('Using %d CPUs', FLAGS.ncpu)

  do = FLAGS.do
  if (do is None) and (len(args) > 0):
    do = args[0]
    args = args[1:]

  action_map = dict(map(lambda k: (k.__name__.lower(), k),
                        Action.__subclasses__()))
  action_class = action_map.get(do)
  if action_class is None:
    logging.error(
        'Unknown action: %r. Available actions: %s.',
        do, ', '.join(sorted(action_map.keys())))
    return os.EX_USAGE
  action = action_class()
  return action(args)


if __name__ == '__main__':
  base.Run(Main)
