__doc__ = """
config loader for pyi2ptunnel
"""

import json

def Load(fname):
    with open(fname) as f:
        data = f.read()
    return json.loads(data.strip())
