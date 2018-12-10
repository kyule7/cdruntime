#!/usr/bin/python
from mylibs import *
import numpy as np
import matplotlib.pyplot as plt
import csv
import copy
import pickle
import os

result = gatherJSONObj('file_list.txt')
#checkJSONObj(result)
#raw_input('Got results\n')
#df = pd.DataFrame.from_dict(result, orient='index')
#
# ###############################################################################
# # Generate empty dataframe
# ###############################################################################
# apps = {}
# ftypes = {}
# nTasks = {}
# inputsizes = {}
# phase_dict = {}
# for app in result:
#     apps[app] = 0
#     for ftype in result[app]:
#         ftypes[ftype] = 0
#         for nTask in result[app][ftype]:
#             nTasks[nTask] = 0
#             for inputsize in result[app][ftype][nTask]:
#                 inputsizes[inputsize] = 0
#                 if len(phase_dict) == 0:
#                     for phase in result[app][ftype][nTask][inputsize]['CD info']:
#                         print phase
#                         phase_dict[phase] = 0
# phase_list = []
# for phase in phase_dict:
#     phase_list.append(phase)
# 
# app_dict = {}
# app_list = []
# ftype_list = []
# nTask_list = []
# inputsize_list = []
# #print apps
# for app in apps:
#     #print app
#     splitted = re.split(r'_', app)
#     app_type = splitted[1]
#     app_name = splitted[0]
#     app_dict[app_name] = app_type
# #    print app_name, app_type
# #    raw_input('asdf')
# ##    if app_type == 'errfree' or app_type == 'bare' or app_type == 'noprv':
# ##        continue
# ##    else:
# #
# #    print app_list
# #    if app_list.find
# for app in app_dict:
#     app_list.append(app)
# 
# for ftype in ftypes:
#     #print ftype
#     ftype_list.append(ftype)
# for nTask in nTasks:
#     #print nTask
#     nTask_list.append(str(nTask))
# for inputsize in inputsizes:
#     #print inputsize
#     inputsize_list.append(inputsize)
# 
# print phase_list
# dfmi = genEmptyDF(app_list, inputsize_list, nTask_list, ftype_list)
# dfmi2 = genEmptyDF2(app_list, inputsize_list, nTask_list, ftype_list, phase_list)
# 
# print dfmi
# print dfmi2
# raw_input('33333333333333333333')
###############################################################################

#print dfmi['bare'].loc['lulesh','40','216']
#for app in result:
#    for ftype in result[app]:
#        for nTask in result[app][ftype]:
#            for inputsize in result[app][ftype][nTask]:
#                print dfmi['bare'].loc[app][inputsize][nTask]
#                raw_input('3333333333333333333333333333333333')

###############################################################################
# Average and merge estimations for the same measurements (same est.json files)
###############################################################################
merged = { 'CD info' : {}, 'trace' : {}, 'total profile' : {} }
trace_dict = {}
for app in result:
    merged['CD info'][app] = {}
    merged['trace'][app] = {}
    merged['total profile'][app] = {}
    for ftype in result[app]:
        merged['CD info'][app][ftype] = {}
        merged['trace'][app][ftype] = {}
        merged['total profile'][app][ftype] = {}
        for nTask in result[app][ftype]:
            merged['CD info'][app][ftype][nTask] = {}
            merged['trace'][app][ftype][nTask] = {}
            merged['total profile'][app][ftype][nTask] = {}
            for inputsize in result[app][ftype][nTask]:
                #print "\n\n\n Final %s %s %s, merging:%d %s ~~~~~~~~~~~~~" % \
                #        (app, nTask, inputsize, 
                #        len(result[app][ftype][nTask][inputsize]['CD info']), 
                #        type(result[app][ftype][nTask][inputsize]))
                merged['CD info'][app][ftype][nTask][inputsize], merged['trace'][app][ftype][nTask][inputsize], merged['total profile'][app][ftype][nTask][inputsize],= mergeProfiles(result[app][ftype][nTask][inputsize])
                for pid in merged['trace'][app][ftype][nTask][inputsize]:
                    trace_dict[(app,ftype,nTask,inputsize,pid)] = merged['trace'][app][ftype][nTask][inputsize][pid]
                #raw_input('averageCDTree')
                #print "\n\n\n Final %s %s %s~~~~~~~~~~~~~" % (app, nTask, inputsize)
