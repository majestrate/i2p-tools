
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
        samples.append((start, artt))

    s = sorted(samples, cmp=lambda x, y: cmp(x[0], y[0]))
    graphit(s)
    
def graphit(data):

    t, l = list(), list()
    for a,b in data:
        t.append(a)
        l.append(b)
    # the histogram of the data
    plt.bar(t, l, facecolor='green', alpha=0.5)
    plt.xlabel('Time')
    plt.ylabel('latency')
    plt.title('i2p latency graph')
    plt.savefig('latency.png')
               

            
if __name__ == '__main__':
    main()
