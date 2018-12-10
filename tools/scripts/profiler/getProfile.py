#!/usr/bin/python
from __future__ import print_function
import pandas as pd
import argparse as ap
import matplotlib.pylab as plt
import numpy as np
from itertools import groupby
#from cd_tools_msg import *
from gen_histogram import *
import sys

pd.options.mode.chained_assignment = None
def processProf(prof):
    for exec_name in prof:
        for fail_type in prof[exec_name]:
            for numTasks in prof[exec_name][fail_type]:
                for input_size in prof[exec_name][fail_type][numTasks]:
                    print('\t\t', exec_name, fail_type, numTasks, input_size)
                    for elem in prof[exec_name][fail_type][numTasks][input_size]:
                        print('----', elem, prof[exec_name][fail_type][numTasks][input_size][elem])
#                        for tr in prof[exec_name][fail_type][numTasks][input_size][elem]:
#                            print(elem, tr, prof[exec_name][fail_type][numTasks][input_size][elem][tr])


def check_Existance(prof, app, ftype, task, size, phase, trace):
    if app in prof.keys():
        if ftype in prof[app].keys():
            if task in prof[app][ftype].keys():
                if size in prof[app][ftype][task].keys():
                    if phase in prof[app][ftype][task][size].keys():
                        if trace in prof[app][ftype][task][size][phase].keys():
                            print('Existance check completed!')
                        else:
                            print(app+' '+ftype+' '+' '+str(task)+' '+size+' '+phase)
                            print(trace+' is not in '+str(prof[app][ftype][task][size][phase].keys()))
                            return False
                    else:
                        print(app+' '+ftype+' '+' '+str(task)+' '+size)
                        print(phase+' is not in '+str(prof[app][ftype][task][size].keys()))
                        return False
                else:
                    print(app+' '+ftype+' '+' '+str(task))
                    print(size+' is not in '+str(prof[app][ftype][task].keys()))
                    return False
            else: 
                print(app+' '+ftype)
                print(task+' is not in '+str(prof[app][ftype].keys()))
                return False
        else:
            print(app)
            print(ftype+' is not in '+str(prof[app].keys()))
            return False
    else :
        print(app+' is not in '+str(prof.keys()))
        return False
    return True


def add_line(ax, xpos, ypos):
    line = plt.Line2D([xpos, xpos], [ypos + .1, ypos],
                      transform=ax.transAxes, color='black')
    line.set_clip_on(False)
    ax.add_line(line)

def label_len(my_index,level):
    labels = my_index.get_level_values(level)
    return [(k, sum(1 for i in g)) for k,g in groupby(labels)]


def label_group_bar_table(ax, df):
    ypos = -.1
    scale = 4./(4*df.index.size+1)
    for level in range(df.index.nlevels)[::-1]:
        pos = 0.125
        for label, rpos in label_len(df.index,level):
            lxpos = (pos + .5 * rpos)*scale
            ax.text(lxpos, ypos, label, ha='center', transform=ax.transAxes)
            add_line(ax, pos*scale, ypos)
            pos += rpos
        add_line(ax, pos*scale , ypos)
        ypos -= .1



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
        else:
            print('\nmissing *')
        #print ftype        
    missing = (orign is '') or (noprv is '') or (noerr is '') or (error is '')
#    print 'check missing:', nTask, inputsize, orign, noprv, noerr, error
#    for ftype in ftype_list:
#        print ftype        
#    input('-ftype-')
    if missing == False:
        missing |= result[orign].get(nTask) == None
        missing |= result[noprv].get(nTask) == None
        missing |= result[noerr].get(nTask) == None
        print('nTask:', nTask)
        if missing == False:
            missing |= result[orign][nTask].get(inputsize) == None
            missing |= result[noprv][nTask].get(inputsize) == None
            missing |= result[noerr][nTask].get(inputsize) == None
        else:
            print('input:', inputsize)
    else:
        print('nTask:', nTask)
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
    print('debug:', tasks, inputsz, ':', time_error, rollback_loss, preserve_time, cdrt_overhead)
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
              , 'reex'
              , 'execution time'
              , 'CD Lv']

