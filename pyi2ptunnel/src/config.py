__doc__ = """
config loader for pyi2ptunnel
"""

import json

def Load(fname):
    with open(fname) as f:
        return json.load(f)
