#!/usr/bin/env python3
# -*- mode: python -*-
# -*- encoding: utf8 -*-

"""Direct acyclic graph of build targets."""

from abc import ABCMeta, abstractmethod
import logging
import threading

import Misc as misc

# ------------------------------------------------------------------------------


class Error(Exception):
  """Base class for errors in this module."""


class AbstractMethod(Error):
  """Invoking an abstract method."""


class BuildFailure(Error):
  """Build operation failed."""


# ------------------------------------------------------------------------------


class TargetNode(object):
  """Abstract base class for target nodes."""
  __metaclass__ = ABCMeta

  BUILD_SUCCESS = 1
  BUILD_FAILURE = 2

  def __init__(self, graph, name, dependencies=tuple()):
    """Initializes the node.

    Args:
      graph: The target graph this node belongs to.
      name: This node name.
      dependencies: Names of this node direct dependencies.
    """
    self._graph = graph
    self._name = name

    # Names of this node direct dependencies
    self._direct_dep_names = frozenset(dependencies)

    # Names of all this node dependencies
    transitive_dep_names = set(self.direct_dep_names)
    for dep_name in self._direct_dep_names:
      transitive_dep_names.update(graph[dep_name].transitive_dep_names)
    self._transitive_dep_names = frozenset(transitive_dep_names)

    # Becomes set once the target Build operation completed
    self._completed = threading.Event()

    # One of None, BUILT, FAILURE
    self._status = None

  def Build(self, env):
    """Builds the target and notify dependees."""
    try:
      for dep_node in self.direct_dep_nodes:
        if dep_node.WaitUntilCompleted() != TargetNode.BUILD_SUCCESS:
          logging.info(
              'Dependency failure: %s -> %s.', self.name, dep_node.name)
          raise BuildFailure()
      self._status = self._Build(env)
    except Error as exn:
      logging.info('Error building target %r: %r', self.name, exn)
      self._status = TargetNode.BUILD_FAILURE
    finally:
      self._completed.set()
    return self._status

  @abstractmethod
  def _Build(self, env):
    """Builds the target. Must be overridden by subclasses.

    Args:
      env: Environment used to build.

    Returns:
      Either TargetNode.BUILD_SUCCESS or TargetNode.BUILD_FAILURE
    """
    raise AbstractMethod()

  @property
  def graph(self):
    """The target graph this node belongs to."""
    return self._graph

  @property
  def name(self):
    """Name of this target node."""
    return self._name

  @property
  def direct_dep_names(self):
    """Names of the target node dependencies."""
    return self._direct_dep_names

  @property
  def direct_dep_nodes(self):
    return (self.graph[dep_name] for dep_name in self.direct_dep_names)

  @property
  def transitive_dep_names(self):
    return self._transitive_dep_names

  @property
  def transitive_dep_nodes(self):
    return (self.graph[dep_name] for dep_name in self._transitive_dep_names)

  @property
  def status(self):
    """Build status: None, BUILD_SUCCESS or BUILD_FAILURE."""
    return self._status

  def WaitUntilCompleted(self):
    """Waits until the Build operation is completed.

    Returns:
      The Build operation status.
    """
    self._completed.wait()
    return self._status


def TopoSortNodes(nodes):
  """Sorts the given nodes in topological order.

  Args:
    nodes: Collection of TargetNode.
  Yield:
    The TargetNode, in topological ordered.
  """
  remaining = set(nodes)
  satisfied = set()

  while remaining:
    runnable = set(filter(
      lambda n: not frozenset(n.transitive_dep_nodes).intersection(remaining),
      remaining))
    assert runnable

    for n in runnable:
      yield n
    remaining.difference_update(runnable)
    satisfied.update(runnable)

# ------------------------------------------------------------------------------


class ObjectNode(TargetNode):
  """Target node for an C++ object."""

  def __init__(self, graph, name, source, dependencies=tuple()):
    super(ObjectNode, self).__init__(graph=graph,
                                     name=name,
                                     dependencies=dependencies)
    self._source = source
    assert source.endswith('.cc')

  def _Build(self, env):
    env.CompileCC(source_ppath=self._source)
    return TargetNode.BUILD_SUCCESS