def GetBreakdown(cdinfo, phase_infos, noprv, mytuple):
    for pid in phase_infos:
        #print('\n',pid, phase_infos[pid])
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
        cdinfo[pid, 'prsv max'  ].loc[mytuple] = elem['loc prv time'] # there is no [max prv time] jeageun 
        cdinfo[pid, 'rtov loc'  ].loc[mytuple] = elem['loc cdrt time'] # is this right?
        cdinfo[pid, 'rtov max'  ].loc[mytuple] = noprv[pid]['max cdrt time']* elem['current counts']
#        cdinfo[pid, 'rtov loc'  ].loc[mytuple] = elem['loc rtov time']
#        cdinfo[pid, 'rtov max'  ].loc[mytuple] = elem['max rtov time']* elem['current count']
        cdinfo[pid, 'exec'      ].loc[mytuple] = elem['exec']['avg']
        cdinfo[pid, 'reex'      ].loc[mytuple] = elem['reexec']['avg']
        cdinfo[pid, 'execution time'].loc[mytuple] = elem['execution time'] *elem['current counts']
# added by geun
        cdinfo[pid, 'CD Lv'].loc[mytuple] = elem['type']
    return cdinfo

def GetParamFromUser(param_out, param_in, param_avail):
    for param in param_in:
        param_str = ''
        while param_str not in param_in:
            print('set %s ( ' % param, end='')
            for each in param_avail[param]:
                print('%s ' % each, end='')
            param_str = input(') : ')
            if param_str == 'all':
                print('is all')
                param_out[param] = param_avail[param]
                break
            else:
                # str() is necessary to convert to ascii
                user_list = [str(x.strip()) for x in param_str.split(',')]
                print('type check:', type(param_avail[param][0]))
                if type(param_avail[param][0]) is int:
                    user_list = [ int(x) if x.isdigit() else -1  for x in user_list ]
                print('user list:', user_list)
                if set(user_list).issubset(param_avail[param]):
                    print("it is a subset")
                    param_out[param] = user_list
                    break
                else:
                    print("it is not a subset")
                    param_str = ''

def GetHistogram(params, fixed, fixed_item):
    fixed.sort()
    print('Now Get Histogram!!\n')
    print('param:', params)
    print('sweep:', fixed)
    todo_list = ['trace']
    trace_avail = { 'trace' : ["exec_trace", "cdrt_trace", "prsv_trace", "max_prsv", "max_cdrt"] }
    trace_param = { }
    GetParamFromUser(trace_param, todo_list, trace_avail)
    print('trace:', trace_param['trace'])
    if len(params[fixed_item]) == 0:
        params[fixed_item].append(fixed[0])
    for app in params['app']:
        for ftype in params['type' ]:
            for task in  params['task' ]:
                for size in params['size' ]:
                    for phase in params['phase' ]:
                        for trace in trace_param['trace']:
                            print('\t\t', appl, ftype, task, size)
                            plist = []
                            if fixed_item == 'app':
                                fixed = prof.keys()
                            elif fixed_item == 'type':
                                fixed = prof[app].keys()
                            elif fixed_item == 'task':
                                fixed = prof[app][ftype].keys()
                            elif fixed_item == 'size':
                                fixed = prof[app][ftype][task].keys()
                            elif fixed_item == 'phase':
                                fixed = prof[app][ftype][task][size].keys()
                            fixed = list(fixed)
                            fixed.sort()


                            for swpr in fixed:
                                if fixed_item == 'app':
                                    if check_Existance(prof, swpr, ftype, task, size, phase, trace):
                                        target = prof[swpr][ftype][task][size][phase][trace]
                                        plist.append(ProfInfo(target, swpr, ftype, task, size, phase, fixed_item, trace))
                                        name = 'all_' + ftype + '_' + task + '_' + size + '_' + phase + '_' + trace
                                elif fixed_item == 'type':
                                    if check_Existance(prof, app, swpr, task, size, phase, trace):
                                        target = prof[app][swpr][task][size][phase][trace]
                                        plist.append(ProfInfo(target, app, swpr, task, size, phase, fixed_item, trace))
                                        name = app + '_all_' + task + '_' + size + '_' + phase + '_' + trace
                                elif fixed_item == 'task':
                                    if check_Existance(prof, app, ftype, swpr, size, phase, trace):
                                        target = prof[app][ftype][swpr][size][phase][trace]
                                        plist.append(ProfInfo(target, app, ftype, swpr, size, phase, fixed_item, trace))
                                        name = app + '_' + ftype + '_all_' + size + '_' + phase + '_' + trace
                                elif fixed_item == 'size':
                                    if check_Existance(prof, app, ftype, task, swpr, phase, trace):
                                        target = prof[app][ftype][task][swpr][phase][trace]
                                        plist.append(ProfInfo(target, app, ftype, task, swpr, phase, fixed_item, trace))
                                        name = app + '_' + ftype + '_all_' + size + phase + trace
                                elif fixed_item == 'phase':
                                    if check_Existance(prof, app, ftype, task, size, swpr, trace):
                                        target = prof[app][ftype][task][size][swpr][trace]
                                        plist.append(ProfInfo(target, app, ftype, task, size, swpr, fixed_item, trace))
                                        name = app + '_' + ftype + '_' + task + '_' + size + '_all_' + trace
                                else:
                                    assert False
                            GetLatencyHist(plist, False,
                                           '', # title
                                           'latency_hist_' + name)
                            
    print('done:', trace_param['trace'])

