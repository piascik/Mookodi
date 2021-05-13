"""This is a namespace package.

Modules for the SDK can be imported from `lesedi.sdk`. See:

    https://packaging.python.org/guides/packaging-namespace-packages/

"""

try:
    __import__('pkg_resources').declare_namespace(__name__)
except ImportError:
    __path__ = __import__('pkgutil').extend_path(__path__, __name__)