class ProtoNode(TargetNode):
  """Target node for a Protocol buffer definition."""

  def __init__(self, graph, name, source, dependencies=tuple()):
    super(ProtoNode, self).__init__(graph=graph,
                                    name=name,
                                    dependencies=dependencies)
    self._source = source
    assert source.endswith('.proto')

  def _Build(self, env):
    env.CompileProto(self._source)
    return TargetNode.BUILD_SUCCESS


class LibraryNode(TargetNode):
  """Target node for a C++ library."""

  def __init__(self, graph, name, objects=tuple(), dependencies=tuple()):
    super(LibraryNode, self).__init__(graph=graph,
                                      name=name,
                                      dependencies=dependencies)
    self._objects = objects

  def _Build(self, env):
    ppath = self.path
    oppaths = [o.name
               for o in self.direct_dep_nodes
               if isinstance(o, ObjectNode)]
    env.BuildLibrary(library_ppath=ppath,
                     object_ppaths=oppaths)
    return TargetNode.BUILD_SUCCESS

  @property
  def path(self):
    return '%s.a' % self.name


class BinaryNode(TargetNode):
  """Target node for a C++ binary program."""
  def __init__(self, graph, name, objects=tuple(), dependencies=tuple()):
    super(BinaryNode, self).__init__(graph=graph,
                                     name=name,
                                     dependencies=dependencies)
    self._objects = objects

  def _Build(self, env):
    oppaths = [o.name
               for o in self.direct_dep_nodes
               if isinstance(o, ObjectNode)]

    libraries = filter(lambda l: isinstance(l, LibraryNode),
                       self.transitive_dep_nodes)
    libraries = list(TopoSortNodes(libraries))
    libraries.reverse()
    lppaths = [l.path for l in libraries]

    env.LinkProgram(program_ppath=self.name,
                    object_ppaths=oppaths,
                    library_ppaths=lppaths)
    return TargetNode.BUILD_SUCCESS


class TestNode(TargetNode):
  """Target node for a C++ test binary."""

  def _Build(self, env):
    oppaths = [o.name
               for o in self.direct_dep_nodes
               if isinstance(o, ObjectNode)]

    libraries = filter(lambda l: isinstance(l, LibraryNode),
                       self.transitive_dep_nodes)
    libraries = list(TopoSortNodes(libraries))
    libraries.reverse()
    lppaths = [l.path for l in libraries]

    env.LinkTest(test_ppath=self.name,
                 object_ppaths=oppaths,
                 library_ppaths=lppaths)
    return TargetNode.BUILD_SUCCESS


# ------------------------------------------------------------------------------


