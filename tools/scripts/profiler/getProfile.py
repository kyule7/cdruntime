#!/usr/bin/python
import pandas as pd
import argparse as ap
from gen_histogram import *
pd.options.mode.chained_assignment = None
def processProf(prof):
    for exec_name in prof:
        for fail_type in prof[exec_name]:
            for numTasks in prof[exec_name][fail_type]:
                for input_size in prof[exec_name][fail_type][numTasks]:
                    print '\t\t', exec_name, fail_type, numTasks, input_size
                    for elem in prof[exec_name][fail_type][numTasks][input_size]:
                        print '----', elem, prof[exec_name][fail_type][numTasks][input_size][elem]
#                        for tr in prof[exec_name][fail_type][numTasks][input_size][elem]:
#                            print elem, tr, prof[exec_name][fail_type][numTasks][input_size][elem][tr]

def CheckMissing(result, nTask, inputsize):
#    missing = result.get('orign') == None
#    if missing:
#        missing = result.get('bare') == None
#        if missing == False:
#            orign = 'bare'
#    else:
#        orign = 'orign'
#    missing_prv = (result.get('noprv') == None)
#    missing_noerr = result.get('noerr') == None
#    if missing_noerr:
#        missing_noerr = result.get('errFree') == None
#        if missing_noerr == False:
#            noerr = 'errFree'
#    else:
#        noerr = 'noerr'
#
#    missing_error = (result.get('error') == None)
    ftype_list = {}
    orign = ''
    noprv = ''
    noerr = ''
    error = ''
    for ftype in result:
        ftype_list[ftype] = 0
        if ftype.find('orign') != -1:
            orign = ftype
        elif ftype.find('bare') != -1:
            orign = 'bare'
        elif ftype.find('noprv') != -1:
            noprv = 'noprv'
        elif ftype.find('noerr') != -1:
            noerr = 'noerr'
        elif ftype.find('errFree') != -1:
            noerr = 'errFree'
        elif ftype.find('error') != -1:
            error = 'error'
        elif ftype.find('Ftype') != -1:
            error = ftype
        #print ftype        
    missing = (orign is '') or (noprv is '') or (noerr is '') or (error is '')
#    print 'check missing:', nTask, inputsize, orign, noprv, noerr, error
#    for ftype in ftype_list:
#        print ftype        
#    raw_input('-ftype-')
    if missing == False:
        missing |= result[orign].get(nTask) == None
        missing |= result[noprv].get(nTask) == None
        missing |= result[noerr].get(nTask) == None
        if missing == False:
            missing |= result[orign][nTask].get(inputsize) == None
            missing |= result[noprv][nTask].get(inputsize) == None
            missing |= result[noerr][nTask].get(inputsize) == None
    return missing, orign, noprv, noerr, error
def checkMissing(result, types, nTask, inputsize):
    missing = False
    missing |= (result.get('orign') == None and result.get('bare') == None)
    missing |= (result.get('noprv') == None)
    missing |= (result.get('noerr') == None and result.get('errFree') == None)
    if missing == False:
        missing |= result[app][ftype].get(nTask) == None
        missing |= result[app][ftype].get(nTask) == None
        missing |= result[app][ftype].get(nTask) == None
        if missing == False:
            missing |= result[app_bare][ftype][nTask].get(inputsize) == None
            missing |= result[app_nopv][ftype][nTask].get(inputsize) == None
            missing |= result[app_noer][ftype][nTask].get(inputsize) == None
    return missing

def GenEmptyDF(apps, inputs, nTasks, labels):
    miindex = pd.MultiIndex.from_product([apps,
                                          inputs,
                                          nTasks])
    micolumns = pd.MultiIndex.from_product( labels )
    
    dfmi = pd.DataFrame(np.zeros(len(miindex)*len(micolumns)).reshape((len(miindex),len(micolumns))),
                        index=miindex,
                        columns=micolumns).sort_index().sort_index(axis=1)
    return dfmi