######################################################################
# Initialization
######################################################################

parser = ap.ArgumentParser(description="CD profiler to generate graph from post-processed results (merging)")
parser.add_argument('file', help='file to process', nargs='*')
parser.add_argument('-p', '--input-files', dest='input_files', help="total profile input files", nargs='*')
parser.add_argument('-r', '--cdinfo-files', dest='cdinfo_files', help="CD info input files", nargs='*')
parser.add_argument('-t', '--trace-files', dest='trace_files', help="trace input files", nargs='*')
parser.add_argument('-i', '--total-df', dest='totalinfo', help="Total profile data frame")
parser.add_argument('-c', '--cdinfo-df', dest='cdinfo', help="CD info data frame")
args = parser.parse_args()
print('in:', args.file)
print('trace files:', args.trace_files, type(args.trace_files))
print('input files:', args.input_files)
print('cdinfo files:', args.cdinfo_files)
print('input df:', args.totalinfo, type(args.totalinfo))
print('cdinfo df:', args.cdinfo, type(args.cdinfo))
# if not (len(args.file) == 0 or len(args.trace_files) == 0):
#     parser.error('Input file is not specified.')
#     raise SystemExit

input('----------------')

totalinfo_df = args.totalinfo
cdinfo_df =  args.cdinfo
tot_info_files = []
cdinfo_files = []
trace_files = []
if type(args.file) is list :
    tot_info_files = args.file

if type(args.input_files) is list :
    tot_info_files = args.input_files

if type(args.cdinfo_files) is list :
    cdinfo_files = args.cdinfo_files

if type(args.trace_files) is list :
    trace_files = args.trace_files

if len(tot_info_files) == 0 \
  and len(cdinfo_files) == 0 \
  and len(trace_files) == 0 \
  and totalinfo_df == None \
  and cdinfo_df == None :
    parser.error('Input file is not specified.!!\n')
    raise SystemExit

######################################################################
# Get python objects from pickle files
######################################################################
prof = None
input_list = []
for pkl in tot_info_files:
    print(pkl)
    elem = pd.read_pickle(pkl)
    input_list.append(elem)
    prof = elem
#    input('\n--- total profile')
#    processProf(elem)
#    input('\n--- total profile')

trace_list = []
for pkl in trace_files:
    print(pkl)
    elem = pd.read_pickle(pkl)
    trace_list.append(elem)
    prof = elem
    #processProf(elem)
    #input('\n--- trace')

cdinfo_list = []
for pkl in cdinfo_files:
    print(pkl)
    elem = pd.read_pickle(pkl)
    cdinfo_list.append(elem)
    prof = elem
    #processProf(elem)
    #input('\n--- cdinfo')

######################################################################
# Get histogram for latency measurements
# Query a histogram for some combination 
# such as app lulesh, task 512, input 80, 
######################################################################

#processProf(prof)
#input('\n--- prof total profile')

#########################################################
# Get placeholders for ROI data
#########################################################

apps_dict = {}
type_dict= {}
task_dict = {}
size_dict = {}
phase_dict = {}
if prof != None:
    for appl in prof:
        apps_dict[appl] = 0
        for ftype in prof[appl]:
            type_dict[ftype] = 0
            for task in prof[appl][ftype]:
                task_dict[task] = 0
                for size in prof[appl][ftype][task]:
                    size_dict[size] = 0
                    for pid in prof[appl][ftype][task][size]:
                        phase_dict[pid] = 0