class TargetGraph(object):
  """A graph of build targets."""

  def __init__(self):
    # Map: node name → TargetNode
    self._nodes = {}

  def __getitem__(self, name):
    return self._nodes[name]

  def _DeclareNode(self, node):
    assert node.name not in self._nodes, (
      'Duplicate target name: "%s"' % node.name)
    self._nodes[node.name] = node

  def DeclareObject(self, name, source, dependencies=tuple()):
    """Declares an object node."""
    logging.debug('Object node: %s (%s)', name, source)
    object_node = ObjectNode(graph=self,
                             name=name,
                             source=source,
                             dependencies=dependencies)
    self._DeclareNode(object_node)
    return object_node

  def _DeclareObjects(self, sources, dependencies):
    for source in sources:
      object_name = misc.SwitchSuffix(source, 'cc', 'o')
      yield self.DeclareObject(name=object_name,
                               source=source,
                               dependencies=dependencies)

  def DeclareLibrary(self,
                     name,
                     sources=tuple(),
                     dependencies=tuple()):
    """Declares a library node."""
    dependencies = set(dependencies)
    dependencies.update(
      [o.name for o in self._DeclareObjects(sources=sources,
                                            dependencies=dependencies)])

    library_node = LibraryNode(graph=self,
                               name=name,
                               dependencies=dependencies)
    self._DeclareNode(library_node)
    return library_node

  def DeclareProtoCompile(self, source):
    name = misc.SwitchSuffix(source, 'proto', 'pb.cc')
    proto_node = ProtoNode(graph=self,
                           name=name,
                           source=source)
    self._DeclareNode(proto_node)
    return proto_node

  def DeclareProtoLibrary(self, name,
                          sources=tuple(),
                          dependencies=tuple()):
    """Declares a protocol-buffer definition node."""
    proto_compile_nodes = map(self.DeclareProtoCompile, sources)

    dependencies = set(dependencies)
    dependencies.update([n.name for n in proto_compile_nodes])

    cc_sources = [misc.SwitchSuffix(source, 'proto', 'pb.cc')
                  for source in sources]

    self.DeclareLibrary(name=name,
                        sources=cc_sources,
                        dependencies=dependencies)

  def DeclareBinary(self,
                    name,
                    sources=tuple(),
                    dependencies=tuple()):
    """Declares a binary node."""
    dependencies = set(dependencies)
    bin_deps = set(dependencies)
    bin_deps.update(
      [o.name for o in self._DeclareObjects(sources=sources,
                                            dependencies=dependencies)])

    binary_node = BinaryNode(graph=self,
                             name=name,
                             dependencies=bin_deps)
    self._DeclareNode(binary_node)
    return binary_node

  def DeclareTest(self,
                  name,
                  sources=tuple(),
                  dependencies=tuple()):
    """Declares a test binary."""
    dependencies = set(dependencies)
    test_deps = set(dependencies)
    test_deps.update(
      [o.name for o in self._DeclareObjects(sources=sources,
                                            dependencies=dependencies)])

    test_node = TestNode(graph=self,
                         name=name,
                         dependencies=test_deps)
    self._DeclareNode(test_node)
    return test_node

  def GetTransitiveTargets(self, target_names):
    """Lists all the transitive targets.

    Args:
      target_names: Names of the target to start from.
    """
    all_names = set(target_names)
    new_names = frozenset(all_names)
    while new_names:
      dep_names = set()
      for target_name in new_names:
        target = self._nodes[target_name]
        dep_names.update(target.direct_dep_names)
      new_names = dep_names.difference(all_names)
      all_names.update(new_names)
    return all_names

  def GetTargetsWithSatisfiedDeps(self, target_names=None,
                                  satisfied=frozenset()):
    """Lists the targets whose dependencies are satisfied.

    Args:
      target_names: Targets to find the runnable ones from.
          None means all.
      satisfied: Target names to consider as satisfied.
    Yields:
      Names of the runnable targets.
    """
    target_names = target_names or frozenset(self._nodes)
    satisfied = frozenset(satisfied)
    for target_name in target_names:
      if target_name in satisfied: continue
      target = self._nodes[target_name]
      if target.direct_dep_names.issubset(satisfied):
        yield target_name

  def TopologicalSort(self, target_names=None):
    """Lists the nodes in tological order.

    Args:
      targets: Names of the target to sort.
          None means all.
    Returns:
      Ordered target names.
    """
    target_names = target_names or frozenset(self._nodes)

    remaining_names = set(self.GetTransitiveTargets(target_names))
    satisfied_names = set()
    while remaining_names:
      runnable_names = frozenset(
        self.GetTargetsWithSatisfiedDeps(remaining_names,
                                         satisfied=satisfied_names))
      assert runnable_names
      for name in runnable_names:
        yield name
      remaining_names.difference_update(runnable_names)
      satisfied_names.update(runnable_names)

  def Dump(self):
    """Dumps the target graph."""
    for target_name in self.TopologicalSort():
      target = self._nodes[target_name]
      logging.debug('%s: [%s]', target_name, ','.join(target.direct_dep_names))


def ParseConfig(target_graph, config_file_path):
  """Parses a configuration file into nodes in the specified target graph."""
  exec_globals = {
    'Library': target_graph.DeclareLibrary,
    'ProtoLibrary': target_graph.DeclareProtoLibrary,
    'Binary': target_graph.DeclareBinary,
    'Test': target_graph.DeclareTest,
  }
  with open(config_file_path) as f:
    exec(f.read(), exec_globals)