#                printResults(avg_result['CD info'])

#raw_input('-------')
if os.path.isfile('cdinfo.pickle'):
    filetmp = pd.read_pickle('cdinfo.pickle')
    for app in filetmp.keys():
        for ftype in filetmp[app].keys():  
            for nTask in filetmp[app][ftype].keys():
                for inputsize in filetmp[app][ftype][nTask].keys():
                    for phase in filetmp[app][ftype][nTask][inputsize].keys():
                        tnum = filetmp[app][ftype][nTask][inputsize][phase]['num_sample']
                        if app not in merged['CD info'].keys():
                            merged['CD info'][app] = filetmp[app]
                        elif ftype not in merged['CD info'][app].keys():
                            merged['CD info'][app][ftype] = filetmp[app][ftype]
                        elif nTask not in merged['CD info'][app][ftype].keys():
                            merged['CD info'][app][ftype][nTask] = filetmp[app][ftype][nTask]
                        elif inputsize not in merged['CD info'][app][ftype][nTask].keys():
                            merged['CD info'][app][ftype][nTask][inputsize] = filetmp[app][ftype][nTask][inputsize]
                        elif phase not in merged['CD info'][app][ftype][nTask][inputsize].keys():
                            merged['CD info'][app][ftype][nTask][inputsize][phase] = filetmp[app][ftype][nTask][inputsize][phase]
                        else :
                            mnum=merged['CD info'][app][ftype][nTask][inputsize][phase]['num_sample']
                            for ls in filetmp[app][ftype][nTask][inputsize][phase].keys():
                                elem = filetmp[app][ftype][nTask][inputsize][phase][ls]
                                elemtype = type(elem)
                                if elemtype is dict:
                                    for e in elem:
                                         merged['CD info'][app][ftype][nTask][inputsize][phase][ls][e] = (mnum*merged['CD info'][app][ftype][nTask][inputsize][phase][ls][e]+ tnum *filetmp[app][ftype][nTask][inputsize][phase][ls][e]) / (tnum+mnum)
                                elif elemtype is float:
                                    merged['CD info'][app][ftype][nTask][inputsize][phase][ls]= (mnum*merged['CD info'][app][ftype][nTask][inputsize][phase][ls]+ tnum *filetmp[app][ftype][nTask][inputsize][phase][ls]) / (tnum+mnum)
                                elif elemtype is int :
                                    merged['CD info'][app][ftype][nTask][inputsize][phase][ls]= (mnum*merged['CD info'][app][ftype][nTask][inputsize][phase][ls]+ tnum *filetmp[app][ftype][nTask][inputsize][phase][ls]) / (tnum+mnum)
                            merged['CD info'][app][ftype][nTask][inputsize][phase]['num_sample'] = tnum+mnum

