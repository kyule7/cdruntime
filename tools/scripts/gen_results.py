#!/usr/bin/python
import json
import pandas as pd
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
#MIN_BIN_PRECISION=0.025
#def createDataFrameFromProfile(results):
#    need_alloc_df = True
#    for eachpgsize in results:
#        for eachtrace in results[eachpgsize]: #each name is each trace for the same page size
#            list_of_each = results[eachpgsize][eachtrace]
#            list_of_each = sorted(list_of_each, key=lambda x: int(itemgetter('interval')(x)))
#            need_xaxis = True
#            plt.clf() # clear figure
#            fig = plt.figure(figsize=(12, 4.5))
#            for eachname in list_of_each:
#                target_name = eachname['name']
#                target_pagesize = pgsize2str(eachname['pagesize'])
#                dirtypage_list = eachname.pop('dirtypages', None)
#                pageinfo_list  = eachname.pop('pageinfo', None)
#                if(need_xaxis == True):
#                    xaxis = range(1, len(dirtypage_list)+1)
#                    need_xaxis = False
#                uniqueID = eachname['name']+'_'+pgsize2str(eachname['pagesize'])+'_'+eachname['interval']+'_'+eachname['pagefault_latency'];
#                dp_ax = plt.plot(xaxis, dirtypage_list, label=eachname['interval'])
#                print uniqueID, ', cudacalls:', len(dirtypage_list)
#                # Add each map object in list_of_each to pandas dataframe
#                if(need_alloc_df):
#                    # key is trace name_pagesize_interval
#                    df = pd.DataFrame(eachname.values(), index=eachname.keys(), columns=(uniqueID,))
#                    need_alloc_df = False
#                else:
#                    df[uniqueID] = pd.Series(eachname)
#
#            plt.title('The Number of Dirty Pages ('+target_pagesize+') at Intervals ('+target_name+')', fontsize=title_fontsize*0.8)
#            plt.ylabel("The number of Dirty Pages", fontsize=axis_fontsize)
#            plt.xlabel("Event Number (Beginning/Completion of Kernel)", fontsize=axis_fontsize)
#            fig = plt.gcf()
##            plt.show()
#            plt.legend(ncol=len(list_of_each))
#            plt.draw()
#            fig_name = eachname['name']+'_'+ target_pagesize
#            fig.savefig('dirtypages_'+fig_name + '.pdf', format='pdf', bbox_inches='tight')
#            fig.savefig('dirtypages_'+fig_name + '.svg', format='svg', bbox_inches='tight')
#            plt.close(fig)
#    return df

#eachfile = 'est.json'
##eachfile = 'test.json'
#with open(eachfile) as each:
#    eachjson = json.load(each)
#print eachjson

#def makeFlatCDs(flattened, cds_list):
#    for cds in cds_list: # list of sequential CDs
#        flattened[cds] = {}
#        for item in cds_list[cds]:
#            print item
#            if item != 'ChildCDs':
#                flattened[cds][item] = cds_list[cds][item]
#            else:
#                makeFlatCDs(flattened, cds_list[cds][item])
#    print flattened

def gatherJSONObj(filelist):
    newfile = open(filelist, 'r')
    for line in newfile:
        file_list = line.split(" ")

#    print file_list
    gathered_input_tasks = {}
    gathered_tasks_input = {}
    tg = []
    for each_filename in file_list:
        print each_filename
        with open(each_filename) as each:
            eachjson = json.load(each)
            #pd.read_json(
            #df = pd.DataFrame.from_dict(eachjson, orient='index')
#            print eachjson
            # exec_name-ftype-numTasks-input
#            print eachjson
            appname = eachjson['name' ]
            inputs  = eachjson['input']
            nTask   = eachjson['nTask']
            prof    = eachjson['prof' ]
#            dump    = eachjson['dump' ]
#            wait    = eachjson['wait' ]
#            cdrt    = eachjson['cdrt' ]
#            print exec_name, fail_type, numTasks, input_size
#            df = pd.DataFrame.from_dict(cd_info, orient='index')
#            print cd_info
#            df.reset_index(level=0, inplace=True)
            #print df
