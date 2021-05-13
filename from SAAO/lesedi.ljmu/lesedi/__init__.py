"""This is a namespace package.

Modules for the core library can be imported from `lesedi.lib`. See:

    https://packaging.python.org/guides/packaging-namespace-packages/

"""

try:
    __import__('pkg_resources').declare_namespace(__name__)
except ImportError:
    __path__ = __import__('pkgutil').extend_path(__path__, __name__)
