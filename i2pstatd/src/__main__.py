from i2pstatd import server
from os import path, environ

i2pdir = environ['I2P'] if 'I2P' in environ else path.join(environ['HOME'],'.i2p')
ndbdir = path.join(i2pdir,'netDb')
server.NDBStatServer(ndbdir).run()
