import importlib
import importlib.util
import os
import sys
import HostLib  # host supplies this module


# we try to set up std out catching as soon as possible
# we call this from HostLib, so we should be able to import
# this safely
class StdoutCatcher:
    def __init__(self):
        pass
    def write(self, stuff):
        out_str = str(stuff) if stuff is not None else str()
        HostLib.Core_out(out_str)
    def flush(self):
        pass
sys.stdout = StdoutCatcher()
sys.stderr = sys.stdout


def bootstrap_dispatch(object, method_name, args):
    return getattr(object, method_name)(*args)


class HostLibProxy:

    def __init__(self, nion_lib):
        self.__nion_lib = nion_lib

    def __getattr__(self, name):
        def _missing(*args, **kwargs):
            return getattr(self.__nion_lib, name)(*args, **kwargs)
        return _missing

    def encode_variant(self, value):
        return value

    def convert_drawing_commands(self, commands):
        return commands

    def encode_text(self, text):
        return str(text) if text else str()

    def encode_data(self, data):
        return data

    def decode_data(self, data):
        return data

    def decode_font_metrics(self, font_metrics):
        from nion.ui import UserInterface
        return UserInterface.FontMetrics(width=font_metrics[0], height=font_metrics[1], ascent=font_metrics[2], descent=font_metrics[3], leading=font_metrics[4])


def load_module_as_path(path):
    if os.path.isfile(path):
        dirname = os.path.dirname(path)
        module_name = os.path.splitext(os.path.basename(path))[0]
        sys.path.insert(0, dirname)
        module = importlib.import_module(module_name)
        return getattr(module, "main", None)
    return None


def load_module_as_package(package):
    try:
        module = importlib.import_module(package)
        main_fn = getattr(module, "main", None)
        if main_fn:
            return main_fn
    except ImportError:
        pass
    try:
        module = importlib.import_module(package + ".main")
        main_fn = getattr(module, "main", None)
        if main_fn:
            return main_fn
    except ImportError:
        pass
    return None


def load_module_local(path=None):
    try:
        if path:
            sys.path.insert(0, path)
        module = importlib.import_module("main")
        main_fn = getattr(module, "main", None)
        if main_fn:
            return main_fn
    except ImportError:
        pass
    return None


def bootstrap_main(args):
    """
    Main function explicitly called from the C++ code.
    Return the main application object.
    """
    version_info = sys.version_info
    if version_info.major != 3 or version_info.minor < 6:
        return None, "python36"
    proxy = HostLibProxy(HostLib)
    main_fn = None
    if len(args) > 2:
        path = os.path.abspath(args[2])
        main_fn = load_module_as_path(path)
        main_fn = main_fn or load_module_as_package(args[2])
        main_fn = main_fn or load_module_local(path)
    if len(args) >= 1:
        main_fn = main_fn or load_module_local()
    if main_fn:
        return main_fn(args, {"proxy": proxy}), None
    return None, "main"
