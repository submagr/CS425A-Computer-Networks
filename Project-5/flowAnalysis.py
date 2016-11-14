import csv
import config
import test
import math
import matplotlib.pyplot as plt

# ---------------Globals-------------------
def plotGraph(X, Y, log):
    plt.figure()
    if log:
        plt.loglog(
            X, Y, 
            basex=10,
            color='darkred', 
            linewidth = 0.5
        )
        plt.title('Log x-axis, Log y-axis')
    plt.grid(True)
    plt.plot(X, Y, 'bo--')
    plt.show()

def loadData():
    data = []
    with open(config.flowFile) as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            newRow = {}
            #TODO: Avoid Dictionary here
            for key, value in row.items():
                try: 
                    newRow[key] = float(value)
                except Exception as e:
                    newRow[key] = value
            data.append(newRow)
    return data

def getAvgPacketSize(data):
    avgPacketSize = 0.0
    for i, row in enumerate(data):
        avgPacketSize = (avgPacketSize*i + row["doctets"]/row["dpkts"])/(i+1)
    return avgPacketSize

def toLogarithmic():
    pass

def getBinBoundaries(minEntry, maxEntry, numBins):
    '''
        This function returns bin boundaries(histogram)
    '''
    binBoundaries = []
    binSize = (maxEntry-minEntry)/float(numBins)
    # python matplotlib provide this functionality
    #if not log:
    for i in range(1, numBins):
        binBoundaries.append(minEntry + i*binSize)
    binBoundaries.append(maxEntry)
    #else: 
    #    a = pow((maxEntry - minEntry), 1/float(numBins))
    #    for i in range(1, numBins):
    #        binBoundaries.append(minEntry + pow(a, i))
    #    binBoundaries.append(maxEntry)
    return binBoundaries

def getCcdf(Y):
    '''
        Input: list of int/float
        Output: Cumulative sum of list in O(n) time
    '''
    newY = [0 for x in range(len(Y))]
    newY[-1] = Y[-1]
    endIndex = max(0, len(Y)-2) # Last 2nd index
    for i in range(endIndex, -1, -1): 
        newY[i] = Y[i] + newY[i+1]
    return newY

def plotCcdf(entries, numBins, log = 0):
    '''
    Input: 
        entries: list of entries 
        numBins 
        log: Boolean to use logarithmic axes
    Output: 
        plot ccdf of entries
    '''
    assert(type(entries[0]) == float)
    sortedEntries = sorted(entries) # Sort in increasing order
    binSize = (sortedEntries[0] - sortedEntries[-1])/numBins # Size of one bin
    binBoundaries = getBinBoundaries(sortedEntries[0], sortedEntries[-1], numBins)
    X = [sortedEntries[0]] 
    Y = [0]
    entriesPtr = 0
    for i in range(numBins):
        X.append(binBoundaries[i])
        count = 0
        while entriesPtr < len(sortedEntries) and sortedEntries[entriesPtr] < binBoundaries[i]:
            count += 1
            entriesPtr += 1
        Y.append(count)
    Y[-1] += 1 # Include biggest entry in last bin automatically
    Y = getCcdf(Y) 
    plotGraph(X, Y, log)

def trafficToOrFromIp():
    pass

if __name__ == "__main__":
    if(config.debug):
        test.test_flowAnalysis()
    data = loadData() 
    print "Data Loaded"
    getAvgPacketSize(data)
    # Plot ccdf for duration
    durations = [x["last"] - x["first"] for x in data]
    plotCcdf(durations, 10, 0)