# appls = ['lulesh', 'comd']
# ftypes = ['orign', 'noprv', 'noerr', 'error']
# tasks = [8, 64, 216, 512, 1000]
# sizes = [40, 60, 80]
# phases = ['root', 'parent', 'child']

appls = []
for app in apps_dict:
    appls.append(app)
ftypes = []
for e in type_dict:
    ftypes.append(e)
tasks = []
for e in task_dict:
    tasks.append(e)
sizes = []
for e in size_dict:
    sizes.append(e)
phases = []
for phase in phase_dict:
    phases.append(phase)

print('\napps:', end='')
for app in apps_dict:
    print('%s ' % app)
print('\ntype:', end='')
for ftype in type_dict:
    print('%s ' % ftype, end='')
print('\ntask:', end='')
for task in task_dict:
    print('%s ' % task, end='')
for size in size_dict:
    print('%s ' % size, end='')
print('\nphase: ', end='')
for phase in phase_dict:
    print('%s ' % phase, end='')

input('\n--------- check items ------------')
#########################################################
# Generate total performance profile data frame
#########################################################

input('tot prof begin\t')
if len(input_list) > 0:
    result = input_list[0]
    tot_prof = GenEmptyDF(appls, sizes, tasks, [tot_labels])
    for app in result:
        ftype = list(result[app].keys())[0] # in python3, it is not list.
        print(ftype)
        for task in result[app][ftype]:
            for inputsz in result[app][ftype][task]:
                missing, orign, noprv, noerr, error = CheckMissing(result[app], task, inputsz)
                if missing:
                    print('missing', task, inputsz, ftype,  orign, noprv, noerr, error)
                    raise SystemExit
                print('generating:', task, inputsz, ftype,  orign, noprv, noerr, error)
                raw_input('----------')
                GetTotProf(tot_prof, 
                           result[app][orign][task][inputsz], 
                           result[app][noprv][task][inputsz], 
                           result[app][noerr][task][inputsz], 
                           result[app][error][task][inputsz], 
                           app, task, inputsz)
    tot_prof.to_csv("tot_profile.csv")
    tot_prof.to_pickle("tot_profile.pkl")
    print(tot_prof)
    input('\ntot prof end')

#########################################################
# Generate per-level breakdown data frame
#########################################################

if len(cdinfo_list) > 0:
    cdinfo = cdinfo_list[0]
    phase_prof = GenEmptyDF(appls, sizes, tasks, [phases, phase_labels])
   #print(phase_prof)
   #input('phase prof begin')

    for app in cdinfo:
        ftype = list(result[app].keys())[0]
        #cdinfo.find('noprv')
        ishere = 0
        for a in cdinfo[app].keys():
            if a == 'noprv' :
                ishere = 1
        if ishere == 0:
            print('ERROR noprv is not in the value')

        for task in cdinfo[app][ftype]:
            for inputsz in cdinfo[app][ftype][task]:
                #print cdinfo[app][ftype][task][inputsz]
                #print('tot prof end', app, ftype, task, inputsz, cdinfo[app][ftype][task][inputsz])
                #input('\nphase prof --- ')
                phase_prof = GetBreakdown(phase_prof, cdinfo[app][ftype][task][inputsz], cdinfo[app]['noprv'][task][inputsz], (app,inputsz,task))
    
    
    phase_prof.to_csv("phase_profile.csv")
    phase_prof.to_pickle("phase_profile.pkl")