#            print cd_info
#            if gathered_input_tasks[each_name][fail_type][numTasks][input_size] 
            if appname not in gathered_input_tasks:
                gathered_input_tasks[appname] = {}
                gathered_tasks_input[appname] = {}
            if inputs not in gathered_input_tasks[appname]:
                gathered_input_tasks[appname][inputs] = {}
            if nTask not in gathered_input_tasks[appname][inputs]:
                gathered_input_tasks[appname][inputs][nTask] = { 'loop':[], 'prsv':[], 'comm':[], 'cdrt':[], 'phase':[] }

            if nTask not in gathered_tasks_input[appname]:
                gathered_tasks_input[appname][nTask] = {}
            if inputs not in gathered_tasks_input[appname][nTask]:
                gathered_tasks_input[appname][nTask][inputs] = { 'loop':[], 'prsv':[], 'comm':[], 'cdrt':[], 'phase':[] }
            #print cd_info
#            df = pd.DataFrame.from_dict(cd_info, orient='index')
#            print '=======================\n'
#            print df
            #flattened = {}
#            for cds in cd_info:
#                flattened[cds] = {}
#                for item in cd_info[cds]:
#                    print item
#                    if item != 'ChildCDs':
#                        flattened[cds][item] = cd_info[cds][item]
            #makeFlatCDs(flattened, cd_info)
            #print '~!~!~!~!~!~!~!~!'
            #print flattened

            gathered_input_tasks[appname][inputs][nTask]["loop"].extend(prof["loop"])
            gathered_input_tasks[appname][inputs][nTask]["prsv"].extend(prof["dump"])
            gathered_input_tasks[appname][inputs][nTask]["comm"].extend(prof["wait"])
            gathered_input_tasks[appname][inputs][nTask]["cdrt"].extend(prof["cdrt"])

            gathered_input_tasks[appname][inputs][nTask]["phase"] = []
            if "dump0" in prof:
                gathered_input_tasks[appname][inputs][nTask]["phase"].append(prof["dump0"])
                #print "\ndump"
                #print gathered_input_tasks[appname][inputs][nTask]["phase"]
                #raw_input("\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n")
            if "dump1" in prof:
                gathered_input_tasks[appname][inputs][nTask]["phase"].append(prof["dump1"])
                #print "\ndump"
                #print gathered_input_tasks[appname][inputs][nTask]["phase"]
                #raw_input("\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n")
            if "dump2" in prof:
                gathered_input_tasks[appname][inputs][nTask]["phase"].append(prof["dump2"])
            if "dump3" in prof:
                gathered_input_tasks[appname][inputs][nTask]["phase"].append(prof["dump3"])
            if "dump4" in prof:
                gathered_input_tasks[appname][inputs][nTask]["phase"].append(prof["dump4"])
            gathered_tasks_input[appname][nTask][inputs]["loop"].extend(prof["loop"])
            gathered_tasks_input[appname][nTask][inputs]["prsv"].extend(prof["dump"])
            gathered_tasks_input[appname][nTask][inputs]["comm"].extend(prof["wait"])
            gathered_tasks_input[appname][nTask][inputs]["cdrt"].extend(prof["cdrt"])
            gathered_tasks_input[appname][nTask][inputs]["phase"] = []
            if "dump0" in prof:
                gathered_tasks_input[appname][nTask][inputs]["phase"].append(prof["dump0"])
            if "dump1" in prof:
                gathered_tasks_input[appname][nTask][inputs]["phase"].append(prof["dump1"])
            if "dump2" in prof:
                gathered_tasks_input[appname][nTask][inputs]["phase"].append(prof["dump2"])
            if "dump3" in prof:
                gathered_tasks_input[appname][nTask][inputs]["phase"].append(prof["dump3"])
            if "dump4" in prof:
                gathered_tasks_input[appname][nTask][inputs]["phase"].append(prof["dump4"])
    return gathered_input_tasks, gathered_tasks_input

