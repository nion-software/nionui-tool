import importlib
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
        HostLib.Core_out(str(stuff) if stuff is not None else str())
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
        return UserInterface.QtFontMetrics(width=font_metrics[0], height=font_metrics[1], ascent=font_metrics[2], descent=font_metrics[3], leading=font_metrics[4])


def bootstrap_main(args):
    """
    Main function explicitly called from the C++ code.
    Return the main application object.
    """
    proxy = HostLibProxy(HostLib)
    module_name = "main"
    if len(args) > 2:
        path = os.path.abspath(args[2])
        if os.path.isfile(path):
            dirname = os.path.dirname(path)
            module_name = os.path.splitext(os.path.basename(path))[0]
            sys.path.insert(0, dirname)
        else:
            sys.path.insert(0, path)
    module = importlib.import_module(module_name)
    return getattr(module, "main")(args, {"proxy": proxy})