if len(trace_list):
    #########################################################
    # parameters available from input file (superset)
    #########################################################
    param_avail = {'app' : appls, 
                  'type' : ftypes,
                  'task' : tasks, 
                  'size' : sizes, 
                  'phase': phases, 
                  }
    
    #########################################################
    # parameters to generate graphs (subset)
    #########################################################
    param_dict = {'app' : [], 
                  'type' : [],
                  'task' : [], 
                  'size' : [], 
                  'phase': [],
                  }
    
    #########################################################
    # Global flag for control
    #########################################################
    param_list = ['app', 'type', 'task', 'size', 'phase']
    gen_list = ['histogram', 'scalability', 'breakdown']
    is_param_set = False
    need_param_set = True
    
    #########################################################
    # Interactive shell to generate graphs begins
    #########################################################
    while len(gen_list) > 0:
    
        #########################################################
        # Set what to perform
        #########################################################
        print('Select what to perform: ( ', end='')
        for gen in gen_list:
            print('%s ' % gen, end='')
        gen_user = input(') : ')
        if gen_user in gen_list:
            gen_list.remove(gen_user)
        else:
            print('%s is not a member of ( ' % gen_user, end='')
            for gen in gen_list:
                print('%s ' % gen, end='')
            print(') !!\n')
            continue
    
        #########################################################
        # Set param
        #########################################################
        if is_param_set:
            print('Do you want to keep params: ')
            for item in param_dict:
                print('%s : (', end='')
                for each in param_dict[item]:
                    print('%s ' % each, end='') 
                print(')')
            want_keep_it = input(': (yes/no)')
            if want_keep_it[0] == 'n':
                need_param_set = True
                # init
                for item in param_dict:
                    param_dict[item].clear() # del param_dict[item][:]
            else:
                need_param_set = False
        if need_param_set:
            sweep_param_str = ''
            while sweep_param_str not in param_list:
                sweep_param_str = input('sweep: (app type task size phase) ')
            param_list.remove(sweep_param_str)
            print(param_list)
            GetParamFromUser(param_dict, param_list, param_avail)
        else: # bypass param
            print("Reuse params")
        print('Parameters', param_dict, ', sweep:', sweep_param_str)
        for params in param_dict: 
            print('%s: ( ' % params, end='')
            for param in param_dict[params]:
                print('%s ' % param, end='')
            print(' )\n')
        is_param_set = True
    
        if gen_user == 'histogram':
            print('Get histogram\n')
            GetHistogram(param_dict, param_avail[sweep_param_str], sweep_param_str)
        elif gen_user == 'scalability':
            print('Get scalability\n')
            #tot_prof[]
        elif gen_user == 'breakdown':
            print('Get breakdown\n')
            #phase_prof[]
        else:
            print('Unsupported thing to do (%s)\n' % gen_user)
        
        # Reinit param list
        param_list = ['app', 'type', 'task', 'size', 'phase']




if totalinfo_df:
    df = pd.read_pickle(totalinfo_df)
#    df.index.set_names(['app', 'size', 'task'], inplace=True).unstack('app')
#    print(df)
    input('asdfsadf')
    #df = pd.DataFrame.from_dict(mydict)
    roi = ['preserve', 'rollback', 'runtime', 'bare', 'noprv', 'errfree', 'total']
    filename_pre = 'total_info_'
    for app, new_df in df.groupby(level=0):
        print(new_df)
        tg = new_df.loc[app]
        tg.index.set_names(['size', 'task'], inplace=True)
#        tg2 = tg.copy()
#        #tg2.rename(index={'prsvHeavy':'hierarchy'}, level='size', inplace=True)        
#        print(app, '\n\n')
#        print(tg)
#        print('\n------ *** app *** -----------\n')
#        print(tg2)
#        print('\n------ *** app 2 *** -----------\n')
#        tg.append(tg2)
#        print(tg2)
#        print('\n------ *** after merge *** -----------\n')
#        raw_input('\n------ *** app *** -----------\n')
#        frames = [tg, tg2]
#        tg = pd.concat(frames)
        ax = tg.plot.bar(y=['preserve', 'rollback', 'runtime', 'bare'], stacked=True, 
#  =======
#          tg2 = tg.copy()
#          tg2.rename(index={'prsvHeavy':'hierarchy'}, level='size', inplace=True)        
#          
#  #        tg3 = tg.copy()
#  #        tg3.rename(index={'prsvHeavy':'jeageun'}, level='size', inplace=True)        
#          
#  #        tg4 = tg.copy()
#  #        tg4.rename(index={'prsvHeavy':'kyushick'}, level='size', inplace=True)        
#  
#  #        tg5 = tg.copy()
#  #        tg5.rename(index={'prsvHeavy':'plus'}, level='size', inplace=True)        
#  
#          print(app, '\n\n')
#          print(tg)
#          print('\n------ *** app *** -----------\n')
#          print(tg2)
#          print('\n------ *** app 2 *** -----------\n')
#         # tg.append(tg2)
#         # print(tg2)
#         # print('\n------ *** after merge *** -----------\n')
#         # input('\n------ *** app *** -----------\n')
#          frames = [tg]
#          tg = pd.concat(frames)
#          ax = tg.plot(kind = 'bar', y=['bare','runtime','preserve', 'rollback'], stacked=True, 
#  >>>>>>> a1b4dd9c5c62c2b2902d65a011c3051ffaa49992
                    #bottom = margin_bottom, color=colors[num], label=month
                    label=['original','runtime','preserve', 'rollback'], width =0.75
                    )