def genEmptyDF(apps, inputs, nTasks):
    apps   = ['full', 'incr', 'cdrt_incr', 'cdrt_opt']
    inputs = [40, 60, 80]
    nTasks = [8, 64, 216, 512, 1000]
    miindex = pd.MultiIndex.from_product([apps,
                                          inputs,
                                          nTasks])
    
    micolumns = pd.MultiIndex.from_tuples([ ('errFree','avg','bare'), ('errFree','avg','cd_noprv'), 
      ('errFree','avg','loop'),('errFree','avg','prsv'), ('errFree','avg','comm'),('errFree','avg','rtov'),
      ('errFree','std','loop'),('errFree','std','prsv'), ('errFree','std','comm'),('errFree','std','rtov'),
      ('errFree','min','loop'),('errFree','min','prsv'), ('errFree','min','comm'),('errFree','min','rtov'),
      ('errFree','max','loop'),('errFree','max','prsv'), ('errFree','max','comm'),('errFree','max','rtov'),
#      ('fRate_0','tot','exec'),('fRate_0','tot','prsv'), ('fRate_0','tot','comm'),('fRate_0','tot','rtov'),
#      ('fRate_0','lv0','exec'),('fRate_0','lv0','prsv'), ('fRate_0','lv0','comm'),('fRate_0','lv0','rtov'),
#      ('fRate_0','lv1','exec'),('fRate_0','lv1','prsv'), ('fRate_0','lv1','comm'),('fRate_0','lv1','rtov'),
#      ('fRate_0','lv2','exec'),('fRate_0','lv2','prsv'), ('fRate_0','lv2','comm'),('fRate_0','lv2','rtov'),
                                          ],
                                          names=['failure', 'level', 'breakdown'])
    
    dfmi = pd.DataFrame(np.zeros(len(miindex)*len(micolumns)).reshape((len(miindex),len(micolumns))),
                        index=miindex,
                        columns=micolumns).sort_index().sort_index(axis=1)
    return dfmi

def getDataFrame(gathered_dict):
    dfmi = genEmptyDF(['full', 'incr', 'cdrt_incr', 'cdrt_opt'],
                      [40, 60, 80],
                      [8, 64, 216, 512, 1000])
    hist_set = {}
    for apps in gathered_dict:
        hist_set[apps] = {}
        for inputs in gathered_dict[apps]:
            print '######################## --> ', inputs, apps
            prof = { 'loop':[], 'prsv':[], 'comm':[], 'cdrt':[], 'phase':[] }
            hist = { 'loop':[], 'prsv':[], 'comm':[], 'cdrt':[], 'phase':[] }
            hist_set[apps][inputs] = {}
            for nTasks in gathered_dict[apps][inputs]:
                print '%s: nTask:%d, input:%d' % (apps, nTasks, inputs)
                prof['loop'].append(ProfInfo(gathered_dict[apps][inputs][nTasks]['loop'], inputs, nTasks, 'loop'))
                prof['prsv'].append(ProfInfo(gathered_dict[apps][inputs][nTasks]['prsv'], inputs, nTasks, 'prsv'))
                prof['comm'].append(ProfInfo(gathered_dict[apps][inputs][nTasks]['comm'], inputs, nTasks, 'comm'))
                prof['cdrt'].append(ProfInfo(gathered_dict[apps][inputs][nTasks]['cdrt'], inputs, nTasks, 'cdrt'))
                counter = 0
                for ph in gathered_dict[apps][inputs][nTasks]['phase']:
                    prof['phase'].append(ProfInfo(ph, inputs, nTasks, 'phase' + str(counter)))
                    counter += 1
                maptype = re.split(r'_', apps)[1]
                if maptype == 'bare':
                    dfmi['errFree','avg','bare'].loc['full'][inputs][nTasks] = prof['loop'][-1].avg_ 
                    dfmi['errFree','avg','bare'].loc['incr'][inputs][nTasks] = prof['loop'][-1].avg_ 
                    dfmi['errFree','avg','bare'].loc['cdrt_incr'][inputs][nTasks] = prof['loop'][-1].avg_ 
                    dfmi['errFree','avg','bare'].loc['cdrt_opt'][inputs][nTasks]  = prof['loop'][-1].avg_ 
                elif maptype == 'noprv':
                    dfmi['errFree','avg','cd_noprv'].loc['cdrt_incr'][inputs][nTasks] = prof['loop'][-1].avg_ 
                elif maptype == 'optnoprv':
                    dfmi['errFree','avg','cd_noprv'].loc['cdrt_opt'][inputs][nTasks] = prof['loop'][-1].avg_ 
                else:
                    if maptype == 'cdrt':
                        newmaptype = 'cdrt_incr'
                    else:
                        newmaptype = maptype
                    dfmi['errFree','avg','loop'].loc[newmaptype][inputs][nTasks] =  prof['loop'][-1].avg_
                    dfmi['errFree','avg','prsv'].loc[newmaptype][inputs][nTasks] =  prof['prsv'][-1].avg_
                    dfmi['errFree','avg','comm'].loc[newmaptype][inputs][nTasks] =  prof['comm'][-1].avg_
                    dfmi['errFree','avg','rtov'].loc[newmaptype][inputs][nTasks] =  prof['cdrt'][-1].avg_
                    dfmi['errFree','std','loop'].loc[newmaptype][inputs][nTasks] =  prof['loop'][-1].std_
                    dfmi['errFree','std','prsv'].loc[newmaptype][inputs][nTasks] =  prof['prsv'][-1].std_
                    dfmi['errFree','std','comm'].loc[newmaptype][inputs][nTasks] =  prof['comm'][-1].std_
                    dfmi['errFree','std','rtov'].loc[newmaptype][inputs][nTasks] =  prof['cdrt'][-1].std_
                    dfmi['errFree','min','loop'].loc[newmaptype][inputs][nTasks] =  prof['loop'][-1].min_
                    dfmi['errFree','min','prsv'].loc[newmaptype][inputs][nTasks] =  prof['prsv'][-1].min_
                    dfmi['errFree','min','comm'].loc[newmaptype][inputs][nTasks] =  prof['comm'][-1].min_
                    dfmi['errFree','min','rtov'].loc[newmaptype][inputs][nTasks] =  prof['cdrt'][-1].min_
                    dfmi['errFree','max','loop'].loc[newmaptype][inputs][nTasks] =  prof['loop'][-1].max_
                    dfmi['errFree','max','prsv'].loc[newmaptype][inputs][nTasks] =  prof['prsv'][-1].max_
                    dfmi['errFree','max','comm'].loc[newmaptype][inputs][nTasks] =  prof['comm'][-1].max_
                    dfmi['errFree','max','rtov'].loc[newmaptype][inputs][nTasks] =  prof['cdrt'][-1].max_
    
            # Get binsize for loop and prsv
            binsize = 0.0
            if len(prof['phase']) == 0:
                for prof_info in prof['loop']:
                    binsize += prof_info.binsize_
                for prof_info in prof['prsv']:
                    binsize += prof_info.binsize_
                binsize /= (len(prof['loop']) + len(prof['prsv']))
            else:
                for prof_info in prof['phase']:
                    binsize += prof_info.binsize_
                binsize /= len(prof['phase'])
    
            bin_degree = math.floor(math.log(binsize, 10))
            min_bin_precision = 2.5 * 10 ** bin_degree
            print binsize, len(prof['loop']), min_bin_precision
            if binsize > min_bin_precision :
                binsize = round(binsize/min_bin_precision) * min_bin_precision
            else:
                min_bin_precision = 10 ** bin_degree
                binsize = round(binsize/min_bin_precision) * min_bin_precision
            
            print 'bin_degree:%f, binsize:%f, len:%f\n' % (bin_degree, binsize, len(prof['loop']))
