#!/usr/bin/env python3
# -*- mode: python -*-
# -*- encoding: utf8 -*-

"""Utilities."""

import hashlib
import os
import re


def ChecksumFile(path):
  assert os.path.isfile(path), (
    'Expecting a file, got: %s' % path)
  f = open(path, 'rb')
  try:
    bytes = f.read()
    md5 = hashlib.md5(bytes)
    return md5.hexdigest()
  finally:
    f.close()


def MakeDirectories(dir_path):
  try:
    os.makedirs(dir_path)
  except OSError as err:
    if err.errno == 17:
      pass  # Dir already exists
    else:
      raise err

def MakeDirectoriesFor(file_path):
  MakeDirectories(os.path.dirname(file_path))


def CleanFilePath(file_path):
  return '/'.join(filter(lambda x: x != '.', file_path.split('/')))


def StripPrefix(string, prefix):
  assert string.startswith(prefix)
  return string[len(prefix):]


def StripSuffix(string, suffix):
  assert string.endswith(suffix)
  return string[:-len(suffix)]


def SwitchSuffix(string, from_suffix, to_suffix):
  return StripSuffix(string, from_suffix) + to_suffix


def FindFiles(file_name_regex, base_dir=None):
  """Recursively lists file whose name matches a given regex.

  Args:
    file_name_regex: Regex to be matched.
    base_dir: Absolute path of the directory to list recursively.
  """
  if not base_dir:
    base_dir = os.getcwd()
  assert os.path.exists(base_dir), base_dir
  file_names = os.listdir(base_dir)
  for file_name in file_names:
    file_path = os.path.join(base_dir, file_name)
    if os.path.isdir(file_path):
      subdir_path = file_path
      for file_path in FindFiles(file_name_regex, base_dir=subdir_path):
        yield file_path
    else:
      if re.match(file_name_regex, file_name):
        yield CleanFilePath(file_path)