#        ax = tg.plot(kind = 'bar', y=['preserve', 'rollback', 'runtime', 'bare'], stacked=True, 
#                    #bottom = margin_bottom, color=colors[num], label=month
#                    label=['preserve', 'rollback', 'runtime', 'original'], width =0.75
#                    )
        
#        patches, labels = ax.get_legend_handles_labels()
#        ax.legend(patches, labels, ncol=len(df.index),
#            fontsize='x-large',
#            labelspacing=0.75, handlelength=1, columnspacing=0.75,
#            handletextpad=0.25, loc=3,
#            bbox_to_anchor=(0.55, 0.95),
#            #loc=2,
#            borderaxespad=0.)
        plt.sca(ax)
        plt.gca().yaxis.grid(True, ls='dotted')
#        ax.set_xticklabels(tg.index, fontsize='large')
        ax.set_ylabel('Execution Time', fontsize='large', labelpad=-1)
        ax.set_xlabel('Scalability', fontsize='large')
        #fig2 = plt.figure(figsize=(10,5))
        fig = ax.get_figure()
        #plt.show()
        filename = filename_pre + app  


#added by jeageun -------------------------------------------
        labels = ['' for item in ax.get_xticklabels()]
        ax.set_xticklabels(labels)
        ax.set_xlabel('')
        label_group_bar_table(ax, tg)
        fig.subplots_adjust(bottom=.1*df.index.nlevels)
        jtmp = 0
        ax2 = ax.twinx()

        for jlabel, jidx in label_len(tg.index,0): 
            tmpx = np.arange(jtmp,jtmp+jidx)
            jtmp += jidx
            #ax2.plot(tmpx, [4,2],color='magenta')


## -------------------------------------------added by jeageun


        fig.savefig(filename + '.pdf', format='pdf', bbox_inches='tight')
        fig.savefig(filename + '.svg', format='svg', bbox_inches='tight')
#        for size, task_df in tg.groupby(level=0):
#            print(size, '\n\n')
#            df2 = task_df[roi].loc[size]
#            #df2.index.name = 'task'
#            print(df2)
#            input('\n------ *** size *** -----------\n')
#            print(df2.unstack('task'))
#            input('\n------ *** unstack *** -----------\n')
#            ax = df2.plot.bar(y=['preserve', 'rollback', 'runtime', 'bare'], stacked=True, 
#                        #bottom = margin_bottom, color=colors[num], label=month
#                        label=['preserve', 'rollback', 'runtime', 'original']
#                        )
#           
##            patches, labels = ax.get_legend_handles_labels()
##            ax.legend(patches, labels, ncol=len(df.index),
##                fontsize='x-large',
##                labelspacing=0.75, handlelength=1, columnspacing=0.75,
##                handletextpad=0.25, loc=3,
##                bbox_to_anchor=(0.55, 0.95),
##                #loc=2,
##                borderaxespad=0.)
#            plt.sca(ax)
#            plt.gca().yaxis.grid(True, ls='dotted')
#            ax.set_xticklabels(df2.index, fontsize='large')
#            ax.set_ylabel('Execution Time', fontsize='large', labelpad=-1)
#            ax.set_xlabel('Scalability', fontsize='large')
#            #fig2 = plt.figure(figsize=(10,5))
#            fig = ax.get_figure()
#            #plt.show()
#            filename = filename_pre + app + '_' + size 
#            fig.savefig(filename + '.pdf', format='pdf', bbox_inches='tight')
#            fig.savefig(filename + '.svg', format='svg', bbox_inches='tight')

 
        input('\n------ *** app *** -----------\n')

