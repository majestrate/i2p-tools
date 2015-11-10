
import bsddb
import matplotlib.mlab as mlab
import matplotlib.pyplot as plt

def main():
    db = bsddb.hashopen('latency.db', 'r')

    samples = list()
    for k, v in db.iteritems():
        # extract key parts
        parts = k.split('|')
        remote, start, end = parts[0], int(parts[1]), int(parts[2])
        # extract value parts
        parts = v.split('|')
        count, timeout, lost, mrtt, artt = int(parts[0]), int(parts[1]), int(parts[2]), float(parts[3]), float(parts[4])
        # add entry
        samples.append((start, artt, mrtt, lost))

    s = sorted(samples, cmp=lambda x, y: cmp(x[0], y[0]))
    graphit(s)
    
def graphit(data):

    t, mean, mx, lost = list(), list(), list(), list()
    for a,b,c,d in data:
        t.append(a)
        mean.append(b)
        mx.append(c)
        lost.append(d / 10.0)

    plt.figure(1)
    plt.subplot(2, 1, 1) 
    plt.title('i2p reliabilty graph')
    plt.plot(t, lost)
    plt.xlabel('time')
    plt.ylabel('packet drop')
    plt.subplot(2, 1, 2) 
    plt.plot(t, mean)
    plt.plot(t, mx)
    plt.xlabel('time')
    plt.ylabel('latency (ms)')
    plt.savefig('latency.png')
               

            
if __name__ == '__main__':
    main()