if os.path.isfile('tot_prof.pickle'):
    filetmp = pd.read_pickle('tot_prof.pickle')
    for app in filetmp.keys():
        if app not in merged['total profile'].keys():
             merged['total profile'][app] = filetmp[app]
        else:
            for ftype in filetmp[app].keys():  
                if ftype not in merged['total profile'][app].keys():
                    merged['total profile'][app][ftype] = filetmp[app][ftype]
                else :
                    for nTask in filetmp[app][ftype].keys():
                        if nTask not in merged['total profile'][app][ftype].keys():
                            merged['total profile'][app][ftype][nTask]= filetmp[app][ftype][nTask]
                    else :
                        for inputsize in filetmp[app][ftype][nTask].keys():
                            if inputsize not in merged['total profile'][app][ftype][nTask].keys():
                                merged['total profile'][app][ftype][nTask][inputsize]= filetmp[app][ftype][nTask][inputsize]
                            else :
                                tnum = filetmp[app][ftype][nTask][inputsize]['num_sample']
                                mnum=merged['total profile'][app][ftype][nTask][inputsize]['num_sample']
                                for phase in filetmp[app][ftype][nTask][inputsize].keys():
                                    elem = filetmp[app][ftype][nTask][inputsize][phase]
                                    elemtype = type(elem)
                                    if elemtype is dict:
                                        for e in elem:
                                            merged['total profile'][app][ftype][nTask][inputsize][phase][e] = (mnum*merged['total profile'][app][ftype][nTask][inputsize][phase][e]+ tnum *filetmp[app][ftype][nTask][inputsize][phase][e]) / (tnum+mnum)
                                    elif elemtype is float:
                                        merged['total profile'][app][ftype][nTask][inputsize][phase]= (mnum*merged['total profile'][app][ftype][nTask][inputsize][phase]+ tnum *filetmp[app][ftype][nTask][inputsize][phase]) / (tnum+mnum)
                                    elif elemtype is int :
                                        merged['total profile'][app][ftype][nTask][inputsize][phase]= (mnum*merged['total profile'][app][ftype][nTask][inputsize][phase]+ tnum *filetmp[app][ftype][nTask][inputsize][phase]) / (tnum+mnum)
                                    elif elemtype is list:
                                        for e in range(4):
                                            merged['total profile'][app][ftype][nTask][inputsize][phase][e] = (mnum*merged['total profile'][app][ftype][nTask][inputsize][phase][e]+ tnum *filetmp[app][ftype][nTask][inputsize][phase][e]) / (tnum+mnum)


                                merged['total profile'][app][ftype][nTask][inputsize]['num_sample'] = tnum+mnum


if os.path.isfile('trace.pickle') :
    filetmp = pd.read_pickle('trace.pickle')
    for app in filetmp.keys():
        if app not in merged['trace'].keys():
            merged['trace'][app] = filetmp[app]
        else:
            for ftype in filetmp[app].keys():
                if ftype not in merged['trace'][app].keys():
                    merged['trace'][app][ftype] = filetmp[app][ftype]
                else:    
                    for nTask in filetmp[app][ftype].keys():
                        if nTask not in merged['trace'][app][ftype].keys():
                            merged['trace'][app][ftype][nTask] = filetmp[app][ftype][nTask]
                        else:
                            for inputsize in filetmp[app][ftype][nTask].keys():
                                if inputsize not in merged['trace'][app][ftype][nTask].keys():
                                    merged['trace'][app][ftype][nTask][inputsize] = filetmp[app][ftype][nTask][inputsize]
                                else:
                                    for phase in filetmp[app][ftype][nTask][inputsize].keys():
                                        if phase not in merged['trace'][app][ftype][nTask][inputsize].keys():
                                             merged['trace'][app][ftype][nTask][inputsize][phase] = filetmp[app][ftype][nTask][inputsize][phase]
                                        else:
                                            for ls in filetmp[app][ftype][nTask][inputsize][phase].keys():
                                                merged['trace'][app][ftype][nTask][inputsize][phase][ls].extend(filetmp[app][ftype][nTask][inputsize][phase][ls])

    for app in result:                                                            
        for ftype in result[app]:                                                 
            for nTask in result[app][ftype]:                                      
                for inputsize in result[app][ftype][nTask]:                       
                    for pid in merged['trace'][app][ftype][nTask][inputsize]:
                        trace_dict[(app,ftype,nTask,inputsize,pid)] = merged['trace'][app][ftype][nTask][inputsize][pid]

with open('cdinfo.pickle', 'wb') as handle:
    pickle.dump(merged['CD info'], handle, protocol=pickle.HIGHEST_PROTOCOL)
with open('trace.pickle', 'wb') as handle:
    pickle.dump(merged['trace'], handle, protocol=pickle.HIGHEST_PROTOCOL)