#    mytuple = apps,inputsz,tasks
#    df['preserve'].loc[mytuple] = preserve_time
#    df['rollback'].loc[mytuple] = rollback_loss
#    df['runtime' ].loc[mytuple] = cdrt_overhead
#    df['original'].loc[mytuple] = time_orign
#    tot_prof['noprv'      ].loc[mytuple] = time_noprv 
#    tot_prof['errfree'    ].loc[mytuple] = time_noerr
#    tot_prof['total'      ].loc[mytuple] = time_error

    input('\ntotal info from data frame\n')
#            sweep_param_str = ''
#            while sweep_param_str not in param_list:
#                sweep_param_str = input('sweep: (app type task size phase) ')
#            param_list.remove(sweep_param_str)
#            print(param_list)
#            GetParamFromUser(param_dict, param_list, param_avail)
    



if cdinfo_df:
    df = pd.read_pickle(cdinfo_df)
    df2 = pd.DataFrame.from_dict(df)
    print(df)
    print(df2)
    cdarr = df.stack(level=1).columns.values
    cdarr = sorted(cdarr, key=lambda x: int(x[3:5].replace('_','').replace('t','').replace(' ','0')))

    pickup_array = ['rollback','preserve','execution time','rtov max', 'CD Lv']

    tmpdf = pd.DataFrame(np.random.rand(len(cdarr), len(pickup_array)),columns=pickup_array,index=cdarr)
    for idx1 in cdarr: 
        for idx2 in pickup_array: 
            tmpdf[idx2][idx1] = df[idx1][idx2][0] #only use first task
    for idx1 in cdarr:
        if tmpdf['CD Lv'][idx1] != 'leaf':
            tmpdf['execution time'][idx1] = max(0,tmpdf['execution time'][idx1]-tmpdf['rollback'][idx1]-tmpdf['preserve'][idx1]-tmpdf['rtov max'][idx1])


    pickup_array.remove('CD Lv')
    sum=np.zeros(len(cdarr))
    
    for idx1 in cdarr:
        i = int(idx1[3:5].replace('_','').replace('t','').replace(' ','0'))
        for idx2 in pickup_array:
            sum[i] = tmpdf[idx2][idx1] + sum[i]
        if tmpdf['CD Lv'][idx1] == 'leaf':
            sum[i] = sum[i] - tmpdf['rtov max'][idx1]
        
    
    
    prev = 0
    for k in range(len(sum)):
        plt.bar([1],sum[k],bottom = prev) 
        prev = prev + sum[k]

    plt.bar([i+2 for i in range(len(tmpdf))], tmpdf[0:len(tmpdf)]['rollback'], bottom= 0)
    a = tmpdf[0:len(tmpdf)]['rollback']
    plt.bar([i+2 for i in range(len(tmpdf))], tmpdf[0:len(tmpdf)]['preserve'], bottom= a )
    a = a + tmpdf[0:len(tmpdf)]['preserve']
    plt.bar([i+2 for i in range(len(tmpdf))], tmpdf[0:len(tmpdf)]['execution time'], bottom= a )
    a = a + tmpdf[0:len(tmpdf)]['execution time']
    plt.bar([i+2 for i in range(len(tmpdf))], tmpdf[0:len(tmpdf)]['rtov max'], bottom= a )
    a = a + tmpdf[0:len(tmpdf)]['rtov max']
    plt.show()



#        patches, labels = ax.get_legend_handles_labels()
#        ax.legend(patches, labels, ncol=len(df.index),
#            fontsize='x-large',
#            labelspacing=0.75, handlelength=1, columnspacing=0.75,
#            handletextpad=0.25, loc=3,
#            bbox_to_anchor=(0.55, 0.95),
#            #loc=2,
#            borderaxespad=0.)


#added by jeageun -------------------------------------------
#    labels = ['' for item in ax.get_xticklabels()]
#    ax.set_xticklabels(labels)
#    ax.set_xlabel('')
#    label_group_bar_table(ax, tg)





    input('\ncd info from data frame\n')
#    tot_prof['preserve'   ].loc[mytuple] = preserve_time
#    tot_prof['rollback'   ].loc[mytuple] = rollback_loss
#    tot_prof['runtime'    ].loc[mytuple] = cdrt_overhead
#    tot_prof['bare'   ].loc[mytuple] = time_orign