#            raw_input('~!~!~!~!')
    
            # Get histogram for each loop and prsv
            maxval = 0.0
            for prof_info in prof['loop']:
                prof_info.GetHistogram(binsize)
                if prof_info.max_ > maxval:
                    maxval = prof_info.max_
            for prof_info in prof['prsv']:
                if prof_info.avg_ > 0.0001: 
                    prof_info.GetHistogram(binsize)
                    if prof_info.max_ > maxval:
                        maxval = prof_info.max_
            for prof_info in prof['phase']:
                prof_info.GetHistogram(binsize)
                print "\nprof_info"
                print prof_info
                raw_input("\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n")
    
            bins_for_all = np.linspace(0, maxval, int(maxval/binsize))
    
            for prof_info in prof['loop']:
                plt.plot(prof_info.bars_, prof_info.hist_, \
                        label=(prof_info.name_ +'('+ str(prof_info.nTask_)+')'), \
                        color=color_map[prof_info.nTask_])
                print prof_info.bars_
            
            for prof_info in prof['prsv']:
                if prof_info.avg_ > 0.0001: 
                    plt.plot(prof_info.bars_, prof_info.hist_, \
                            label=(prof_info.name_ +'('+ str(prof_info.nTask_)+')'), \
                            color=color_map[prof_info.nTask_] )
                    print prof_info.bars_
            for prof_info in prof['phase']:
                plt.plot(prof_info.bars_, prof_info.hist_, \
                        label=(prof_info.name_ + '('+str(prof_info.nTask_)+')'), \
                        color=color_map[prof_info.nTask_])
            plt.legend()
            plt.title('Latency of loop and preservation (app:' + apps + ', input:'+str(inputs)+')')
    #        plt.show()
            pdffig = plt.gcf()
            plt.draw()
            filename = 'latency_' + apps + '_' + str(inputs)
            pdffig.savefig(filename+'.svg', format='svg', bbox_inches='tight')
            pdffig.savefig(filename+'.pdf', format='pdf', bbox_inches='tight')
            plt.clf()
            hist_set[apps][inputs] = hist
    return dfmi, hist_set
