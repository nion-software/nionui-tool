# print ACK and exit

class Application:
    def start(self):
        print("ACK")
        return False

def main(args, bootstrap_args):
    # importing a compiled extension with bundled native libraries verifies the launcher loads Python correctly
    import numpy
    return Application()