with open('trace_dict.pickle', 'wb') as handle:
    pickle.dump(trace_dict, handle, protocol=pickle.HIGHEST_PROTOCOL)
with open('tot_prof.pickle', 'wb') as handle:
    pickle.dump(merged['total profile'], handle, protocol=pickle.HIGHEST_PROTOCOL)

#                   # Now result[app][ftype][nTask][inputsize] is dict of phaseID:cd_info
#                   #print dfmi['bare'].loc['lulesh_fgcd','80','1000']
#                   #print dfmi['errfree'].loc['lulesh_fgcd','60','512'] 
#                   
#                   ###############################################################################
#                   # Get per estimation info in list (in order to print per-level info in csv format)
#                   ###############################################################################
#                   per_est_info = getEstInfo(result)
#                   #for per_est in per_est_info:
#                       #print per_est
#                       #raw_input("\n******************************\n")
#                   #
#                   #print cd_info
#                   #for lv0 in cd_info:
#                   #    print lv0
#                   #
#                   #    raw_input("222222222222check2322222222")
#                   #    print cd_info[lv0]['child CDs']
#                   #    for lv1 in cd_info[lv0]["child CDs"]:
#                   #        print "  ", lv1
#                   #        for lv2 in cd_info[lv0]["child CDs"][lv1]["child CDs"]:
#                   #            print "    ", lv2
#                   #raw_input("222222222222check")
#                   #print per_est_info
#                   #raw_input("222222222222check")
#                   
#                   writeCSV(per_est_info, "gathered.csv")
#                   
#                   for app in result:
#                      for ftype in result[app]:
#                          for nTask in result[app][ftype]:
#                              for inputsize in result[app][ftype][nTask]:
#                                   print app, inputsize, nTask
#                                   print 'check first'
#                   
#                   for app in result:
#                       app_type = re.split(r'_', app)[1]
#                       app_name = re.split(r'_', app)[0]
#                       if app_type == 'errfree' or app_type == 'bare' or app_type == 'noprv':
#                           continue
#                       else:
#                           for ftype in result[app]:
#                               for nTask in result[app][ftype]:
#                                   for inputsize in result[app][ftype][nTask]:
#                   #                    print (app_name + '_errfree'), ftype, nTask, inputsize
#                   #                    print result[app_name + '_errfree'][ftype][nTask][inputsize]
#                   #                    print app, inputsize, nTask
#                   #                    print dfmi
#                   #                    print app_name + '_noprv'
#                   #                    raw_input('333333333333333333333')
#                   #                    for key in result[app_name + '_noprv'][ftype][nTask][inputsize]:
#                   #                        print key
#                                       cd_elem = result[app][ftype][nTask][inputsize]
#                                       app_bare = app_name + '_bare'   
#                                       app_nopv = app_name + '_noprv'  
#                                       app_noer = app_name + '_errfree'
#                   
#                                       missing = checkMissing(result, app_bare, app_nopv, app_noer, ftype, nTask, inputsize)
#                                       if missing:
#                                           continue
#                   
#                                       time_bare     = result[app_bare][ftype][nTask][inputsize]['total time']
#                                       comm_bare     = result[app_bare][ftype][nTask][inputsize][message_time]
#                                       time_noprv    = result[app_nopv][ftype][nTask][inputsize]['total time']
#                                       time_errfree  = result[app_noer][ftype][nTask][inputsize]['total time']
#                                       prsv_noe      = result[app_noer][ftype][nTask][inputsize]['preserve time']
#                                       cdrt_noe      = result[app_noer][ftype][nTask][inputsize]['CD overhead']
#                                       comm_noe      = result[app_noer][ftype][nTask][inputsize][message_time]
#                                       time_w_e      = cd_elem['total time']
#                                       reex_w_e      = cd_elem['reex time']
#                                       prsv_w_e      = cd_elem['preserve time']
#                                       rstr_w_e      = cd_elem['restore time']
#                                       cdrt_w_e      = cd_elem['CD overhead']
#                                       reex_t        = cd_elem['reex time']
#                                       prsv_t_w_e    = cd_elem['preserve time']
#                                       prsv_t_noe    = cd_elem['preserve time']
#                                       cd_overhead_w_e = cd_elem['CD overhead']
#                                       dfmi['bare'     ].loc[app_name,str(inputsize),str(nTask)]   = time_bare
#                                       dfmi['noprv'    ].loc[app_name,str(inputsize),str(nTask)]   = time_noprv 
#                                       dfmi['errfree'  ].loc[app_name,str(inputsize),str(nTask)]   = time_errfree
#                                       dfmi['preserve'].loc[app_name,str(inputsize),str(nTask)]    = (prsv_noe+prsv_w_e) / 2
#                                       dfmi['total'].loc[app_name,str(inputsize),str(nTask)]       = time_w_e
#                                       dfmi['reex time'  ].loc[app_name,str(inputsize),str(nTask)] = reex_w_e
#                                       dfmi['restore'    ].loc[app_name,str(inputsize),str(nTask)] = rstr_w_e
#                                       dfmi['comm time'  ].loc[app_name,str(inputsize),str(nTask)] = comm_bare
#                                       dfmi['presv w/ e' ].loc[app_name,str(inputsize),str(nTask)] = prsv_w_e
#                                       dfmi['presv w/o e'].loc[app_name,str(inputsize),str(nTask)] = prsv_noe
#                                       dfmi['CD overhead'].loc[app_name,str(inputsize),str(nTask)] = cdrt_noe
#                                       dfmi['comm w/ CD' ].loc[app_name,str(inputsize),str(nTask)] = comm_noe
#                                       total    = result[app][ftype][nTask][inputsize]['total time']
#                                       runtime_overhead = result[app][ftype][nTask][inputsize]['begin time'] \
#                                                        + result[app][ftype][nTask][inputsize]['complete time'] \
#                                                        + result[app][ftype][nTask][inputsize]['create time'] \
#                                                        + result[app][ftype][nTask][inputsize]['destory time'] \
#                                                        + result[app][ftype][nTask][inputsize]['sync time exec']
#                   #                                    + result[app][ftype][nTask][inputsize]['sync time reex']
#                   #                                    + result[app][ftype][nTask][inputsize]['sync time recr']
#                                       rt_overhead = time_noprv - time_bare
#                                       if rt_overhead > 0:
#                                           dfmi['runtime'].loc[app_name,str(inputsize),str(nTask)] = rt_overhead
#                                       else:
#                                           dfmi['runtime'].loc[app_name,str(inputsize),str(nTask)] = cdrt_noe 
#                                       dfmi['rollback'].loc[app_name,str(inputsize),str(nTask)] = time_w_e - time_errfree
#                                       for phase in cd_elem['CD info']:
#                                           elem = cd_elem['CD info'][phase]
#                       #labels = ['total time','preserve', 'runtime', 'rollback', 'vol', 'bw', 'prv loc', 'rt loc', 'rt max', 'exec', 'reex']
#                                           dfmi2[phase, 'total time'].loc[app_name,str(inputsize),str(nTask)] = cd_elem['CD info'][phase]['total_time']['avg']
#                                           dfmi2[phase, 'preserve'  ].loc[app_name,str(inputsize),str(nTask)] = cd_elem['CD info'][phase]['total preserve']
#                                           dfmi2[phase, 'runtime'   ].loc[app_name,str(inputsize),str(nTask)] = cd_elem['CD info'][phase]['CDrt overhead']
#                                           dfmi2[phase, 'restore'   ].loc[app_name,str(inputsize),str(nTask)] = cd_elem['CD info'][phase]['restore']['avg']
#                                           dfmi2[phase, 'rollback'  ].loc[app_name,str(inputsize),str(nTask)] = cd_elem['CD info'][phase]['reex_time']['avg']
#                                           dfmi2[phase, 'vol in'    ].loc[app_name,str(inputsize),str(nTask)] = cd_elem['CD info'][phase]['input volume']
#                                           dfmi2[phase, 'vol out'   ].loc[app_name,str(inputsize),str(nTask)] = cd_elem['CD info'][phase]['output volume']
#                                           dfmi2[phase, 'bw'        ].loc[app_name,str(inputsize),str(nTask)] = cd_elem['CD info'][phase]['rd_bw']
#                                           dfmi2[phase, 'bw real'   ].loc[app_name,str(inputsize),str(nTask)] = cd_elem['CD info'][phase]['rd_bw_mea']
#                                           dfmi2[phase, 'prsv loc'  ].loc[app_name,str(inputsize),str(nTask)] = cd_elem['CD info'][phase]['loc prv time']
#                                           dfmi2[phase, 'prsv max'  ].loc[app_name,str(inputsize),str(nTask)] = cd_elem['CD info'][phase]['loc prv time']
#                                           dfmi2[phase, 'rtov loc'  ].loc[app_name,str(inputsize),str(nTask)] = 0#cd_elem['CD info'][phase]['loc rtov time']
#                                           dfmi2[phase, 'rtov max'  ].loc[app_name,str(inputsize),str(nTask)] = 0#cd_elem['CD info'][phase]['max rtov time']
#                                           dfmi2[phase, 'exec'      ].loc[app_name,str(inputsize),str(nTask)] = cd_elem['CD info'][phase]['exec']['avg']
#                                           dfmi2[phase, 'reex'      ].loc[app_name,str(inputsize),str(nTask)] = cd_elem['CD info'][phase]['reexec']['avg']
#                                          #prof_list.extend([ elem["preserve time"], elem["CDrt overhead"], elem["reex_time"]["avg"], elem["rst_time"]["avg"] ])
#                   
#                   print dfmi
#                   #raw_input('~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~')
#                   print dfmi2
#                   dfmi.to_csv("result_lulesh.csv")
#                   dfmi.to_pickle("result_lulesh.pkl")
#                   dfmi2.to_csv("breakdown.csv")
#                   dfmi2.to_pickle("breakdown.pkl")
#                   
#                   
#                   #dfmi.plot(x='A', y='B').plot(kind='bar')
#                   ##raw_input('~~~~~~~~~~~~~~~0~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~')
#                   #df2 = dfmi2.groupby(level=0).count()
#                   #print df2
#                   #raw_input('~~~~~~~~~~~~~~~1~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~')
#                   #df2 = dfmi2.groupby(level=1).count()
#                   #print df2
#                   #
#                   #raw_input('~~~~~~~~~~~~~~~2~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~')
#                   #df2 = dfmi2.groupby(level=2).count()
#                   #print df2
#                   #raw_input('~~~~~~~~~~~~~~~3~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~')
#                   #
#                   ##df3 =dfmi.groupby(['1000', '512']).cout()
#                   ##print df3
#                   ##df3 =dfmi.groupby(['1000', '512'])['1000']
#                   ##print df3
#                   #df3 =dfmi2.groupby(level=2).unstack(level=0).plot(kind='bar', subplots=True)
#                   ##['total time'].count().unstack('preserve').fillna(0)
#                   #print df3
#                   #print 'END'
#                   #df2 = dfmi2.groupby(['Name', 'Abuse/NFF'])['Name'].count().unstack('Abuse/NFF').fillna(0)
#                   #df2[['abuse','nff']].plot(kind='bar', stacked=True)
#                   ###############################################################################
#                   # Now draw graph
#                   ###############################################################################
#                   
#                   
#                   
#                   #                for cdinfo in result[app][ftype][nTasks][inputsize]:
#                   #
#                   #                    print '=======================\n'
#                   #                    print cdinfo
#                   #                    print '=======================\n'
#                   #                    for root in cdinfo:
#                   #                        if root[0] == 'C' and root[1] == 'D':
#                   #                            print root, len(cdinfo[root])
#                   #                            for elem in cdinfo[root]:
#                   #                                print cdinfo[root]["exec"]["max"]
#                   #
#                   
#                   