############################################################################################333




#def GetHistogram(samples, binlen, binsize):
#    ret = range(0, binlen)
##    print binlen
#    for i in ret:
#        ret[i] = 0
#    for sample in samples:
#        ret[int(sample/binsize)] += 1
#    return ret

class ProfInfo:
    def __init__(self, samples, inputs, nTasks, profname):
        self.name_ = profname
        self.input_ = inputs
        self.nTask_ = nTasks
        self.avg_ = np.mean(samples)
        self.std_ = np.std(samples)
        self.min_ = min(samples)
        self.max_ = max(samples)
        self.num_samples_ = len(samples)
        self.samples_ = samples
        self.binsize_ = GetBinsize(self.std_, self.num_samples_)

    def GetHistogram(self, binsize):
#        print int(self.max_/self.binsize_)
        print "name:%s, input:%d, nTask:%d, %d, avg:%f, std:%f, min:%f, max:%f, binsize:%f (%f), #bins:%d\n" % (self.name_, self.input_, self.nTask_, self.num_samples_, self.avg_, self.std_, self.min_, self.max_, binsize, self.binsize_, (self.max_ - self.min_)/self.binsize_)
        hist, bars = np.histogram(self.samples_, bins=np.linspace(0, self.max_, int((self.max_)/binsize)))
#        #np.divide(hist, self.num_samples_)
##        print hist
        self.hist_ = np.divide(hist,self.num_samples_, dtype=float)
        self.bars_ = bars[1:]# + int(self.min_/binsize) * binsize
        print bars
        print self.bars_
        print len(self.hist_), len(bars), int(self.min_/binsize) * binsize, binsize
#        plt.plot(bars[1:], self.hist_, self.bars_, self.hist_)

#        plt.show()
#        hist2 = GetHistogram(samples, int(self.max_/self.binsize_)+1, self.binsize_)
#        hist2 = np.divide(hist2, self.num_samples_, dtype=float)
#        plt.plot(hist2)
#        plt.show()
        #print "name:%s, input:%d, nTask:%d, %d, avg:%f, std:%f, min:%f, max:%f, binsize:%f, #bins:%d\n" % (self.name_, self.input_, self.nTask_, self.num_samples_, self.avg_, self.std_, self.min_, self.max_, self.binsize_, (self.max_ - self.min_)/self.binsize_)

        #raw_input('####################################')


def GetAvgStdHist(prof):
    avg = {}
    avg['loop'] = np.mean(prof['loop'])
    avg['prsv'] = np.mean(prof['prsv'])
    avg['comm'] = np.mean(prof['wait'])
    avg['cdrt'] = np.mean(prof['cdrt'])
    std = {}
    std['loop'] = np.std(prof['loop'])
    std['prsv'] = np.std(prof['prsv'])
    std['comm'] = np.std(prof['wait'])
    std['cdrt'] = np.std(prof['cdrt'])
    hist = {}
    hist['loop'] = np.hist(prof['loop'])
    hist['prsv'] = np.hist(prof['prsv'])
    hist['comm'] = np.hist(prof['wait'])
    hist['cdrt'] = np.hist(prof['cdrt'])
    return avg, std, hist

def GetBinsize(std, nSamples):
#    print 'std:', std, nSamples
    return ( 3.49*std*(nSamples**(-0.33333)) )

sweep_rank, sweep_input = gatherJSONObj('file_list.txt')

mydf, hist_sweep_rank = getDataFrame(sweep_rank)

mydf.to_csv("results_gathered.csv")


