import numpy as np
import matplotlib.pyplot as plt
import math
import re

class AVG:
    LOOP = 0
    PRSV = 1
    COMM = 2
    CDRT = 3
class STD:
    LOOP = 4
    PRSV = 5
    COMM = 6
    CDRT = 7

color_map = { 1000:'red', 512:'blue', 216:'c', 64:'green', 8:'m'}

class ProfInfo:
    ''' Data structure that captures data to represent histogram of CD profile '''
    def __init__(self, samples, app, ftype, tasks, inputs, profname):
        self.name_ = profname
        self.app_ = app
        self.type_ = ftype
        self.input_ = inputs
        self.nTask_ = tasks
        self.avg_ = np.mean(samples)
        self.std_ = np.std(samples)
        self.min_ = min(samples)
        self.max_ = max(samples)
        self.num_samples_ = len(samples)
        self.samples_ = samples
        self.binsize_ = ( 3.49*self.std_*(self.num_samples_**(-0.33333)) )

    def GetPercentile(self):
        return self.avg_ + self.std_*3
    def GetLegend(self, mylegend):
        if mylegend == '':
            return self.app_ + ' ' + self.name_ + ' ' + self.type_ + ' ' + str(self.nTask_)
        else:
            return self.app_ + ' ' + mylegend
    def GetColor(self, mycolor):
        if mycolor == '':
            return color_map[self.nTask_]
        else:
            return mycolor
    def GetTitle(self, mytitle=''):
        if mytitle == '':
            return 'Histogram of Latency with ' + self.app_ + ' (' + str(self.input_) + ')'
        else:
            return 'Histogram of Latency with ' + self.app_ + ' (' + mytitle + ')'
    def GetHistogram(self, binsize, maxtime):
        if binsize == 0:
           bin_size = self.max_
        else:
           bin_size = binsize
        if maxtime == 0:
           max_time = self.max_
        else:
            max_time = maxtime

        hist, bars = np.histogram(self.samples_, bins=np.linspace(0, max_time, int((max_time)/bin_size)))
        self.hist_ = np.divide(hist, self.num_samples_, dtype=float)
        self.bars_ = bars[1:]# + int(self.min_/bin_size) * bin_size
#        print "name:%s, input:%d, nTask:%d, %d, avg:%f, std:%f, min:%f, max:%f, binsize:%f (%f), #bins:%d\n" % \
#            (self.name_, self.input_, self.nTask_, self.num_samples_, 
#             self.avg_, self.std_, self.min_, self.max_, bin_size, self.binsize_, 
#             (self.max_ - self.min_)/self.binsize_)
#         print 'bars    :', bars
#         print 'selfbars:', self.bars_
#         print 'hist    :', hist
#         print 'hist    :', self.hist_
#         print 'xaxis   :', np.linspace(0, max_time, int((max_time)/bin_size))
#         print len(self.hist_), len(bars), int(self.min_/bin_size) * bin_size, bin_size, self.num_samples_

def GetBinSize(prof_info_list):
    ######################################################################
    # Get average bin size for multiple histograms
    ######################################################################
    binsize = 0.0
    min_binsize = 0.000001
    prof_info_cnt = 0
    for prof_info in prof_info_list:
        if prof_info.binsize_ > min_binsize:
            binsize += prof_info.binsize_
            prof_info_cnt += 1
    binsize /= prof_info_cnt

    bin_degree = math.floor(math.log(binsize, 10))
    min_bin_precision = 2.5 * 10 ** bin_degree
    if binsize > min_bin_precision :
        bin_size = round(binsize/min_bin_precision) * min_bin_precision
    else:
        min_bin_precision = 10 ** bin_degree
        bin_size = round(binsize/min_bin_precision) * min_bin_precision
    print binsize, bin_size, min_bin_precision
    return bin_size

def GetMaxSize(prof_info_list):
    maxval = 0.0
    for prof_info in prof_info_list:
        if prof_info.GetPercentile() > maxval:
            maxval = prof_info.GetPercentile()
    return maxval

def GetLatencyHist(prof_info_list, save_file=True, mytitle='', filename='latency_hist', mylegend='', mycolor=''): 
    ######################################################################
    # Get max time and bin size for multiple histograms
    ######################################################################
    binsize = GetBinSize(prof_info_list) 
    maxsize = GetMaxSize(prof_info_list)
    print 'binsize:%f, max:%f, # prof:%d\n' % (binsize, maxsize, len(prof_info_list))
    ######################################################################

    for prof_info in prof_info_list:
        prof_info.GetHistogram(binsize, maxsize)
    raw_input("\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n")
    
    bins_for_all = np.linspace(0, maxsize, int(maxsize/binsize))
    
    for prof_info in prof_info_list:
        plt.plot(prof_info.bars_, prof_info.hist_, \
                label=prof_info.GetLegend(mylegend))
        print prof_info.bars_
    plt.xlim(left=0) 
    plt.ylim(bottom=0) 
    plt.xlabel('Preservation Latency [s]') 
    plt.ylabel('Probability Density') 
    plt.legend()
    plt.title(prof_info_list[0].GetTitle(mytitle))
    plt.draw()
    plt.show()
    print filename
    if save_file:
        pdffig = plt.gcf()
        pdffig.savefig(filename+'.svg', format='svg', bbox_inches='tight')
        pdffig.savefig(filename+'.pdf', format='pdf', bbox_inches='tight')
    else:
        plt.show()
    plt.clf()