message_time = "messaging time"
reex_time = "reex time"    
def GetTotProf(tot_prof, orign, noprv, noerr, error, apps, tasks, inputsz):

    time_orign  = float(orign['total time'][0])
    time_noprv  = float(noprv['total time'][0])
    time_noerr  = float(noerr['total time'][0])
    time_error  = float(error['total time'][0])
    prsv_noe    = float(noerr['preserve time'][0])
    prsv_w_e    = float(error['preserve time'][0])
    reex_w_e    = float(error['reex time'][0])
    rstr_w_e    = float(error['restore time'][0])
    cdrt_noe    = float(noerr['CD overhead'][0])
    cdrt_w_e    = float(error['CD overhead'][0])
    cdrt_w_e_calc   = float(error['begin time'][0] \
                    + error['complete time'][0] \
                    + error['create time'][0] \
                    + error['destory time'][0] \
                    + error['sync time exec'][0] \
                    + error['sync time reex'][0] \
                    + error['sync time recr'][0])
    rollback_loss = time_error - time_noerr
    preserve_time = time_noerr - time_noprv
    cdrt_overhead = time_noprv - time_orign
    if rollback_loss < 0:
        rollback_loss = reex_w_e
    if preserve_time < 0:
        preserve_time = prsv_noe
    if cdrt_overhead < 0:
        cdrt_overhead = cdrt_noe
    print 'debug:', tasks, inputsz, ':', time_error, rollback_loss, preserve_time, cdrt_overhead
    mytuple = apps,inputsz,tasks
    tot_prof['bare'       ].loc[mytuple] = time_orign
    tot_prof['noprv'      ].loc[mytuple] = time_noprv 
    tot_prof['errfree'    ].loc[mytuple] = time_noerr
    tot_prof['total'      ].loc[mytuple] = time_error
    tot_prof['preserve'   ].loc[mytuple] = preserve_time
    tot_prof['preserve w/o error'].loc[mytuple] = prsv_noe
    tot_prof['preserve w error'  ].loc[mytuple] = prsv_w_e
    tot_prof['rollback'      ].loc[mytuple] = rollback_loss
    tot_prof['rollback calc' ].loc[mytuple] = reex_w_e
    tot_prof['restore'       ].loc[mytuple] = rstr_w_e
    tot_prof['runtime'       ].loc[mytuple] = cdrt_overhead
    tot_prof['runtime w/o CD'].loc[mytuple] = cdrt_noe
    tot_prof['runtime w/ CD' ].loc[mytuple] = cdrt_w_e
    tot_prof['runtime calc'  ].loc[mytuple] = cdrt_w_e_calc
    tot_prof['comm'          ].loc[mytuple] = orign[message_time][0]
    tot_prof['comm w/ CD'    ].loc[mytuple] = noerr[message_time][0]
    tot_prof['comm w/ error' ].loc[mytuple] = error[message_time][0]

tot_labels = [ 'bare'              
              ,'noprv'             
              ,'errfree'           
              ,'total'             
              ,'preserve'          
              ,'preserve w/o error'
              ,'preserve w error'  
              ,'rollback'      
              ,'rollback calc' 
              ,'restore'       
              ,'runtime'       
              ,'runtime w/o CD'
              ,'runtime w/ CD' 
              ,'runtime calc'  
              ,'comm'          
              ,'comm w/ CD'    
              ,'comm w/ error'] 
phase_labels = ['total time'
              , 'preserve'  
              , 'runtime'   
              , 'restore'   
              , 'rollback'  
              , 'vol in'    
              , 'vol out'   
              , 'bw'        
              , 'bw real'   
              , 'prsv loc'  
              , 'prsv max'  
              , 'rtov loc'  
              , 'rtov max'  
              , 'exec'      
              , 'reex']
def GetBreakdown(cdinfo, phase_infos, mytuple):
    for pid in phase_infos:
        print pid, phase_infos[pid]
        elem = phase_infos[pid]
        cdinfo[pid, 'total time'].loc[mytuple] = elem['total_time']['avg']
        cdinfo[pid, 'preserve'  ].loc[mytuple] = elem['total preserve']
        cdinfo[pid, 'runtime'   ].loc[mytuple] = elem['CDrt overhead']
        cdinfo[pid, 'restore'   ].loc[mytuple] = elem['restore']['avg']
        cdinfo[pid, 'rollback'  ].loc[mytuple] = elem['reex_time']['avg']
        cdinfo[pid, 'vol in'    ].loc[mytuple] = elem['input volume']
        cdinfo[pid, 'vol out'   ].loc[mytuple] = elem['output volume']
        cdinfo[pid, 'bw'        ].loc[mytuple] = elem['rd_bw']
        cdinfo[pid, 'bw real'   ].loc[mytuple] = elem['rd_bw_mea']
        cdinfo[pid, 'prsv loc'  ].loc[mytuple] = elem['loc prv time']
        cdinfo[pid, 'prsv max'  ].loc[mytuple] = elem['loc prv time']
        cdinfo[pid, 'rtov loc'  ].loc[mytuple] = 0 #elem['loc rtov time']
        cdinfo[pid, 'rtov max'  ].loc[mytuple] = 0 #elem['max rtov time']
        cdinfo[pid, 'exec'      ].loc[mytuple] = elem['exec']['avg']
        cdinfo[pid, 'reex'      ].loc[mytuple] = elem['reexec']['avg']