print '\n\n\n\n'
print mydf
#for apps in sweep_rank:
#    for inputs in sweep_rank[apps]:
#        for nTasks in sweep_rank[apps][inputs]:
#            prof = sweep_rank[apps][inputs][nTasks]
#            avg = {}
#            avg['loop'] = np.avg(prof['loop'])
#            avg['prsv'] = np.avg(prof['prsv'])
#            avg['comm'] = np.avg(prof['wait'])
#            avg['cdrt'] = np.avg(prof['cdrt'])
#            std = {}
#            std['loop'] = np.std(prof['loop'])
#            std['prsv'] = np.std(prof['prsv'])
#            std['comm'] = np.std(prof['wait'])
#            std['cdrt'] = np.std(prof['cdrt'])
#            hist = {}
#            binsize = GetBinsize(std['loop'], len(prof['loop']))
#            print "binsize:%f, nbins:%d\n" % (binsize, int(max(prof['loop']) / binsize))
#            binsize2 = GetBinsize(std['comm'], len(prof['wait']))
#            print "binsize:%f, nbins:%d\n" % (binsize2, int(max(prof['wait']) / binsize2))
#            time_axis = [ i*binsize in range(int(max(prof['loop']) / binsize)) ]
#            raw_input('.................')
#            hist['loop'] = np.hist(prof['loop'], bins=time_axis)
#            hist['prsv'] = np.hist(prof['prsv'], bins=time_axis)
#            hist['comm'] = np.hist(prof['wait'], bins=time_axis)
#            hist['cdrt'] = np.hist(prof['cdrt'], bins=time_axis)
#           hist_set = {}
#           
#           for apps in sweep_rank:
#               hist_set[apps] = {}
#               for inputs in sweep_rank[apps]:
#                   print '######################## --> ', inputs, apps
#           #        if apps == "lulesh_cdrt":
#                   prof = { 'loop':[], 'prsv':[], 'comm':[], 'cdrt':[], 'phase':[] }
#                   hist = { 'loop':[], 'prsv':[], 'comm':[], 'cdrt':[], 'phase':[] }
#           #        else:
#           #        prof = { 'loop':[], 'prsv':[], 'comm':[], 'cdrt':[] }
#           #        hist = { 'loop':[], 'prsv':[], 'comm':[], 'cdrt':[] }
#                   hist_set[apps][inputs] = {}
#                   for nTasks in sweep_rank[apps][inputs]:
#                       print '%s: nTask:%d, input:%d' % (apps, nTasks, inputs)
#                       prof['loop'].append(ProfInfo(sweep_rank[apps][inputs][nTasks]['loop'], inputs, nTasks, 'loop'))
#                       prof['prsv'].append(ProfInfo(sweep_rank[apps][inputs][nTasks]['prsv'], inputs, nTasks, 'prsv'))
#                       prof['comm'].append(ProfInfo(sweep_rank[apps][inputs][nTasks]['comm'], inputs, nTasks, 'comm'))
#                       prof['cdrt'].append(ProfInfo(sweep_rank[apps][inputs][nTasks]['cdrt'], inputs, nTasks, 'cdrt'))
#                       counter = 0
#                       for ph in sweep_rank[apps][inputs][nTasks]['phase']:
#                           prof['phase'].append(ProfInfo(ph, inputs, nTasks, 'phase' + str(counter)))
#                           counter += 1
#           
#                   # Get binsize for loop and prsv
#                   binsize = 0.0
#                   if len(prof['phase']) == 0:
#                       for prof_info in prof['loop']:
#                           binsize += prof_info.binsize_
#                       for prof_info in prof['prsv']:
#                           binsize += prof_info.binsize_
#                       binsize /= (len(prof['loop']) + len(prof['prsv']))
#                   else:
#                       for prof_info in prof['phase']:
#                           binsize += prof_info.binsize_
#                       binsize /= len(prof['phase'])
#           
#                   bin_degree = math.floor(math.log(binsize, 10))
#                   min_bin_precision = 2.5 * 10 ** bin_degree
#                   print binsize, len(prof['loop']), min_bin_precision
#                   if binsize > min_bin_precision :
#                       binsize = round(binsize/min_bin_precision) * min_bin_precision
#                   else:
#                       min_bin_precision = 10 ** bin_degree
#                       binsize = round(binsize/min_bin_precision) * min_bin_precision
#                   
#                   print 'bin_degree:%f, binsize:%f, len:%f\n' % (bin_degree, binsize, len(prof['loop']))
#                   raw_input('~!~!~!~!')
#           
#                   # Get histogram for each loop and prsv
#                   maxval = 0.0
#                   for prof_info in prof['loop']:
#                       prof_info.GetHistogram(binsize)
#                       if prof_info.max_ > maxval:
#                           maxval = prof_info.max_
#                   for prof_info in prof['prsv']:
#                       if prof_info.avg_ > 0.0001: 
#                           prof_info.GetHistogram(binsize)
#                           if prof_info.max_ > maxval:
#                               maxval = prof_info.max_
#                   for prof_info in prof['phase']:
#                       prof_info.GetHistogram(binsize)
#                       print "\nprof_info"
#                       print prof_info
#                       raw_input("\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n")
#           
#                   bins_for_all = np.linspace(0, maxval, int(maxval/binsize))
#           
#                   for prof_info in prof['loop']:
#                       plt.plot(prof_info.bars_, prof_info.hist_, label=(prof_info.name_ +'('+ str(prof_info.nTask_)+')'), color=color_map[prof_info.nTask_])
#                       print prof_info.bars_
#                   
#                   for prof_info in prof['prsv']:
#                       if prof_info.avg_ > 0.0001: 
#                           plt.plot(prof_info.bars_, prof_info.hist_, label=(prof_info.name_ +'('+ str(prof_info.nTask_)+')'), color=color_map[prof_info.nTask_] )
#                           print prof_info.bars_
#                   for prof_info in prof['phase']:
#                       plt.plot(prof_info.bars_, prof_info.hist_, label=(prof_info.name_ + '('+str(prof_info.nTask_)+')'), color=color_map[prof_info.nTask_])
#                   plt.legend()
#                   plt.title('Latency of loop and preservation (app:' + apps + ', input:'+str(inputs)+')')
#           #        plt.show()
#                   pdffig = plt.gcf()
#                   plt.draw()
#                   filename = 'latency_' + apps + '_' + str(inputs)
#                   pdffig.savefig(filename+'.svg', format='svg', bbox_inches='tight')
#                   pdffig.savefig(filename+'.pdf', format='pdf', bbox_inches='tight')
#                   plt.clf()
#                   hist_set[apps][inputs] = hist
#           #        hist_set[inputs] = min_bin_precision
#           #        print prof['loop']
#                   #raw_input('~~~~~~~~~~~~~~~~~~~~~~~')
#           #            prof[nTasks] = [np.mean(sweep_rank[apps][inputs][nTasks]['loop']),
#           #                            np.mean(sweep_rank[apps][inputs][nTasks]['dump']),
#           #                            np.mean(sweep_rank[apps][inputs][nTasks]['wait']),
#           #                            np.mean(sweep_rank[apps][inputs][nTasks]['cdrt']),
#           #                             np.std(sweep_rank[apps][inputs][nTasks]['loop']),
#           #                             np.std(sweep_rank[apps][inputs][nTasks]['dump']),
#           #                             np.std(sweep_rank[apps][inputs][nTasks]['wait']),
#           #                             np.std(sweep_rank[apps][inputs][nTasks]['cdrt'])]
#           #            print 'nTasks', nTasks
#           ##        avg = {}
#           ##        avg['loop'] = np.mean(prof['loop'])
#           ##        avg['prsv'] = np.mean(prof['prsv'])
#           ##        avg['comm'] = np.mean(prof['comm'])
#           ##        avg['cdrt'] = np.mean(prof['cdrt'])
#           ##        std = {}
#           ##        std['loop'] = np.std(prof['loop'])
#           ##        std['prsv'] = np.std(prof['prsv'])
#           ##        std['comm'] = np.std(prof['comm'])
#           ##        std['cdrt'] = np.std(prof['cdrt'])
#           #        hist = {}
#           #        binsize0 = GetBinsize(prof[1000][STD.LOOP], len(prof['loop']))
#           #        binsize1 = GetBinsize(prof[1000][STD.PRSV], len(prof['prsv']))
#           #        binsize2 = GetBinsize(prof[1000][STD.COMM], len(prof['comm']))
#           #        binsize3 = GetBinsize(prof[1000][STD.CDRT], len(prof['cdrt']))
#           #        print 'bins:', binsize0, binsize1, binsize2, binsize3
#           #        raw_input('..1...............')
#           #        #binsize = GetBinsize(std['loop'], len(prof['loop']))
#           #        print binsize
#           #        print int(max(prof['loop']) / binsize)
#           #        #print "binsize:%f, nbins:%d\n" % (binsize, int(max(prof['loop']) / binsize))
#           #        binsize2 = GetBinsize(std['comm'], len(prof['wait']))
#           #        print "binsize:%f, nbins:%d\n" % (binsize2, int(max(prof['wait']) / binsize2))
#           #        time_axis = [ i*binsize in range(int(max(prof['loop']) / binsize)) ]
#           #        raw_input('.................')
#           #        hist['loop'] = np.hist(prof['loop'], bins=time_axis)
#           #        hist['prsv'] = np.hist(prof['prsv'], bins=time_axis)
#           #        hist['comm'] = np.hist(prof['comm'], bins=time_axis)
#           #        hist['cdrt'] = np.hist(prof['cdrt'], bins=time_axis)
           










# for apps in sweep_rank:
#     for inputs in sweep_rank[apps]:
#         prof = { 'loop':[], 'prsv':[], 'comm':[], 'cdrt':[] }
#         hist = { 'loop':[], 'prsv':[], 'comm':[], 'cdrt':[] }
#         for nTasks in sweep_rank[apps][inputs]:
#             print 'nTask:%d, input:%d' % (nTasks, inputs)
#             prof['loop'].append(sweep_rank[apps][inputs][nTasks]['loop'])
#             prof['prsv'].append(sweep_rank[apps][inputs][nTasks]['dump'])
#             prof['comm'].append(sweep_rank[apps][inputs][nTasks]['wait'])
#             prof['cdrt'].append(sweep_rank[apps][inputs][nTasks]['cdrt'])
#             prof[nTasks] = [np.mean(sweep_rank[apps][inputs][nTasks]['loop']),
#                             np.mean(sweep_rank[apps][inputs][nTasks]['dump']),
#                             np.mean(sweep_rank[apps][inputs][nTasks]['wait']),
#                             np.mean(sweep_rank[apps][inputs][nTasks]['cdrt']),
#                              np.std(sweep_rank[apps][inputs][nTasks]['loop']),
#                              np.std(sweep_rank[apps][inputs][nTasks]['dump']),
#                              np.std(sweep_rank[apps][inputs][nTasks]['wait']),
#                              np.std(sweep_rank[apps][inputs][nTasks]['cdrt'])]
#             print 'nTasks', nTasks
# #        avg = {}
# #        avg['loop'] = np.mean(prof['loop'])
# #        avg['prsv'] = np.mean(prof['prsv'])
# #        avg['comm'] = np.mean(prof['comm'])
# #        avg['cdrt'] = np.mean(prof['cdrt'])
# #        std = {}
# #        std['loop'] = np.std(prof['loop'])
# #        std['prsv'] = np.std(prof['prsv'])
# #        std['comm'] = np.std(prof['comm'])
# #        std['cdrt'] = np.std(prof['cdrt'])
#         hist = {}
#         binsize0 = GetBinsize(prof[1000][STD.LOOP], len(prof['loop']))
#         binsize1 = GetBinsize(prof[1000][STD.PRSV], len(prof['prsv']))
#         binsize2 = GetBinsize(prof[1000][STD.COMM], len(prof['comm']))
#         binsize3 = GetBinsize(prof[1000][STD.CDRT], len(prof['cdrt']))
#         print 'bins:', binsize0, binsize1, binsize2, binsize3
#         raw_input('..1...............')
#         #binsize = GetBinsize(std['loop'], len(prof['loop']))
#         print binsize
#         print int(max(prof['loop']) / binsize)
#         #print "binsize:%f, nbins:%d\n" % (binsize, int(max(prof['loop']) / binsize))
#         binsize2 = GetBinsize(std['comm'], len(prof['wait']))
#         print "binsize:%f, nbins:%d\n" % (binsize2, int(max(prof['wait']) / binsize2))
#         time_axis = [ i*binsize in range(int(max(prof['loop']) / binsize)) ]
#         raw_input('.................')
#         hist['loop'] = np.hist(prof['loop'], bins=time_axis)
#         hist['prsv'] = np.hist(prof['prsv'], bins=time_axis)
#         hist['comm'] = np.hist(prof['comm'], bins=time_axis)
#         hist['cdrt'] = np.hist(prof['cdrt'], bins=time_axis)
           













##df = pd.DataFrame.from_dict(result, orient='index')
#for app in result:
#    for ftype in result[app]:
#        for nTasks in result[app][ftype]:
#            for inputsize in result[app][ftype][nTasks]:
#                # Now we have all the measurements for the same configs
##                avg_cdinfo = {}
#                avg_result = averageCDTree(result[app][ftype][nTasks][inputsize])
#                print "\n\n\n Final~~~~~~~~~~~~~``"
#                printResults(avg_result)
##                for cdinfo in result[app][ftype][nTasks][inputsize]:
##
##                    print '=======================\n'
##                    print cdinfo
##                    print '=======================\n'
##                    for root in cdinfo:
##                        if root[0] == 'C' and root[1] == 'D':
##                            print root, len(cdinfo[root])
##                            for elem in cdinfo[root]:
##                                print cdinfo[root]["exec"]["max"]
##
#
#
#
#
#
#
#
#
#
#
#
#
#
#
#