######################################################################
# Initialization
######################################################################

parser = ap.ArgumentParser(description="CD profiler to generate graph from post-processed results (merging)")
parser.add_argument('file', help='file to process', nargs='*')
parser.add_argument('-i', '--input-files', dest='input_files', help="total profile input files", nargs='*')
parser.add_argument('-t', '--trace-files', dest='trace_files', help="trace input files", nargs='*')
parser.add_argument('-c', '--cdinfo-files', dest='cdinfo_files', help="CD info input files", nargs='*')
args = parser.parse_args()
print 'in:', args.file
print 'trace files:', args.trace_files, type(args.trace_files)
print 'input files:', args.input_files
print 'cdinfo files:', args.cdinfo_files
# if not (len(args.file) == 0 or len(args.trace_files) == 0):
#     parser.error('Input file is not specified.')
#     raise SystemExit

raw_input('----------------')

if not (len(args.file) == 0 or len(args.trace_files) == 0):
    parser.error('Input file is not specified.')
    raise SystemExit

if (len(args.file) == 0) :
    input_files = args.input_files
else:
    input_files = args.file

if len(input_files) == 0:
    parser.error('Input file is not specified.')
    raise SystemExit

######################################################################
# Get python objects from pickle files
######################################################################

trace_list = []
for pkl in args.trace_files:
    prof = pd.read_pickle(pkl)
    trace_list.append(prof)
    processProf(prof)
    raw_input('\n--- trace')

cdinfo_list = []
for pkl in args.cdinfo_files:
    prof = pd.read_pickle(pkl)
    cdinfo_list.append(prof)
    processProf(prof)
    raw_input('\n--- cdinfo')

input_list = []
for pkl in input_files:
    prof = pd.read_pickle(pkl)
    input_list.append(prof)
    processProf(prof)
    raw_input('\n--- total profile')


######################################################################
# Get histogram for latency measurements
# Query a histogram for some combination 
# such as app lulesh, task 512, input 80, 
######################################################################
prof = trace_list[0]
appls = {}
ftypes= {}
tasks = {}
sizes = {}
phase_list = []
phase_dict = {}
for appl in prof:
    appls[appl] = 0
    for ftype in prof[appl]:
        ftypes[ftype] = 0
        for task in prof[appl][ftype]:
            tasks[task] = 0
            for size in prof[appl][ftype][task]:
                sizes[size] = 0
                print '\t\t', appl, ftype, task, size
                plist = []
                for pid in prof[appl][ftype][task][size]:
                    phase_dict[pid] = 0
#                    print pid, prof[appl][ftype][task][size][pid]
#                    plist.append(ProfInfo(prof[appl][ftype][task][size][pid]['exec_trace'], appl, ftype, task, size, pid))
#                raw_input('---')
#                GetLatencyHist(plist, False)
for phase in phase_dict:
    phase_list.append(phase)

tot_prof = GenEmptyDF(appls, sizes, tasks, [tot_labels])
print tot_prof
raw_input('tot prof begin')
result = input_list[0]
for app in result:
    ftype = result[app].keys()[0]
    for task in result[app][ftype]:
        for inputsz in result[app][ftype][task]:
            missing, orign, noprv, noerr, error = CheckMissing(result[app], task, inputsz)
            if missing:
                print 'missing', task, inputsz, orign, noprv, noerr, error
                raise SystemExit
            GetTotProf(tot_prof, 
                       result[app][orign][task][inputsz], 
                       result[app][noprv][task][inputsz], 
                       result[app][noerr][task][inputsz], 
                       result[app][error][task][inputsz], 
                       app, task, inputsz)

cdinfo = cdinfo_list[0]
phase_prof = GenEmptyDF(appls, sizes, tasks, [phase_list, phase_labels])
for app in cdinfo:
    ftype = 'Ftype1'
    for task in cdinfo[app][ftype]:
        for inputsz in cdinfo[app][ftype][task]:
            print cdinfo[app][ftype][task][inputsz]
            print('tot prof end', app, ftype, task, inputsz)
            GetBreakdown(phase_prof, cdinfo[app][ftype][task][inputsz], (app,task,inputsz))
            raw_input('tot prof end')


print tot_prof
tot_prof.to_csv("tot_profile.csv")
tot_prof.to_csv("tot_profile.pkl")
raw_input('tot prof end')



















