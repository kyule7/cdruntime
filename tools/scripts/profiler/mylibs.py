#!/usr/bin/python
import json
import pandas as pd
import numpy as np
import re
import copy

def getJsonString(json_file):
    json_str = '\n'
    for row in json_file.readlines():
#        print 'orig:', row
        splitted = row.split('//')
        if len(splitted) == 1:
#            print 'splt:',splitted[0]
            json_str += row 
        else:
#            print 'splt:',splitted[0]
            json_str += splitted[0] + '\n'
    print json_str
    return json_str

def makeFlatCDs(profile, cdtrace, cds_list):
    for cds in cds_list: # list of sequential CDs
        if cds not in profile:
            profile[cds] = []
        if cds not in cdtrace:
            cdtrace[cds] = []
        print 'CD:', cds
        profile[cds].append(cds_list[cds]['profile'])
        cdtrace[cds].append(cds_list[cds]['trace'])
        if 'child CDs' in cds_list[cds] and len(cds_list[cds]['child CDs']) > 0 :
            makeFlatCDs(profile, cdtrace, cds_list[cds]['child CDs'])


def checkJSONObj(result):
    print 'check json obj'
    for exec_name in result:
        print exec_name
        for fail_type in result[exec_name]:
            print '\t', fail_type
            for numTasks in result[exec_name][fail_type]:
                print '\t\t', numTasks
                for input_size in result[exec_name][fail_type][numTasks]:
                    #print 'Sample:', result[exec_name][fail_type][numTasks]
                    print '\t\t\t', exec_name,fail_type,numTasks,input_size
                    print '\t\t\t\t---- CD info:'
                    for ln in result[exec_name][fail_type][numTasks][input_size]['CD info']:
                        print ln, len(result[exec_name][fail_type][numTasks][input_size]['CD info'][ln])
                    print result[exec_name][fail_type][numTasks][input_size]['CD info']
                    print '\t\t\t\t---- trace:'
                    for ln in result[exec_name][fail_type][numTasks][input_size]['trace']:
                        print ln, len(result[exec_name][fail_type][numTasks][input_size]['trace'][ln])
                    print result[exec_name][fail_type][numTasks][input_size]['trace']
                    print '\t\t\t\t---- total profile:', len(result[exec_name][fail_type][numTasks][input_size]['total profile'])
                    print result[exec_name][fail_type][numTasks][input_size]['total profile']
#                            for meas in result[exec_name][fail_type][numTasks][input_size][elem]:
#                                print meas
                    raw_input('\n\n----------------------------------------------\n\n')

def gatherJSONObj(filelist):
    newfile = open(filelist, 'r')
    for line in newfile:
        file_list = line.split(" ")

    print file_list
    gathered = {}
    tg = []
    for each_filename in file_list:
        print each_filename
        with open(each_filename, 'r') as each:
            jsonfile = getJsonString(each)

#            eachjson = json.loads('\n'.join([row for row in each.readlines() if len(row.split('//')) == 1]))
            eachjson = json.loads(jsonfile)

            # exec_name-ftype-numTasks-input
            exec_name = eachjson['exec_name']
            fail_type = eachjson['ftype'    ]
            numTasks  = eachjson['numTasks' ]
            input_size= eachjson['input'    ]
            cd_info   = eachjson['CD info'  ]
            profile   = eachjson['total profile']
            print exec_name, fail_type, numTasks, input_size
            raw_input('check cdinfo')
            if exec_name not in gathered:
                gathered[exec_name] = {}
            if fail_type not in gathered[exec_name]:
                gathered[exec_name][fail_type] = {}
            if numTasks not in gathered[exec_name][fail_type]:
                gathered[exec_name][fail_type][numTasks] = {}
            if input_size not in gathered[exec_name][fail_type][numTasks]:
                gathered[exec_name][fail_type][numTasks][input_size] = { 'total profile' : [], 'CD info' : {}, 'trace' : {} }
            flattened = {}
            trace = {}
            makeFlatCDs(flattened, trace, cd_info)
            for each in flattened:
                print each, flattened[each]
                print '--'
            for each in trace:
                print each, trace[each]
            print '\n-- profile --'
            print profile
            print '--- So far --', exec_name, fail_type, numTasks, input_size
            gathered[exec_name][fail_type][numTasks][input_size]['total profile'].append(profile)
            for pid in flattened:
                if pid not in gathered[exec_name][fail_type][numTasks][input_size]['CD info']:
                    gathered[exec_name][fail_type][numTasks][input_size]['CD info'][pid] = []
                    gathered[exec_name][fail_type][numTasks][input_size]['trace'][pid] = []
                gathered[exec_name][fail_type][numTasks][input_size]['CD info'][pid].extend(flattened[pid])
                gathered[exec_name][fail_type][numTasks][input_size]['trace'][pid].extend(trace[pid])
            print gathered[exec_name][fail_type][numTasks][input_size]['total profile']
            for pid in gathered[exec_name][fail_type][numTasks][input_size]['CD info']:
                print 'cd phase:', pid, len(gathered[exec_name][fail_type][numTasks][input_size]['CD info'][pid]) 
            for pid in gathered[exec_name][fail_type][numTasks][input_size]['trace']:
                print 'cd phase:', pid, len(gathered[exec_name][fail_type][numTasks][input_size]['trace'][pid])
            print 'cd phase:', gathered[exec_name][fail_type][numTasks][input_size]['trace']
            raw_input('\n\n-------- done ---------------------')
    return gathered

def extractLatencies(cd_tree):
    new_tree = {} # flat tree with CD phases
    print type(cd_tree)
    for pid in cd_tree:
        new_tree[pid] = {}
        new_tree[pid]["exec_trace"] = cd_tree[pid]["exec_trace"]
        new_tree[pid]["cdrt_trace"] = cd_tree[pid]["cdrt_trace"]
        new_tree[pid]["prsv_trace"] = cd_tree[pid]["prsv_trace"]
        new_tree[pid]["max_prsv"]   = cd_tree[pid]["max_prsv"]
        new_tree[pid]["max_cdrt"]   = cd_tree[pid]["max_cdrt"]
    for item in new_tree:
        print "[]trace:", item, len(new_tree[item])
        print "\n"
        for jtem in new_tree[item]:
#            for ktem in new_tree[item][jtem]:
#                print "jnfo:", ktem#new_tree[item]#, len(new_tree[item][jtem])
            if type(new_tree[item]) is list or type(new_tree[item]) is dict:
                print "trace detail:", item
                for ktem in new_tree[item][jtem]:
                    print item, jtem, len(new_tree[item][jtem]) #new_tree[item]#, len(new_tree[item][jtem])
            else:
                print "detail trace:", jtem, new_tree[item]
        raw_input('trace done')
    print '[]done'
    raw_input('--------------[]------------[]---------')
    return new_tree

def removeLatencies(cd_tree):
    new_tree = {} # flat tree with CD phases
    for pid in cd_tree:
        new_tree[pid] = {}
        new_tree[pid]["execution time"]  = cd_tree[pid]["execution time"]
        new_tree[pid]["current counts" ] = cd_tree[pid]["current counts" ]
        new_tree[pid]["input volume"   ] = cd_tree[pid]["input volume"   ]
        new_tree[pid]["output volume"  ] = cd_tree[pid]["output volume"  ]
        new_tree[pid]["rd_bw"          ] = cd_tree[pid]["rd_bw"          ]
        new_tree[pid]["wr_bw"          ] = cd_tree[pid]["wr_bw"          ]
        new_tree[pid]["rd_bw_mea"      ] = cd_tree[pid]["rd_bw_mea"      ]
        new_tree[pid]["wr_bw_mea"      ] = cd_tree[pid]["wr_bw_mea"      ]
        new_tree[pid]["fault rate"     ] = cd_tree[pid]["fault rate"     ]
        new_tree[pid]["tasksize"       ] = cd_tree[pid]["tasksize"       ]
        new_tree[pid]["max prv only bw"] = cd_tree[pid]["max prv only bw"]
        new_tree[pid]["loc prv only bw"] = cd_tree[pid]["loc prv only bw"]
        new_tree[pid]["loc prv time"   ] = cd_tree[pid]["loc prv time"   ]
        new_tree[pid]["CDrt overhead"  ] = cd_tree[pid]["CDrt overhead"  ]
        new_tree[pid]["total preserve" ] = cd_tree[pid]["total preserve" ]
        new_tree[pid]["preserve time"  ] = cd_tree[pid]["preserve time"  ]
        new_tree[pid]["exec"        ]    = cd_tree[pid]["exec"        ]
        new_tree[pid]["reexec"      ]    = cd_tree[pid]["reexec"      ]
        new_tree[pid]["prv_copy"    ]    = cd_tree[pid]["prv_copy"    ]
        new_tree[pid]["prv_ref"     ]    = cd_tree[pid]["prv_ref"     ]
        new_tree[pid]["restore"     ]    = cd_tree[pid]["restore"     ]
        new_tree[pid]["msg_log"     ]    = cd_tree[pid]["msg_log"     ]
        new_tree[pid]["total_time"  ]    = cd_tree[pid]["total_time"  ]
        new_tree[pid]["reex_time"   ]    = cd_tree[pid]["reex_time"   ]
        new_tree[pid]["sync_time"   ]    = cd_tree[pid]["sync_time"   ]
        new_tree[pid]["prv_time"    ]    = cd_tree[pid]["prv_time"    ]
        new_tree[pid]["rst_time"    ]    = cd_tree[pid]["rst_time"    ]
        new_tree[pid]["create_time" ]    = cd_tree[pid]["create_time" ]
        new_tree[pid]["destroy_time"]    = cd_tree[pid]["destroy_time"]
        new_tree[pid]["begin_time"  ]    = cd_tree[pid]["begin_time"  ]
        new_tree[pid]["compl_time"  ]    = cd_tree[pid]["compl_time"  ]
        new_tree[pid]["advance_time"]    = cd_tree[pid]["advance_time"]
    for item in new_tree:
        print "[]trace:", item, len(new_tree[item])
        print "\n"
        for jtem in new_tree[item]:
#            for ktem in new_tree[item][jtem]:
#                print "jnfo:", ktem#new_tree[item]#, len(new_tree[item][jtem])
            if type(new_tree[item][jtem]) is list or type(new_tree[item][jtem]) is dict:
                print "trace detail:", item
                for ktem in new_tree[item][jtem]:
                    print item, jtem, ktem #new_tree[item]#, len(new_tree[item][jtem])
            else:
                print "detail:", jtem, new_tree[item][jtem], new_tree[item][jtem][ktem]
        raw_input('trace done')
    print '[]done'
    raw_input('--------------[]------------[]---------')
    return new_tree

def mergeCDInfo(samples):
    new_tree = {}
    num_samples = {}
    for pid in samples:
        print '\n\n\n', pid, samples[pid], type(samples[pid][0])
        new_tree[pid] = copy.deepcopy(samples[pid][0])
        for each in samples[pid][1:]:
            for elem in new_tree[pid]:
                elemtype = type(new_tree[pid][elem])
                if elemtype is dict:
                    for e in new_tree[pid][elem]:
                       # print '\n\ncheck:', elem, type(each[elem][e]), each[elem]
                        new_tree[pid][elem][e]      += float(each[elem][e]     )
                        new_tree[pid][elem][e]      += float(each[elem][e]     )
                        new_tree[pid][elem][e]      += float(each[elem][e]     )
                        new_tree[pid][elem][e]      += float(each[elem][e]     )
                elif elemtype is float:
                    new_tree[pid][elem]      += float(each[elem]     )
                    if elem == 'preserve time' or elem == 'execution time':
                        print '\n\ncheckf:', elem, type(each[elem]), each[elem], new_tree[pid][elem]
                elif elemtype is int:
#                    print '\n\nchecki:', elem, type(each[elem]), each[elem]
                    new_tree[pid][elem]      += float(each[elem]     )
#            new_tree[pid]["execution time"]      += float(each["execution time"]     )
#            new_tree[pid]["current counts"]      += float(each["current counts"]     )
#            new_tree[pid]["input volume"]        += float(each["input volume"]       )
#            new_tree[pid]["output volume"]       += float(each["output volume"]      )
#            new_tree[pid]["rd_bw"]               += float(each["rd_bw"]              )
#            new_tree[pid]["wr_bw"]               += float(each["wr_bw"]              )
#            new_tree[pid]["rd_bw_mea"]           += float(each["rd_bw_mea"]          )
#            new_tree[pid]["wr_bw_mea"]           += float(each["wr_bw_mea"]          )
#            new_tree[pid]["fault rate"]          += float(each["fault rate"]         )
#            new_tree[pid]["tasksize"]            += float(each["tasksize"]           )
#            new_tree[pid]["max prv only bw"]     += float(each["max prv only bw"]    )
#            new_tree[pid]["loc prv only bw"]     += float(each["loc prv only bw"]    )
#            new_tree[pid]["loc prv time"]        += float(each["loc prv time"]       )
#            new_tree[pid]["CDrt overhead"]       += float(each["CDrt overhead"]      )
#            new_tree[pid]["total preserve"]      += float(each["total preserve"]     )
#            new_tree[pid]["preserve time"]       += float(each["preserve time"]      )
#            new_tree[pid]["exec"        ]["max"] += float(each["exec"        ]["max"])
#            new_tree[pid]["exec"        ]["min"] += float(each["exec"        ]["min"])
#            new_tree[pid]["exec"        ]["avg"] += float(each["exec"        ]["avg"])
#            new_tree[pid]["exec"        ]["std"] += float(each["exec"        ]["std"])
#            new_tree[pid]["reexec"      ]["max"] += float(each["reexec"      ]["max"])
#            new_tree[pid]["reexec"      ]["min"] += float(each["reexec"      ]["min"])
#            new_tree[pid]["reexec"      ]["avg"] += float(each["reexec"      ]["avg"])
#            new_tree[pid]["reexec"      ]["std"] += float(each["reexec"      ]["std"])
#            new_tree[pid]["prv_copy"    ]["max"] += float(each["prv_copy"    ]["max"])
#            new_tree[pid]["prv_copy"    ]["min"] += float(each["prv_copy"    ]["min"])
#            new_tree[pid]["prv_copy"    ]["avg"] += float(each["prv_copy"    ]["avg"])
#            new_tree[pid]["prv_copy"    ]["std"] += float(each["prv_copy"    ]["std"])
#            new_tree[pid]["prv_ref"     ]["max"] += float(each["prv_ref"     ]["max"])
#            new_tree[pid]["prv_ref"     ]["min"] += float(each["prv_ref"     ]["min"])
#            new_tree[pid]["prv_ref"     ]["avg"] += float(each["prv_ref"     ]["avg"])
#            new_tree[pid]["prv_ref"     ]["std"] += float(each["prv_ref"     ]["std"])
#            new_tree[pid]["restore"     ]["max"] += float(each["restore"     ]["max"])
#            new_tree[pid]["restore"     ]["min"] += float(each["restore"     ]["min"])
#            new_tree[pid]["restore"     ]["avg"] += float(each["restore"     ]["avg"])
#            new_tree[pid]["restore"     ]["std"] += float(each["restore"     ]["std"])
#            new_tree[pid]["msg_log"     ]["max"] += float(each["msg_log"     ]["max"])
#            new_tree[pid]["msg_log"     ]["min"] += float(each["msg_log"     ]["min"])
#            new_tree[pid]["msg_log"     ]["avg"] += float(each["msg_log"     ]["avg"])
#            new_tree[pid]["msg_log"     ]["std"] += float(each["msg_log"     ]["std"])
#            new_tree[pid]["total_time"  ]["max"] += float(each["total_time"  ]["max"])
#            new_tree[pid]["total_time"  ]["min"] += float(each["total_time"  ]["min"])
#            new_tree[pid]["total_time"  ]["avg"] += float(each["total_time"  ]["avg"])
#            new_tree[pid]["total_time"  ]["std"] += float(each["total_time"  ]["std"])
#            new_tree[pid]["reex_time"   ]["max"] += float(each["reex_time"   ]["max"])
#            new_tree[pid]["reex_time"   ]["min"] += float(each["reex_time"   ]["min"])
#            new_tree[pid]["reex_time"   ]["avg"] += float(each["reex_time"   ]["avg"])
#            new_tree[pid]["reex_time"   ]["std"] += float(each["reex_time"   ]["std"])
#            new_tree[pid]["sync_time"   ]["max"] += float(each["sync_time"   ]["max"])
#            new_tree[pid]["sync_time"   ]["min"] += float(each["sync_time"   ]["min"])
#            new_tree[pid]["sync_time"   ]["avg"] += float(each["sync_time"   ]["avg"])
#            new_tree[pid]["sync_time"   ]["std"] += float(each["sync_time"   ]["std"])
#            new_tree[pid]["prv_time"    ]["max"] += float(each["prv_time"    ]["max"])
#            new_tree[pid]["prv_time"    ]["min"] += float(each["prv_time"    ]["min"])
#            new_tree[pid]["prv_time"    ]["avg"] += float(each["prv_time"    ]["avg"])
#            new_tree[pid]["prv_time"    ]["std"] += float(each["prv_time"    ]["std"])
#            new_tree[pid]["rst_time"    ]["max"] += float(each["rst_time"    ]["max"])
#            new_tree[pid]["rst_time"    ]["min"] += float(each["rst_time"    ]["min"])
#            new_tree[pid]["rst_time"    ]["avg"] += float(each["rst_time"    ]["avg"])
#            new_tree[pid]["rst_time"    ]["std"] += float(each["rst_time"    ]["std"])
#            new_tree[pid]["create_time" ]["max"] += float(each["create_time" ]["max"])
#            new_tree[pid]["create_time" ]["min"] += float(each["create_time" ]["min"])
#            new_tree[pid]["create_time" ]["avg"] += float(each["create_time" ]["avg"])
#            new_tree[pid]["create_time" ]["std"] += float(each["create_time" ]["std"])
#            new_tree[pid]["destroy_time"]["max"] += float(each["destroy_time"]["max"])
#            new_tree[pid]["destroy_time"]["min"] += float(each["destroy_time"]["min"])
#            new_tree[pid]["destroy_time"]["avg"] += float(each["destroy_time"]["avg"])
#            new_tree[pid]["destroy_time"]["std"] += float(each["destroy_time"]["std"])
#            new_tree[pid]["begin_time"  ]["max"] += float(each["begin_time"  ]["max"])
#            new_tree[pid]["begin_time"  ]["min"] += float(each["begin_time"  ]["min"])
#            new_tree[pid]["begin_time"  ]["avg"] += float(each["begin_time"  ]["avg"])
#            new_tree[pid]["begin_time"  ]["std"] += float(each["begin_time"  ]["std"])
#            new_tree[pid]["compl_time"  ]["max"] += float(each["compl_time"  ]["max"])
#            new_tree[pid]["compl_time"  ]["min"] += float(each["compl_time"  ]["min"])
#            new_tree[pid]["compl_time"  ]["avg"] += float(each["compl_time"  ]["avg"])
#            new_tree[pid]["compl_time"  ]["std"] += float(each["compl_time"  ]["std"])
#            new_tree[pid]["advance_time"]["max"] += float(each["advance_time"]["max"])
#            new_tree[pid]["advance_time"]["min"] += float(each["advance_time"]["min"])
#            new_tree[pid]["advance_time"]["avg"] += float(each["advance_time"]["avg"])
#            new_tree[pid]["advance_time"]["std"] += float(each["advance_time"]["std"])
        num_sample = len(samples[pid])
        for elem in new_tree[pid]:
            elemtype = type(new_tree[pid][elem])
            if elemtype is dict:
                for e in new_tree[pid][elem]:
                    new_tree[pid][elem][e]      /= num_sample
                    new_tree[pid][elem][e]      /= num_sample
                    new_tree[pid][elem][e]      /= num_sample
                    new_tree[pid][elem][e]      /= num_sample
            elif elemtype is float :
                new_tree[pid][elem]             /= num_sample
                if elem == 'preserve time' or elem == 'execution time':
                    print '\n\nfinal checkf:', pid, elem, new_tree[pid][elem]
            elif elemtype is int :
                new_tree[pid][elem]             /= num_sample
#        new_tree[pid]["execution time"]      /= num_sample
#        new_tree[pid]["current counts"]      /= num_sample
#        new_tree[pid]["input volume"]        /= num_sample
#        new_tree[pid]["output volume"]       /= num_sample
#        new_tree[pid]["rd_bw"]               /= num_sample
#        new_tree[pid]["wr_bw"]               /= num_sample
#        new_tree[pid]["rd_bw_mea"]           /= num_sample
#        new_tree[pid]["wr_bw_mea"]           /= num_sample
#        new_tree[pid]["fault rate"]          /= num_sample
#        new_tree[pid]["tasksize"]            /= num_sample
#        new_tree[pid]["max prv only bw"]     /= num_sample
#        new_tree[pid]["loc prv only bw"]     /= num_sample
#        new_tree[pid]["loc prv time"]        /= num_sample
#        new_tree[pid]["CDrt overhead"]       /= num_sample
#        new_tree[pid]["total preserve"]      /= num_sample
#        new_tree[pid]["preserve time"]       /= num_sample
#        new_tree[pid]["exec"        ]["max"] /= num_sample
#        new_tree[pid]["exec"        ]["min"] /= num_sample
#        new_tree[pid]["exec"        ]["avg"] /= num_sample
#        new_tree[pid]["exec"        ]["std"] /= num_sample
#        new_tree[pid]["reexec"      ]["max"] /= num_sample
#        new_tree[pid]["reexec"      ]["min"] /= num_sample
#        new_tree[pid]["reexec"      ]["avg"] /= num_sample
#        new_tree[pid]["reexec"      ]["std"] /= num_sample
#        new_tree[pid]["prv_copy"    ]["max"] /= num_sample
#        new_tree[pid]["prv_copy"    ]["min"] /= num_sample
#        new_tree[pid]["prv_copy"    ]["avg"] /= num_sample
#        new_tree[pid]["prv_copy"    ]["std"] /= num_sample
#        new_tree[pid]["prv_ref"     ]["max"] /= num_sample
#        new_tree[pid]["prv_ref"     ]["min"] /= num_sample
#        new_tree[pid]["prv_ref"     ]["avg"] /= num_sample
#        new_tree[pid]["prv_ref"     ]["std"] /= num_sample
#        new_tree[pid]["restore"     ]["max"] /= num_sample
#        new_tree[pid]["restore"     ]["min"] /= num_sample
#        new_tree[pid]["restore"     ]["avg"] /= num_sample
#        new_tree[pid]["restore"     ]["std"] /= num_sample
#        new_tree[pid]["msg_log"     ]["max"] /= num_sample
#        new_tree[pid]["msg_log"     ]["min"] /= num_sample
#        new_tree[pid]["msg_log"     ]["avg"] /= num_sample
#        new_tree[pid]["msg_log"     ]["std"] /= num_sample
#        new_tree[pid]["total_time"  ]["max"] /= num_sample
#        new_tree[pid]["total_time"  ]["min"] /= num_sample
#        new_tree[pid]["total_time"  ]["avg"] /= num_sample
#        new_tree[pid]["total_time"  ]["std"] /= num_sample
#        new_tree[pid]["reex_time"   ]["max"] /= num_sample
#        new_tree[pid]["reex_time"   ]["min"] /= num_sample
#        new_tree[pid]["reex_time"   ]["avg"] /= num_sample
#        new_tree[pid]["reex_time"   ]["std"] /= num_sample
#        new_tree[pid]["sync_time"   ]["max"] /= num_sample
#        new_tree[pid]["sync_time"   ]["min"] /= num_sample
#        new_tree[pid]["sync_time"   ]["avg"] /= num_sample
#        new_tree[pid]["sync_time"   ]["std"] /= num_sample
#        new_tree[pid]["prv_time"    ]["max"] /= num_sample
#        new_tree[pid]["prv_time"    ]["min"] /= num_sample
#        new_tree[pid]["prv_time"    ]["avg"] /= num_sample
#        new_tree[pid]["prv_time"    ]["std"] /= num_sample
#        new_tree[pid]["rst_time"    ]["max"] /= num_sample
#        new_tree[pid]["rst_time"    ]["min"] /= num_sample
#        new_tree[pid]["rst_time"    ]["avg"] /= num_sample
#        new_tree[pid]["rst_time"    ]["std"] /= num_sample
#        new_tree[pid]["create_time" ]["max"] /= num_sample
#        new_tree[pid]["create_time" ]["min"] /= num_sample
#        new_tree[pid]["create_time" ]["avg"] /= num_sample
#        new_tree[pid]["create_time" ]["std"] /= num_sample
#        new_tree[pid]["destroy_time"]["max"] /= num_sample
#        new_tree[pid]["destroy_time"]["min"] /= num_sample
#        new_tree[pid]["destroy_time"]["avg"] /= num_sample
#        new_tree[pid]["destroy_time"]["std"] /= num_sample
#        new_tree[pid]["begin_time"  ]["max"] /= num_sample
#        new_tree[pid]["begin_time"  ]["min"] /= num_sample
#        new_tree[pid]["begin_time"  ]["avg"] /= num_sample
#        new_tree[pid]["begin_time"  ]["std"] /= num_sample
#        new_tree[pid]["compl_time"  ]["max"] /= num_sample
#        new_tree[pid]["compl_time"  ]["min"] /= num_sample
#        new_tree[pid]["compl_time"  ]["avg"] /= num_sample
#        new_tree[pid]["compl_time"  ]["std"] /= num_sample
#        new_tree[pid]["advance_time"]["max"] /= num_sample
#        new_tree[pid]["advance_time"]["min"] /= num_sample
#        new_tree[pid]["advance_time"]["avg"] /= num_sample
#        new_tree[pid]["advance_time"]["std"] /= num_sample
    return new_tree

def mergeTotalProf(samples):
    num_sample = len(samples)
    print '\n\n\n', num_sample, samples, type(samples[0])
    new_prof = copy.deepcopy(samples[0])
    for each in samples[1:]:
        for elem in new_prof:
            if type(new_prof[elem]) is list:
                new_prof[elem][0]      += float(each[elem][0]     )
                new_prof[elem][1]      += float(each[elem][1]     )
                new_prof[elem][2]      += float(each[elem][2]     )
                new_prof[elem][3]      += float(each[elem][3]     )
            else:
                new_prof[elem]         += float(each[elem]     )
#        new_prof["total time"    ][0]   += float(each["total time"    ][0])
#        new_prof["total time"    ][1]   += float(each["total time"    ][1])
#        new_prof["total time"    ][2]   += float(each["total time"    ][2])
#        new_prof["total time"    ][3]   += float(each["total time"    ][3])
#        new_prof["reex time"     ][0]   += float(each["reex time"     ][0])
#        new_prof["reex time"     ][1]   += float(each["reex time"     ][1])
#        new_prof["reex time"     ][2]   += float(each["reex time"     ][2])
#        new_prof["reex time"     ][3]   += float(each["reex time"     ][3])
#        new_prof["CD overhead"   ][0]   += float(each["CD overhead"   ][0])
#        new_prof["CD overhead"   ][1]   += float(each["CD overhead"   ][1])
#        new_prof["CD overhead"   ][2]   += float(each["CD overhead"   ][2])
#        new_prof["CD overhead"   ][3]   += float(each["CD overhead"   ][3])
#        new_prof["CD body"       ][0]   += float(each["CD overhead"   ][0])
#        new_prof["CD body"       ][1]   += float(each["CD overhead"   ][1])
#        new_prof["CD body"       ][2]   += float(each["CD overhead"   ][2])
#        new_prof["CD body"       ][3]   += float(each["CD overhead"   ][3])
#        new_prof["sync time exec"][0]   += float(each["sync time exec"][0])
#        new_prof["sync time exec"][1]   += float(each["sync time exec"][1])
#        new_prof["sync time exec"][2]   += float(each["sync time exec"][2])
#        new_prof["sync time exec"][3]   += float(each["sync time exec"][3])
#        new_prof["sync time reex"][0]   += float(each["sync time reex"][0])
#        new_prof["sync time reex"][1]   += float(each["sync time reex"][1])
#        new_prof["sync time reex"][2]   += float(each["sync time reex"][2])
#        new_prof["sync time reex"][3]   += float(each["sync time reex"][3])
#        new_prof["sync time recr"][0]   += float(each["sync time recr"][0])
#        new_prof["sync time recr"][1]   += float(each["sync time recr"][1])
#        new_prof["sync time recr"][2]   += float(each["sync time recr"][2])
#        new_prof["sync time recr"][3]   += float(each["sync time recr"][3])
#        new_prof["preserve time" ][0]   += float(each["preserve time" ][0])
#        new_prof["preserve time" ][1]   += float(each["preserve time" ][1])
#        new_prof["preserve time" ][2]   += float(each["preserve time" ][2])
#        new_prof["preserve time" ][3]   += float(each["preserve time" ][3])
#        new_prof["restore time"  ][0]   += float(each["restore time"  ][0])
#        new_prof["restore time"  ][1]   += float(each["restore time"  ][1])
#        new_prof["restore time"  ][2]   += float(each["restore time"  ][2])
#        new_prof["restore time"  ][3]   += float(each["restore time"  ][3])
#        new_prof["create time"   ][0]   += float(each["create time"   ][0])
#        new_prof["create time"   ][1]   += float(each["create time"   ][1])
#        new_prof["create time"   ][2]   += float(each["create time"   ][2])
#        new_prof["create time"   ][3]   += float(each["create time"   ][3])
#        new_prof["destory time"  ][0]   += float(each["destory time"  ][0])
#        new_prof["destory time"  ][1]   += float(each["destory time"  ][1])
#        new_prof["destory time"  ][2]   += float(each["destory time"  ][2])
#        new_prof["destory time"  ][3]   += float(each["destory time"  ][3])
#        new_prof["begin time"    ][0]   += float(each["begin time"    ][0])
#        new_prof["begin time"    ][1]   += float(each["begin time"    ][1])
#        new_prof["begin time"    ][2]   += float(each["begin time"    ][2])
#        new_prof["begin time"    ][3]   += float(each["begin time"    ][3])
#        new_prof["complete time" ][0]   += float(each["complete time" ][0])
#        new_prof["complete time" ][1]   += float(each["complete time" ][1])
#        new_prof["complete time" ][2]   += float(each["complete time" ][2])
#        new_prof["complete time" ][3]   += float(each["complete time" ][3])
#        new_prof[message_time    ][0]   += float(each[message_time    ][0])
#        new_prof[message_time    ][1]   += float(each[message_time    ][1])
#        new_prof[message_time    ][2]   += float(each[message_time    ][2])
#        new_prof[message_time    ][3]   += float(each[message_time    ][3])
#        new_prof["libc logging"  ][0]   += float(each["libc logging"  ][0])
#        new_prof["libc logging"  ][1]   += float(each["libc logging"  ][1])
#        new_prof["libc logging"  ][2]   += float(each["libc logging"  ][2])
#        new_prof["libc logging"  ][3]   += float(each["libc logging"  ][3])
#        new_prof["mailbox overhead"] += float(each["mailbox overhead"])

    for elem in new_prof:
        if type(new_prof[elem]) is list:
            new_prof[elem][0]      /= num_sample
            new_prof[elem][1]      /= num_sample
            new_prof[elem][2]      /= num_sample
            new_prof[elem][3]      /= num_sample
        else:
            new_prof[elem]         /= num_sample
#    new_prof["total time"    ][0]   /= num_sample
#    new_prof["total time"    ][1]   /= num_sample
#    new_prof["total time"    ][2]   /= num_sample
#    new_prof["total time"    ][3]   /= num_sample
#    new_prof["reex time"     ][0]   /= num_sample
#    new_prof["reex time"     ][1]   /= num_sample
#    new_prof["reex time"     ][2]   /= num_sample
#    new_prof["reex time"     ][3]   /= num_sample
#    new_prof["CD overhead"   ][0]   /= num_sample
#    new_prof["CD overhead"   ][1]   /= num_sample
#    new_prof["CD overhead"   ][2]   /= num_sample
#    new_prof["CD overhead"   ][3]   /= num_sample
#    new_prof["CD body"       ][0]   /= num_sample
#    new_prof["CD body"       ][1]   /= num_sample
#    new_prof["CD body"       ][2]   /= num_sample
#    new_prof["CD body"       ][3]   /= num_sample
#    new_prof["sync time exec"][0]   /= num_sample
#    new_prof["sync time exec"][1]   /= num_sample
#    new_prof["sync time exec"][2]   /= num_sample
#    new_prof["sync time exec"][3]   /= num_sample
#    new_prof["sync time reex"][0]   /= num_sample
#    new_prof["sync time reex"][1]   /= num_sample
#    new_prof["sync time reex"][2]   /= num_sample
#    new_prof["sync time reex"][3]   /= num_sample
#    new_prof["sync time recr"][0]   /= num_sample
#    new_prof["sync time recr"][1]   /= num_sample
#    new_prof["sync time recr"][2]   /= num_sample
#    new_prof["sync time recr"][3]   /= num_sample
#    new_prof["preserve time" ][0]   /= num_sample
#    new_prof["preserve time" ][1]   /= num_sample
#    new_prof["preserve time" ][2]   /= num_sample
#    new_prof["preserve time" ][3]   /= num_sample
#    new_prof["restore time"  ][0]   /= num_sample
#    new_prof["restore time"  ][1]   /= num_sample
#    new_prof["restore time"  ][2]   /= num_sample
#    new_prof["restore time"  ][3]   /= num_sample
#    new_prof["create time"   ][0]   /= num_sample
#    new_prof["create time"   ][1]   /= num_sample
#    new_prof["create time"   ][2]   /= num_sample
#    new_prof["create time"   ][3]   /= num_sample
#    new_prof["destory time"  ][0]   /= num_sample
#    new_prof["destory time"  ][1]   /= num_sample
#    new_prof["destory time"  ][2]   /= num_sample
#    new_prof["destory time"  ][3]   /= num_sample
#    new_prof["begin time"    ][0]   /= num_sample
#    new_prof["begin time"    ][1]   /= num_sample
#    new_prof["begin time"    ][2]   /= num_sample
#    new_prof["begin time"    ][3]   /= num_sample
#    new_prof["complete time" ][0]   /= num_sample
#    new_prof["complete time" ][1]   /= num_sample
#    new_prof["complete time" ][2]   /= num_sample
#    new_prof["complete time" ][3]   /= num_sample
#    new_prof[message_time    ][0]   /= num_sample
#    new_prof[message_time    ][1]   /= num_sample
#    new_prof[message_time    ][2]   /= num_sample
#    new_prof[message_time    ][3]   /= num_sample
#    new_prof["libc logging"  ][0]   /= num_sample
#    new_prof["libc logging"  ][1]   /= num_sample
#    new_prof["libc logging"  ][2]   /= num_sample
#    new_prof["libc logging"  ][3]   /= num_sample
#    new_prof["mailbox overhead"]    /= num_sample
    return new_prof

def mergeTraces(samples):
    new_tree = {}
    num_samples = {}
    for pid in samples:
        print '\n\n\n', pid, samples[pid], type(samples[pid][0])
        new_tree[pid] = copy.deepcopy(samples[pid][0])
        for each in samples[pid][1:]:
            new_tree[pid]["exec_trace"].extend(each["exec_trace"])
            new_tree[pid]["cdrt_trace"].extend(each["cdrt_trace"])
            new_tree[pid]["prsv_trace"].extend(each["prsv_trace"])
            new_tree[pid]["max_prsv"].extend(each["max_prsv"])
            new_tree[pid]["max_cdrt"].extend(each["max_cdrt"])

    return new_tree

def mergeProfiles(cd_profile):
    cdinfo = mergeCDInfo(cd_profile['CD info'])
    trace  = mergeTraces(cd_profile['trace'])
    tot_prof = mergeTotalProf(cd_profile['total profile'])
    raw_input('-----------[]---------------[]-----------------')
    return  cdinfo, trace, tot_prof 
#    for elem in cd_profile['total profile']:
#        print type(elem)
#        print elem
#    for elem in cd_profile['trace']:
#        print type(elem)
#        print elem
    
#    print 'len:', len(cd_profile_list), len(cdtrace_list)


message_time = "messaging time"
reex_time = "reex time"    
def averageCDTree(cdtree_list, trace_infos):

    #print 'now start\n\n'
    print 'cdtree_list type:', type(cdtree_list)
    #print 'now start'
    if len(cdtree_list['CD info']) == 1:
        result = copy.deepcopy(copy.deepcopy(cdtree_list['CD info'][0]))
        result = {}
        result['CD info']          = copy.deepcopy(cdtree_list['CD info'][0])
        result["total time"    ]   = copy.deepcopy(cdtree_list["total time"    ][0])
        result["reex time"     ]   = copy.deepcopy(cdtree_list["reex time"     ][0])
        result["CD overhead"   ]   = copy.deepcopy(cdtree_list["CD overhead"   ][0])
        result["sync time exec"]   = copy.deepcopy(cdtree_list["sync time exec"][0])
        result["sync time reex"]   = copy.deepcopy(cdtree_list["sync time reex"][0])
        result["sync time recr"]   = copy.deepcopy(cdtree_list["sync time recr"][0])
        result["preserve time" ]   = copy.deepcopy(cdtree_list["preserve time" ][0])
        result["restore time"  ]   = copy.deepcopy(cdtree_list["restore time"  ][0])
        result["create time"   ]   = copy.deepcopy(cdtree_list["create time"   ][0])
        result["destory time"  ]   = copy.deepcopy(cdtree_list["destory time"  ][0])
        result["begin time"    ]   = copy.deepcopy(cdtree_list["begin time"    ][0])
        result["complete time" ]   = copy.deepcopy(cdtree_list["complete time" ][0])
        result[message_time    ]   = copy.deepcopy(cdtree_list[message_time    ][0])
        result["libc logging"  ]   = copy.deepcopy(cdtree_list["libc logging"  ][0])
        result["mailbox overhead"] = copy.deepcopy(cdtree_list["mailbox overhead"][0])
        return result
    else:
        result = {}
        result['CD info']          = copy.deepcopy(cdtree_list['CD info'][0])
        result["total time"    ]   = copy.deepcopy(np.mean(cdtree_list["total time"    ]  ))
        result["reex time"     ]   = copy.deepcopy(np.mean(cdtree_list["reex time"     ]  ))
        result["CD overhead"   ]   = copy.deepcopy(np.mean(cdtree_list["CD overhead"   ]  ))
        result["sync time exec"]   = copy.deepcopy(np.mean(cdtree_list["sync time exec"]  ))
        result["sync time reex"]   = copy.deepcopy(np.mean(cdtree_list["sync time reex"]  ))
        result["sync time recr"]   = copy.deepcopy(np.mean(cdtree_list["sync time recr"]  ))
        result["preserve time" ]   = copy.deepcopy(np.mean(cdtree_list["preserve time" ]  ))
        result["restore time"  ]   = copy.deepcopy(np.mean(cdtree_list["restore time"  ]  ))
        result["create time"   ]   = copy.deepcopy(np.mean(cdtree_list["create time"   ]  ))
        result["destory time"  ]   = copy.deepcopy(np.mean(cdtree_list["destory time"  ]  ))
        result["begin time"    ]   = copy.deepcopy(np.mean(cdtree_list["begin time"    ]  ))
        result["complete time" ]   = copy.deepcopy(np.mean(cdtree_list["complete time" ]  ))
        result[message_time    ]   = copy.deepcopy(np.mean(cdtree_list[message_time    ]  ))
        result["libc logging"  ]   = copy.deepcopy(np.mean(cdtree_list["libc logging"  ]  ))
        result["mailbox overhead"] = copy.deepcopy(np.mean(cdtree_list["mailbox overhead"]))
        result_infos = result['CD info']
        print 'result_infos:', type(result_infos)
        raw_input('1111111111111111111')
        num_meas = len(result['CD info'])
        if len(result_infos) == 0:
            raise SystemExit
            return result
        else:
            print 'extract info'
            trace_infos = copy.deepcopy(result['CD info'])
            extractLatencies(trace_infos)
            removeLatencies(result_infos)
            for item in result_infos:
                print "info:", item, len(result_infos[item])
                print "\n"
                for jtem in result_infos[item]:
                    #print "detail:", result_infos[item]#, len(result_infos[item][jtem])
                    if type(result_infos[item][jtem]) is list or type(result_infos[item][jtem]) is dict:
                        print "detail:", item
                        for ktem in result_infos[item][jtem]:
                            print item, jtem, ktem #result_infos[item]#, len(result_infos[item][jtem])
                    else:
                        print "detail:", jtem, result_infos[item][jtem]
                raw_input('extract info done')
            print 'trace'
            for item in trace_infos:
                print "trace:", item, len(trace_infos[item])
                print "\n"
                for jtem in trace_infos[item]:
#                    for ktem in trace_infos[item][jtem]:
#                        print "jnfo:", ktem#trace_infos[item]#, len(trace_infos[item][jtem])
                    if type(trace_infos[item][jtem]) is list or type(trace_infos[item][jtem]) is dict:
                        print "trace detail:", item
                        for ktem in trace_infos[item][jtem]:
                            print 'len:', item, jtem, len(trace_infos[item][jtem]) #trace_infos[item]#, len(trace_infos[item][jtem])
                    else:
                        print "detail:", jtem, trace_infos[item][jtem]
                raw_input('trace done')
            print 'done'
        for cdtree in cdtree_list['CD info'][1:]:
            cdtree_infos = cdtree
            for phase_id in cdtree_infos:
                if phase_id == "CD_1_1" and cdtree_infos[phase_id]["tasksize"] == 1000 and cdtree_infos[phase_id]["label"] == "MainLoop":
                    print "prv time: ", cdtree_infos[phase_id]["total preserve"], result_infos[phase_id]["total preserve"]
#                    raw_input("!!!!!!!!!!!!")

                print "phase : ", phase_id, len(cdtree_list)
                if phase_id not in result_infos:

                    print "phase is not here: ", len(result_infos), len(result['CD info']), len(cdtree_list['CD info'][0])
                    for phases in result_infos:
                        print phases

#                raw_input("@@@@")
                result_infos[phase_id]["execution time"] += float(cdtree_infos[phase_id]["execution time"] )
                result_infos[phase_id]["current counts"] += float(cdtree_infos[phase_id]["current counts"] )
                result_infos[phase_id]["input volume"]   += float(cdtree_infos[phase_id]["input volume"]   )
                result_infos[phase_id]["output volume"]  += float(cdtree_infos[phase_id]["output volume"]  )
                result_infos[phase_id]["rd_bw"]          += float(cdtree_infos[phase_id]["rd_bw"]          )
                result_infos[phase_id]["wr_bw"]          += float(cdtree_infos[phase_id]["wr_bw"]          )
                result_infos[phase_id]["rd_bw_mea"]      += float(cdtree_infos[phase_id]["rd_bw_mea"]      )
                result_infos[phase_id]["wr_bw_mea"]      += float(cdtree_infos[phase_id]["wr_bw_mea"]      )
                result_infos[phase_id]["fault rate"]     += float(cdtree_infos[phase_id]["fault rate"]     )
                result_infos[phase_id]["tasksize"]      += float(cdtree_infos[phase_id]["tasksize"]      )
                result_infos[phase_id]["max prv only bw"]+= float(cdtree_infos[phase_id]["max prv only bw"])
                result_infos[phase_id]["loc prv only bw"]+= float(cdtree_infos[phase_id]["loc prv only bw"])
                result_infos[phase_id]["loc prv time"]   += float(cdtree_infos[phase_id]["loc prv time"]   )
                result_infos[phase_id]["CDrt overhead"]  += float(cdtree_infos[phase_id]["CDrt overhead"]  )
                result_infos[phase_id]["total preserve"]  += float(cdtree_infos[phase_id]["total preserve"]  )
                result_infos[phase_id]["preserve time"]  += float(cdtree_infos[phase_id]["preserve time"]  )
                result_infos[phase_id]["exec"        ]["max"] += float(cdtree_infos[phase_id]["exec"        ]["max"])
                result_infos[phase_id]["exec"        ]["min"] += float(cdtree_infos[phase_id]["exec"        ]["min"])
                result_infos[phase_id]["exec"        ]["avg"] += float(cdtree_infos[phase_id]["exec"        ]["avg"])
                result_infos[phase_id]["exec"        ]["std"] += float(cdtree_infos[phase_id]["exec"        ]["std"])
                result_infos[phase_id]["reexec"      ]["max"] += float(cdtree_infos[phase_id]["reexec"      ]["max"])
                result_infos[phase_id]["reexec"      ]["min"] += float(cdtree_infos[phase_id]["reexec"      ]["min"])
                result_infos[phase_id]["reexec"      ]["avg"] += float(cdtree_infos[phase_id]["reexec"      ]["avg"])
                result_infos[phase_id]["reexec"      ]["std"] += float(cdtree_infos[phase_id]["reexec"      ]["std"])
                result_infos[phase_id]["prv_copy"    ]["max"] += float(cdtree_infos[phase_id]["prv_copy"    ]["max"])
                result_infos[phase_id]["prv_copy"    ]["min"] += float(cdtree_infos[phase_id]["prv_copy"    ]["min"])
                result_infos[phase_id]["prv_copy"    ]["avg"] += float(cdtree_infos[phase_id]["prv_copy"    ]["avg"])
                result_infos[phase_id]["prv_copy"    ]["std"] += float(cdtree_infos[phase_id]["prv_copy"    ]["std"])
                result_infos[phase_id]["prv_ref"     ]["max"] += float(cdtree_infos[phase_id]["prv_ref"     ]["max"])
                result_infos[phase_id]["prv_ref"     ]["min"] += float(cdtree_infos[phase_id]["prv_ref"     ]["min"])
                result_infos[phase_id]["prv_ref"     ]["avg"] += float(cdtree_infos[phase_id]["prv_ref"     ]["avg"])
                result_infos[phase_id]["prv_ref"     ]["std"] += float(cdtree_infos[phase_id]["prv_ref"     ]["std"])
                result_infos[phase_id]["restore"     ]["max"] += float(cdtree_infos[phase_id]["restore"     ]["max"])
                result_infos[phase_id]["restore"     ]["min"] += float(cdtree_infos[phase_id]["restore"     ]["min"])
                result_infos[phase_id]["restore"     ]["avg"] += float(cdtree_infos[phase_id]["restore"     ]["avg"])
                result_infos[phase_id]["restore"     ]["std"] += float(cdtree_infos[phase_id]["restore"     ]["std"])
                result_infos[phase_id]["msg_log"     ]["max"] += float(cdtree_infos[phase_id]["msg_log"     ]["max"])
                result_infos[phase_id]["msg_log"     ]["min"] += float(cdtree_infos[phase_id]["msg_log"     ]["min"])
                result_infos[phase_id]["msg_log"     ]["avg"] += float(cdtree_infos[phase_id]["msg_log"     ]["avg"])
                result_infos[phase_id]["msg_log"     ]["std"] += float(cdtree_infos[phase_id]["msg_log"     ]["std"])
                result_infos[phase_id]["total_time"  ]["max"] += float(cdtree_infos[phase_id]["total_time"  ]["max"])
                result_infos[phase_id]["total_time"  ]["min"] += float(cdtree_infos[phase_id]["total_time"  ]["min"])
                result_infos[phase_id]["total_time"  ]["avg"] += float(cdtree_infos[phase_id]["total_time"  ]["avg"])
                result_infos[phase_id]["total_time"  ]["std"] += float(cdtree_infos[phase_id]["total_time"  ]["std"])
                result_infos[phase_id]["reex_time"   ]["max"] += float(cdtree_infos[phase_id]["reex_time"   ]["max"])
                result_infos[phase_id]["reex_time"   ]["min"] += float(cdtree_infos[phase_id]["reex_time"   ]["min"])
                result_infos[phase_id]["reex_time"   ]["avg"] += float(cdtree_infos[phase_id]["reex_time"   ]["avg"])
                result_infos[phase_id]["reex_time"   ]["std"] += float(cdtree_infos[phase_id]["reex_time"   ]["std"])
                result_infos[phase_id]["sync_time"   ]["max"] += float(cdtree_infos[phase_id]["sync_time"   ]["max"])
                result_infos[phase_id]["sync_time"   ]["min"] += float(cdtree_infos[phase_id]["sync_time"   ]["min"])
                result_infos[phase_id]["sync_time"   ]["avg"] += float(cdtree_infos[phase_id]["sync_time"   ]["avg"])
                result_infos[phase_id]["sync_time"   ]["std"] += float(cdtree_infos[phase_id]["sync_time"   ]["std"])
                result_infos[phase_id]["prv_time"    ]["max"] += float(cdtree_infos[phase_id]["prv_time"    ]["max"])
                result_infos[phase_id]["prv_time"    ]["min"] += float(cdtree_infos[phase_id]["prv_time"    ]["min"])
                result_infos[phase_id]["prv_time"    ]["avg"] += float(cdtree_infos[phase_id]["prv_time"    ]["avg"])
                result_infos[phase_id]["prv_time"    ]["std"] += float(cdtree_infos[phase_id]["prv_time"    ]["std"])
                result_infos[phase_id]["rst_time"    ]["max"] += float(cdtree_infos[phase_id]["rst_time"    ]["max"])
                result_infos[phase_id]["rst_time"    ]["min"] += float(cdtree_infos[phase_id]["rst_time"    ]["min"])
                result_infos[phase_id]["rst_time"    ]["avg"] += float(cdtree_infos[phase_id]["rst_time"    ]["avg"])
                result_infos[phase_id]["rst_time"    ]["std"] += float(cdtree_infos[phase_id]["rst_time"    ]["std"])
                result_infos[phase_id]["create_time" ]["max"] += float(cdtree_infos[phase_id]["create_time" ]["max"])
                result_infos[phase_id]["create_time" ]["min"] += float(cdtree_infos[phase_id]["create_time" ]["min"])
                result_infos[phase_id]["create_time" ]["avg"] += float(cdtree_infos[phase_id]["create_time" ]["avg"])
                result_infos[phase_id]["create_time" ]["std"] += float(cdtree_infos[phase_id]["create_time" ]["std"])
                result_infos[phase_id]["destroy_time"]["max"] += float(cdtree_infos[phase_id]["destroy_time"]["max"])
                result_infos[phase_id]["destroy_time"]["min"] += float(cdtree_infos[phase_id]["destroy_time"]["min"])
                result_infos[phase_id]["destroy_time"]["avg"] += float(cdtree_infos[phase_id]["destroy_time"]["avg"])
                result_infos[phase_id]["destroy_time"]["std"] += float(cdtree_infos[phase_id]["destroy_time"]["std"])
                result_infos[phase_id]["begin_time"  ]["max"] += float(cdtree_infos[phase_id]["begin_time"  ]["max"])
                result_infos[phase_id]["begin_time"  ]["min"] += float(cdtree_infos[phase_id]["begin_time"  ]["min"])
                result_infos[phase_id]["begin_time"  ]["avg"] += float(cdtree_infos[phase_id]["begin_time"  ]["avg"])
                result_infos[phase_id]["begin_time"  ]["std"] += float(cdtree_infos[phase_id]["begin_time"  ]["std"])
                result_infos[phase_id]["compl_time"  ]["max"] += float(cdtree_infos[phase_id]["compl_time"  ]["max"])
                result_infos[phase_id]["compl_time"  ]["min"] += float(cdtree_infos[phase_id]["compl_time"  ]["min"])
                result_infos[phase_id]["compl_time"  ]["avg"] += float(cdtree_infos[phase_id]["compl_time"  ]["avg"])
                result_infos[phase_id]["compl_time"  ]["std"] += float(cdtree_infos[phase_id]["compl_time"  ]["std"])
                result_infos[phase_id]["advance_time"]["max"] += float(cdtree_infos[phase_id]["advance_time"]["max"])
                result_infos[phase_id]["advance_time"]["min"] += float(cdtree_infos[phase_id]["advance_time"]["min"])
                result_infos[phase_id]["advance_time"]["avg"] += float(cdtree_infos[phase_id]["advance_time"]["avg"])
                result_infos[phase_id]["advance_time"]["std"] += float(cdtree_infos[phase_id]["advance_time"]["std"])

                trace_infos[phase_id]["exec_trace"].extend(cdtree_infos[phase_id]["exec_trace"])
                trace_infos[phase_id]["cdrt_trace"].extend(cdtree_infos[phase_id]["cdrt_trace"])
                trace_infos[phase_id]["prsv_trace"].extend(cdtree_infos[phase_id]["prsv_trace"])
                trace_infos[phase_id]["max_prsv"].extend(cdtree_infos[phase_id]["max_prsv"])
                trace_infos[phase_id]["max_cdrt"].extend(cdtree_infos[phase_id]["max_cdrt"])
        num_results = len(cdtree_list['CD info'])
        print 'num results:', num_results
    
        for phase_id in result_infos:
            result_infos[phase_id]["execution time"] /= num_results
            result_infos[phase_id]["current counts"] /= num_results
            result_infos[phase_id]["input volume"]   /= num_results
            result_infos[phase_id]["output volume"]  /= num_results
            result_infos[phase_id]["rd_bw"]          /= num_results
            result_infos[phase_id]["wr_bw"]          /= num_results
            result_infos[phase_id]["rd_bw_mea"]      /= num_results
            result_infos[phase_id]["wr_bw_mea"]      /= num_results
            result_infos[phase_id]["fault rate"]     /= num_results
            result_infos[phase_id]["tasksize"]      /= num_results
            result_infos[phase_id]["max prv only bw"]/= num_results
            result_infos[phase_id]["loc prv only bw"]/= num_results
            result_infos[phase_id]["loc prv time"]   /= num_results
            result_infos[phase_id]["CDrt overhead"]  /= num_results
            result_infos[phase_id]["total preserve"]  /= num_results
            result_infos[phase_id]["preserve time"]  /= num_results
            result_infos[phase_id]["exec"        ]["max"] /= num_results
            result_infos[phase_id]["exec"        ]["min"] /= num_results
            result_infos[phase_id]["exec"        ]["avg"] /= num_results
            result_infos[phase_id]["exec"        ]["std"] /= num_results
            result_infos[phase_id]["reexec"      ]["max"] /= num_results
            result_infos[phase_id]["reexec"      ]["min"] /= num_results
            result_infos[phase_id]["reexec"      ]["avg"] /= num_results
            result_infos[phase_id]["reexec"      ]["std"] /= num_results
            result_infos[phase_id]["prv_copy"    ]["max"] /= num_results
            result_infos[phase_id]["prv_copy"    ]["min"] /= num_results
            result_infos[phase_id]["prv_copy"    ]["avg"] /= num_results
            result_infos[phase_id]["prv_copy"    ]["std"] /= num_results
            result_infos[phase_id]["prv_ref"     ]["max"] /= num_results
            result_infos[phase_id]["prv_ref"     ]["min"] /= num_results
            result_infos[phase_id]["prv_ref"     ]["avg"] /= num_results
            result_infos[phase_id]["prv_ref"     ]["std"] /= num_results
            result_infos[phase_id]["restore"     ]["max"] /= num_results
            result_infos[phase_id]["restore"     ]["min"] /= num_results
            result_infos[phase_id]["restore"     ]["avg"] /= num_results
            result_infos[phase_id]["restore"     ]["std"] /= num_results
            result_infos[phase_id]["msg_log"     ]["max"] /= num_results
            result_infos[phase_id]["msg_log"     ]["min"] /= num_results
            result_infos[phase_id]["msg_log"     ]["avg"] /= num_results
            result_infos[phase_id]["msg_log"     ]["std"] /= num_results
            result_infos[phase_id]["total_time"  ]["max"] /= num_results
            result_infos[phase_id]["total_time"  ]["min"] /= num_results
            result_infos[phase_id]["total_time"  ]["avg"] /= num_results
            result_infos[phase_id]["total_time"  ]["std"] /= num_results
            result_infos[phase_id]["reex_time"   ]["max"] /= num_results
            result_infos[phase_id]["reex_time"   ]["min"] /= num_results
            result_infos[phase_id]["reex_time"   ]["avg"] /= num_results
            result_infos[phase_id]["reex_time"   ]["std"] /= num_results
            result_infos[phase_id]["sync_time"   ]["max"] /= num_results
            result_infos[phase_id]["sync_time"   ]["min"] /= num_results
            result_infos[phase_id]["sync_time"   ]["avg"] /= num_results
            result_infos[phase_id]["sync_time"   ]["std"] /= num_results
            result_infos[phase_id]["prv_time"    ]["max"] /= num_results
            result_infos[phase_id]["prv_time"    ]["min"] /= num_results
            result_infos[phase_id]["prv_time"    ]["avg"] /= num_results
            result_infos[phase_id]["prv_time"    ]["std"] /= num_results
            result_infos[phase_id]["rst_time"    ]["max"] /= num_results
            result_infos[phase_id]["rst_time"    ]["min"] /= num_results
            result_infos[phase_id]["rst_time"    ]["avg"] /= num_results
            result_infos[phase_id]["rst_time"    ]["std"] /= num_results
            result_infos[phase_id]["create_time" ]["max"] /= num_results
            result_infos[phase_id]["create_time" ]["min"] /= num_results
            result_infos[phase_id]["create_time" ]["avg"] /= num_results
            result_infos[phase_id]["create_time" ]["std"] /= num_results
            result_infos[phase_id]["destroy_time"]["max"] /= num_results
            result_infos[phase_id]["destroy_time"]["min"] /= num_results
            result_infos[phase_id]["destroy_time"]["avg"] /= num_results
            result_infos[phase_id]["destroy_time"]["std"] /= num_results
            result_infos[phase_id]["begin_time"  ]["max"] /= num_results
            result_infos[phase_id]["begin_time"  ]["min"] /= num_results
            result_infos[phase_id]["begin_time"  ]["avg"] /= num_results
            result_infos[phase_id]["begin_time"  ]["std"] /= num_results
            result_infos[phase_id]["compl_time"  ]["max"] /= num_results
            result_infos[phase_id]["compl_time"  ]["min"] /= num_results
            result_infos[phase_id]["compl_time"  ]["avg"] /= num_results
            result_infos[phase_id]["compl_time"  ]["std"] /= num_results
            result_infos[phase_id]["advance_time"]["max"] /= num_results
            result_infos[phase_id]["advance_time"]["min"] /= num_results
            result_infos[phase_id]["advance_time"]["avg"] /= num_results
            result_infos[phase_id]["advance_time"]["std"] /= num_results
        
        return result
        #return result_infos

def printResults(result):
    for phase_id in result:
        print "%s         max    min    avg    std   "% phase_id
        print "exec        %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["exec"        ]["max"], result[phase_id]["exec"        ]["min"], result[phase_id]["exec"        ]["avg"], result[phase_id]["exec"        ]["std"])
        print "reexec      %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["reexec"      ]["max"], result[phase_id]["reexec"      ]["min"], result[phase_id]["reexec"      ]["avg"], result[phase_id]["reexec"      ]["std"])
        print "prv_copy    %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["prv_copy"    ]["max"], result[phase_id]["prv_copy"    ]["min"], result[phase_id]["prv_copy"    ]["avg"], result[phase_id]["prv_copy"    ]["std"])
        print "prv_ref     %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["prv_ref"     ]["max"], result[phase_id]["prv_ref"     ]["min"], result[phase_id]["prv_ref"     ]["avg"], result[phase_id]["prv_ref"     ]["std"])
        print "restore     %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["restore"     ]["max"], result[phase_id]["restore"     ]["min"], result[phase_id]["restore"     ]["avg"], result[phase_id]["restore"     ]["std"])
        print "msg_log     %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["msg_log"     ]["max"], result[phase_id]["msg_log"     ]["min"], result[phase_id]["msg_log"     ]["avg"], result[phase_id]["msg_log"     ]["std"])
        print "total_time  %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["total_time"  ]["max"], result[phase_id]["total_time"  ]["min"], result[phase_id]["total_time"  ]["avg"], result[phase_id]["total_time"  ]["std"])
        print "reex_time   %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["reex_time"   ]["max"], result[phase_id]["reex_time"   ]["min"], result[phase_id]["reex_time"   ]["avg"], result[phase_id]["reex_time"   ]["std"])
        print "sync_time   %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["sync_time"   ]["max"], result[phase_id]["sync_time"   ]["min"], result[phase_id]["sync_time"   ]["avg"], result[phase_id]["sync_time"   ]["std"])
        print "prv_time    %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["prv_time"    ]["max"], result[phase_id]["prv_time"    ]["min"], result[phase_id]["prv_time"    ]["avg"], result[phase_id]["prv_time"    ]["std"])
        print "rst_time    %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["rst_time"    ]["max"], result[phase_id]["rst_time"    ]["min"], result[phase_id]["rst_time"    ]["avg"], result[phase_id]["rst_time"    ]["std"])
        print "create_time %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["create_time" ]["max"], result[phase_id]["create_time" ]["min"], result[phase_id]["create_time" ]["avg"], result[phase_id]["create_time" ]["std"])
        print "destroy_time %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["destroy_time"]["max"], result[phase_id]["destroy_time"]["min"], result[phase_id]["destroy_time"]["avg"], result[phase_id]["destroy_time"]["std"])
        print "begin_time  %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["begin_time"  ]["max"], result[phase_id]["begin_time"  ]["min"], result[phase_id]["begin_time"  ]["avg"], result[phase_id]["begin_time"  ]["std"])
        print "compl_time  %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["compl_time"  ]["max"], result[phase_id]["compl_time"  ]["min"], result[phase_id]["compl_time"  ]["avg"], result[phase_id]["compl_time"  ]["std"])
        print "advance_time %6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["advance_time"]["max"], result[phase_id]["advance_time"]["min"], result[phase_id]["advance_time"]["avg"], result[phase_id]["advance_time"]["std"])
                                                                                       
                                                                                       
def genEmptyDF(apps, inputs, nTasks, ftype):
    print 'genDF'
    print apps, inputs, nTasks, ftype
    miindex = pd.MultiIndex.from_product([apps,
                                          inputs,
                                          nTasks])
    labels = ['bare', 'noprv', 'errfree', 'total', 'preserve', 'rollback', 'runtime', 'reex time', 'restore', 'comm time', 'presv w/ e', 'presv w/o e', 'CD overhead', 'comm w/ CD' ]
    micolumns = pd.MultiIndex.from_product( [labels] )
#    micolumns = pd.MultiIndex.from_tuples([ ('errFree','avg','bare'), ('errFree','avg','cd_noprv'), 
#      ('errFree','avg','loop'),('errFree','avg','prsv'), ('errFree','avg','comm'),('errFree','avg','rtov'),
#      ('errFree','std','loop'),('errFree','std','prsv'), ('errFree','std','comm'),('errFree','std','rtov'),
#      ('errFree','min','loop'),('errFree','min','prsv'), ('errFree','min','comm'),('errFree','min','rtov'),
#      ('errFree','max','loop'),('errFree','max','prsv'), ('errFree','max','comm'),('errFree','max','rtov'),
#      ('fRate_0','tot','exec'),('fRate_0','tot','prsv'), ('fRate_0','tot','comm'),('fRate_0','tot','rtov'),
#      ('fRate_0','lv0','exec'),('fRate_0','lv0','prsv'), ('fRate_0','lv0','comm'),('fRate_0','lv0','rtov'),
#      ('fRate_0','lv1','exec'),('fRate_0','lv1','prsv'), ('fRate_0','lv1','comm'),('fRate_0','lv1','rtov'),
#      ('fRate_0','lv2','exec'),('fRate_0','lv2','prsv'), ('fRate_0','lv2','comm'),('fRate_0','lv2','rtov'),
#                                          ],
#                                          names=['failure', 'level', 'breakdown'])
    
    dfmi = pd.DataFrame(np.zeros(len(miindex)*len(micolumns)).reshape((len(miindex),len(micolumns))),
                        index=miindex,
                        columns=micolumns).sort_index().sort_index(axis=1)
    return dfmi

def genEmptyDF2(apps, inputs, nTasks, ftype, phase_list):
    print 'genDF'
    print apps, inputs, nTasks, ftype
    miindex = pd.MultiIndex.from_product([apps,
                                          inputs,
                                          nTasks])
#    labels = ['total time', 'runtime', 'preserve', 'restore', 'rollback', 'vol', 'bw', 'prv loc', 'rt loc', 'rt max', 'exec', 'reex']


    labels = ['total time','preserve','runtime','restore','rollback','vol in','vol out','bw','bw real','prsv loc','prsv max','rtov loc','rtov max','exec','reex']
    micolumns = pd.MultiIndex.from_product( [phase_list, labels] )
#    micolumns = pd.MultiIndex.from_tuples([ ('errFree','avg','bare'), ('errFree','avg','cd_noprv'), 
#      ('errFree','avg','loop'),('errFree','avg','prsv'), ('errFree','avg','comm'),('errFree','avg','rtov'),
#      ('errFree','std','loop'),('errFree','std','prsv'), ('errFree','std','comm'),('errFree','std','rtov'),
#      ('errFree','min','loop'),('errFree','min','prsv'), ('errFree','min','comm'),('errFree','min','rtov'),
#      ('errFree','max','loop'),('errFree','max','prsv'), ('errFree','max','comm'),('errFree','max','rtov'),
#      ('fRate_0','tot','exec'),('fRate_0','tot','prsv'), ('fRate_0','tot','comm'),('fRate_0','tot','rtov'),
#      ('fRate_0','lv0','exec'),('fRate_0','lv0','prsv'), ('fRate_0','lv0','comm'),('fRate_0','lv0','rtov'),
#      ('fRate_0','lv1','exec'),('fRate_0','lv1','prsv'), ('fRate_0','lv1','comm'),('fRate_0','lv1','rtov'),
#      ('fRate_0','lv2','exec'),('fRate_0','lv2','prsv'), ('fRate_0','lv2','comm'),('fRate_0','lv2','rtov'),
#                                          ],
#                                          names=['failure', 'level', 'breakdown'])
    
    dfmi = pd.DataFrame(np.zeros(len(miindex)*len(micolumns)).reshape((len(miindex),len(micolumns))),
                        index=miindex,
                        columns=micolumns).sort_index().sort_index(axis=1)
    return dfmi

# Recurse CD hierarchy and make per-level info flattened (as list)
def getLevelInfoFlattened(cd_elem, prof_list):
    #print "-----get Level Info start--------------------------------"
    #print cd_elem
    #print "-----get Level Info start--------------------------------"
    for phase in cd_elem['CD info']:
       elem = cd_elem['CD info'][phase]
       #print elem
       #raw_input('3333333333333333')
       prof_list.extend([ elem["preserve time"], elem["CDrt overhead"], elem["reex_time"]["avg"], elem["rst_time"]["avg"] ])
       #prof_list.extend([ elem['wr_bw'], elem[u'loc prv time'] ])
       #print elem["wr_bw"], elem["loc prv time"]
       #print "%s: %f %f %f %f" % (phase,elem["preserve time"], elem["CDrt overhead"], elem["reex_time"]["avg"], elem["rst_time"]["avg"])
#       raw_input('---------------------------------')

def getEstInfo(result):
    printed_head = False
    header = ['app', 'input', 'tasks']
    per_est_file_info = []
    for app in result:
        for ftype in result[app]:
            for nTask in result[app][ftype]:
                for inputsize in result[app][ftype][nTask]:
                    splitted = re.split(r'_', app)
                    app_type = splitted[1]
                    app_name = splitted[0]
                    if app_type == 'errfree' or app_type == 'bare' or app_type == 'noprv':
                        continue
                    else:
                        #print app, nTask, inputsize
                        #print result[app][ftype][nTask][inputsize]
                        elem = result[app][ftype][nTask][inputsize]#["CD info"]
                        # now we have a CD info for one estimation file
                        # which is dict of phaseID:cd_info
                        if printed_head == False:
                            for phase in elem:
    #                            print 'phase', phase
    #                            print '12234'
    #                            raw_input('0000')
                                header.append(phase)
#                                 header.append(phase)
#                                 header.append(phase)
#                                 header.append(phase)
                            printed_head = True
                        print app, nTask, inputsize, type(elem)#, header
                        #print elem
                        print 'cccccccccccccccccccccccccccccc'
                        prof_list = [app, inputsize, nTask]
                        getLevelInfoFlattened(elem, prof_list)
                        per_est_file_info.append(prof_list)
    ret = [header,per_est_file_info]
    return ret
#                    for lv0 in cd_info: 
#                    result[app][ftype][nTask][inputsize]
                    
#     for cdtree in cdtree_list['CD info'][1:]:
#         cdtree_infos = cdtree
#         for phase_id in cdtree_infos:
#     for lv0 in cd_info: 
#         for lv1 in cd_info[lv0]['child CDs']:
#             for lv2 in cd_info[lv0]['child CDs'][lv1]['child CDs']:
#         flattened[cds] = {}
#         for item in cds_list[cds]:
#             #print item
#             if item != 'child CDs':
#                 flattened[cds][item] = cds_list[cds][item]
#             else:
#                 makeFlatCDs(flattened, cds_list[cds][item])

def checkMissing(result, app_bare, app_nopv, app_noer, ftype, nTask, inputsize):
    missing = False
    missing |= (result.get(app_bare) == None)
    missing |= (result.get(app_nopv) == None)
    missing |= (result.get(app_noer) == None)
    if missing == False:
        missing |= result[app_bare].get(ftype) == None
        missing |= result[app_nopv].get(ftype) == None
        missing |= result[app_noer].get(ftype) == None
        if missing == False:
            missing |= result[app_bare][ftype].get(nTask) == None
            missing |= result[app_nopv][ftype].get(nTask) == None
            missing |= result[app_noer][ftype].get(nTask) == None
            if missing == False:
                missing |= result[app_bare][ftype][nTask].get(inputsize) == None
                missing |= result[app_nopv][ftype][nTask].get(inputsize) == None
                missing |= result[app_noer][ftype][nTask].get(inputsize) == None
    return missing


# Write each elem info in CSV format                
def writeCSV(est_info, csv_filename):
    csvf = csv.writer(open(csv_filename, "wb"))
    csvf.writerow(est_info[0])
    for row in est_info[1]:
        csvf.writerow(row)

#    #print flattened
#    f = csv.writer(open(csv_filename, "wb+"))
#    f.writerow(["app", "size","task","bare","noprv","errfree","error","preserve","rollback","runtime","prsv errfree","prsv w/error", "rtov errfree", "rtov w/error", "rollback w/error"])
#
#def writeCSV(elemcsv, result, cd_info, app, inputsize, nTask):
#
#    elemcsv.writerow([app, inputsize, nTask, ])
#    elemcsv.writerow(['GPUs / Node'               , gpus_per_node             ,])
#    elemcsv.writerow(['Nodes / Blade'             , nodes_per_blade           ,])
#    elemcsv.writerow(['Blades / Cage'             , blades_per_cage           ,])
#    elemcsv.writerow(['Cages / Cabinet'           , cages_per_cabinet         ,])
#    elemcsv.writerow(['Cabinets / System'         , cabinets_per_system       ,])
#    elemcsv.writerow(['GPU Cores / GPU'           , gpu_cores_per_gpu         ,])
#    elemcsv.writerow(['GPU Clock Frequency'       , gpu_clock_freq            ,])
#    elemcsv.writerow(['GPU TFLOPS / GPU'          , gpu_tflops_per_gpu        ,])
#    elemcsv.writerow(['GPU Memory stacks / GPU'   , gpu_mem_stacks_per_gpu    ,])
#    elemcsv.writerow(['GPU Memory chips / stack'  , gpu_mem_chips_per_stack   ,])
#    elemcsv.writerow(['GPU Memory capacity / chip', gpu_mem_capacity_per_chip ,])
#    elemcsv.writerow(['GPU Memory IO clock (GHz)' , gpu_mem_io_clk            ,])
#    elemcsv.writerow(['GPU Memory IO width / ch'  , gpu_mem_io_width_per_ch   ,])
#    elemcsv.writerow(['GPU Memory channels'       , gpu_mem_channels          ,])
#    elemcsv.writerow(['CPU Cores / CPU'           , cpu_cores_per_cpu         ,])
#    elemcsv.writerow(['CPU Clock Frequency'       , cpu_clk_freq              ,])
#    if len(cdtree_list['CD info']) == 1:
#        result = {}
#        result['CD info'] = cdtree_list['CD info'][0]
#        result["total time"    ]   = cdtree_list["total time"    ][0]
#        result["CD overhead"   ]   = cdtree_list["CD overhead"   ][0]
#        result["sync time exec"]   = cdtree_list["sync time exec"][0]
#        result["sync time reex"]   = cdtree_list["sync time reex"][0]
#        result["sync time recr"]   = cdtree_list["sync time recr"][0]
#        result["preserve time" ]   = cdtree_list["preserve time" ][0]
#        result["restore time"  ]   = cdtree_list["restore time"  ][0]
#        result["create time"   ]   = cdtree_list["create time"   ][0]
#        result["destory time"  ]   = cdtree_list["destory time"  ][0]
#        result["begin time"    ]   = cdtree_list["begin time"    ][0]
#        result["complete time" ]   = cdtree_list["complete time" ][0]
#        result[message_time  ]   = cdtree_list[message_time  ][0]
#        result["libc logging"  ]   = cdtree_list["libc logging"  ][0]
#        result["mailbox overhead"] = cdtree_list["mailbox overhead"][0]
#        return result
#    else:
#        result = {}
#        result['CD info'] = cdtree_list['CD info'][0]
#        result["total time"    ]   = np.mean(cdtree_list["total time"    ]  )
#        result["CD overhead"   ]   = np.mean(cdtree_list["CD overhead"   ]  )
#        result["sync time exec"]   = np.mean(cdtree_list["sync time exec"]  )
#        result["sync time reex"]   = np.mean(cdtree_list["sync time reex"]  )
#        result["sync time recr"]   = np.mean(cdtree_list["sync time recr"]  )
#        result["preserve time" ]   = np.mean(cdtree_list["preserve time" ]  )
#        result["restore time"  ]   = np.mean(cdtree_list["restore time"  ]  )
#        result["create time"   ]   = np.mean(cdtree_list["create time"   ]  )
#        result["destory time"  ]   = np.mean(cdtree_list["destory time"  ]  )
#        result["begin time"    ]   = np.mean(cdtree_list["begin time"    ]  )
#        result["complete time" ]   = np.mean(cdtree_list["complete time" ]  )
#        result[message_time  ]   = np.mean(cdtree_list[message_time  ]  )
#        result["libc logging"  ]   = np.mean(cdtree_list["libc logging"  ]  )
#        result["mailbox overhead"] = np.mean(cdtree_list["mailbox overhead"])
#        result_infos = result['CD info']
#        for cdtree in cdtree_list['CD info'][1:]:
#            cdtree_infos = cdtree
#            for phase_id in cdtree_infos:
##                print "phase : ", phase_id, len(cdtree_list)
##                raw_input("@@@@")
#                result_infos[phase_id]["execution time"] += float(cdtree_infos[phase_id]["execution time"] )
#                result_infos[phase_id]["current counts"] += float(cdtree_infos[phase_id]["current counts"] )
#                result_infos[phase_id]["input volume"]   += float(cdtree_infos[phase_id]["input volume"]   )
#                result_infos[phase_id]["output volume"]  += float(cdtree_infos[phase_id]["output volume"]  )
#                result_infos[phase_id]["rd_bw"]          += float(cdtree_infos[phase_id]["rd_bw"]          )






# def averageCDTree(cdtree_list, trace_infos):
# 
#     #print 'now start\n\n'
#     print 'cdtree_list type:', type(cdtree_list)
#     #print 'now start'
#     if len(cdtree_list['CD info']) == 1:
#         result = {}
#         result['CD info']          = copy.deepcopy(cdtree_list['CD info'][0])
#         result["total time"    ]   = copy.deepcopy(cdtree_list["total time"    ][0])
#         result["reex time"     ]   = copy.deepcopy(cdtree_list["reex time"     ][0])
#         result["CD overhead"   ]   = copy.deepcopy(cdtree_list["CD overhead"   ][0])
#         result["sync time exec"]   = copy.deepcopy(cdtree_list["sync time exec"][0])
#         result["sync time reex"]   = copy.deepcopy(cdtree_list["sync time reex"][0])
#         result["sync time recr"]   = copy.deepcopy(cdtree_list["sync time recr"][0])
#         result["preserve time" ]   = copy.deepcopy(cdtree_list["preserve time" ][0])
#         result["restore time"  ]   = copy.deepcopy(cdtree_list["restore time"  ][0])
#         result["create time"   ]   = copy.deepcopy(cdtree_list["create time"   ][0])
#         result["destory time"  ]   = copy.deepcopy(cdtree_list["destory time"  ][0])
#         result["begin time"    ]   = copy.deepcopy(cdtree_list["begin time"    ][0])
#         result["complete time" ]   = copy.deepcopy(cdtree_list["complete time" ][0])
#         result[message_time    ]   = copy.deepcopy(cdtree_list[message_time    ][0])
#         result["libc logging"  ]   = copy.deepcopy(cdtree_list["libc logging"  ][0])
#         result["mailbox overhead"] = copy.deepcopy(cdtree_list["mailbox overhead"][0])
#         return result
#     else:
#         result = {}
#         result['CD info']          = copy.deepcopy(cdtree_list['CD info'][0])
#         result["total time"    ]   = copy.deepcopy(np.mean(cdtree_list["total time"    ]  ))
#         result["reex time"     ]   = copy.deepcopy(np.mean(cdtree_list["reex time"     ]  ))
#         result["CD overhead"   ]   = copy.deepcopy(np.mean(cdtree_list["CD overhead"   ]  ))
#         result["sync time exec"]   = copy.deepcopy(np.mean(cdtree_list["sync time exec"]  ))
#         result["sync time reex"]   = copy.deepcopy(np.mean(cdtree_list["sync time reex"]  ))
#         result["sync time recr"]   = copy.deepcopy(np.mean(cdtree_list["sync time recr"]  ))
#         result["preserve time" ]   = copy.deepcopy(np.mean(cdtree_list["preserve time" ]  ))
#         result["restore time"  ]   = copy.deepcopy(np.mean(cdtree_list["restore time"  ]  ))
#         result["create time"   ]   = copy.deepcopy(np.mean(cdtree_list["create time"   ]  ))
#         result["destory time"  ]   = copy.deepcopy(np.mean(cdtree_list["destory time"  ]  ))
#         result["begin time"    ]   = copy.deepcopy(np.mean(cdtree_list["begin time"    ]  ))
#         result["complete time" ]   = copy.deepcopy(np.mean(cdtree_list["complete time" ]  ))
#         result[message_time    ]   = copy.deepcopy(np.mean(cdtree_list[message_time    ]  ))
#         result["libc logging"  ]   = copy.deepcopy(np.mean(cdtree_list["libc logging"  ]  ))
#         result["mailbox overhead"] = copy.deepcopy(np.mean(cdtree_list["mailbox overhead"]))
#         result_infos = result['CD info']
#         print 'result_infos:', type(result_infos)
#         raw_input('1111111111111111111')
#         num_meas = len(result['CD info'])
#         if len(result_infos) == 0:
#             raise SystemExit
#             return result
#         else:
#             print 'extract info'
#             trace_infos = copy.deepcopy(result['CD info'])
#             extractLatencies(trace_infos)
#             removeLatencies(result_infos)
#             for item in result_infos:
#                 print "info:", item, len(result_infos[item])
#                 print "\n"
#                 for jtem in result_infos[item]:
#                     #print "detail:", result_infos[item]#, len(result_infos[item][jtem])
#                     if type(result_infos[item][jtem]) is list or type(result_infos[item][jtem]) is dict:
#                         print "detail:", item
#                         for ktem in result_infos[item][jtem]:
#                             print item, jtem, ktem #result_infos[item]#, len(result_infos[item][jtem])
#                     else:
#                         print "detail:", jtem, result_infos[item][jtem]
#                 raw_input('extract info done')
#             print 'trace'
#             for item in trace_infos:
#                 print "trace:", item, len(trace_infos[item])
#                 print "\n"
#                 for jtem in trace_infos[item]:
# #                    for ktem in trace_infos[item][jtem]:
# #                        print "jnfo:", ktem#trace_infos[item]#, len(trace_infos[item][jtem])
#                     if type(trace_infos[item][jtem]) is list or type(trace_infos[item][jtem]) is dict:
#                         print "trace detail:", item
#                         for ktem in trace_infos[item][jtem]:
#                             print 'len:', item, jtem, len(trace_infos[item][jtem]) #trace_infos[item]#, len(trace_infos[item][jtem])
#                     else:
#                         print "detail:", jtem, trace_infos[item][jtem]
#                 raw_input('trace done')
#             print 'done'
#         for cdtree in cdtree_list['CD info'][1:]:
#             cdtree_infos = cdtree
#             for phase_id in cdtree_infos:
#                 if phase_id == "CD_1_1" and cdtree_infos[phase_id]["tasksize"] == 1000 and cdtree_infos[phase_id]["label"] == "MainLoop":
#                     print "prv time: ", cdtree_infos[phase_id]["total preserve"], result_infos[phase_id]["total preserve"]
# #                    raw_input("!!!!!!!!!!!!")
# 
#                 print "phase : ", phase_id, len(cdtree_list)
#                 if phase_id not in result_infos:
# 
#                     print "phase is not here: ", len(result_infos), len(result['CD info']), len(cdtree_list['CD info'][0])
#                     for phases in result_infos:
#                         print phases
# 
# #                raw_input("@@@@")
#                 result_infos[phase_id]["execution time"] += float(cdtree_infos[phase_id]["execution time"] )
#                 result_infos[phase_id]["current counts"] += float(cdtree_infos[phase_id]["current counts"] )
#                 result_infos[phase_id]["input volume"]   += float(cdtree_infos[phase_id]["input volume"]   )
#                 result_infos[phase_id]["output volume"]  += float(cdtree_infos[phase_id]["output volume"]  )
#                 result_infos[phase_id]["rd_bw"]          += float(cdtree_infos[phase_id]["rd_bw"]          )
#                 result_infos[phase_id]["wr_bw"]          += float(cdtree_infos[phase_id]["wr_bw"]          )
#                 result_infos[phase_id]["rd_bw_mea"]      += float(cdtree_infos[phase_id]["rd_bw_mea"]      )
#                 result_infos[phase_id]["wr_bw_mea"]      += float(cdtree_infos[phase_id]["wr_bw_mea"]      )
#                 result_infos[phase_id]["fault rate"]     += float(cdtree_infos[phase_id]["fault rate"]     )
#                 result_infos[phase_id]["tasksize"]      += float(cdtree_infos[phase_id]["tasksize"]      )
#                 result_infos[phase_id]["max prv only bw"]+= float(cdtree_infos[phase_id]["max prv only bw"])
#                 result_infos[phase_id]["loc prv only bw"]+= float(cdtree_infos[phase_id]["loc prv only bw"])
#                 result_infos[phase_id]["loc prv time"]   += float(cdtree_infos[phase_id]["loc prv time"]   )
#                 result_infos[phase_id]["CDrt overhead"]  += float(cdtree_infos[phase_id]["CDrt overhead"]  )
#                 result_infos[phase_id]["total preserve"]  += float(cdtree_infos[phase_id]["total preserve"]  )
#                 result_infos[phase_id]["preserve time"]  += float(cdtree_infos[phase_id]["preserve time"]  )
#                 result_infos[phase_id]["exec"        ]["max"] += float(cdtree_infos[phase_id]["exec"        ]["max"])
#                 result_infos[phase_id]["exec"        ]["min"] += float(cdtree_infos[phase_id]["exec"        ]["min"])
#                 result_infos[phase_id]["exec"        ]["avg"] += float(cdtree_infos[phase_id]["exec"        ]["avg"])
#                 result_infos[phase_id]["exec"        ]["std"] += float(cdtree_infos[phase_id]["exec"        ]["std"])
#                 result_infos[phase_id]["reexec"      ]["max"] += float(cdtree_infos[phase_id]["reexec"      ]["max"])
#                 result_infos[phase_id]["reexec"      ]["min"] += float(cdtree_infos[phase_id]["reexec"      ]["min"])
#                 result_infos[phase_id]["reexec"      ]["avg"] += float(cdtree_infos[phase_id]["reexec"      ]["avg"])
#                 result_infos[phase_id]["reexec"      ]["std"] += float(cdtree_infos[phase_id]["reexec"      ]["std"])
#                 result_infos[phase_id]["prv_copy"    ]["max"] += float(cdtree_infos[phase_id]["prv_copy"    ]["max"])
#                 result_infos[phase_id]["prv_copy"    ]["min"] += float(cdtree_infos[phase_id]["prv_copy"    ]["min"])
#                 result_infos[phase_id]["prv_copy"    ]["avg"] += float(cdtree_infos[phase_id]["prv_copy"    ]["avg"])
#                 result_infos[phase_id]["prv_copy"    ]["std"] += float(cdtree_infos[phase_id]["prv_copy"    ]["std"])
#                 result_infos[phase_id]["prv_ref"     ]["max"] += float(cdtree_infos[phase_id]["prv_ref"     ]["max"])
#                 result_infos[phase_id]["prv_ref"     ]["min"] += float(cdtree_infos[phase_id]["prv_ref"     ]["min"])
#                 result_infos[phase_id]["prv_ref"     ]["avg"] += float(cdtree_infos[phase_id]["prv_ref"     ]["avg"])
#                 result_infos[phase_id]["prv_ref"     ]["std"] += float(cdtree_infos[phase_id]["prv_ref"     ]["std"])
#                 result_infos[phase_id]["restore"     ]["max"] += float(cdtree_infos[phase_id]["restore"     ]["max"])
#                 result_infos[phase_id]["restore"     ]["min"] += float(cdtree_infos[phase_id]["restore"     ]["min"])
#                 result_infos[phase_id]["restore"     ]["avg"] += float(cdtree_infos[phase_id]["restore"     ]["avg"])
#                 result_infos[phase_id]["restore"     ]["std"] += float(cdtree_infos[phase_id]["restore"     ]["std"])
#                 result_infos[phase_id]["msg_log"     ]["max"] += float(cdtree_infos[phase_id]["msg_log"     ]["max"])
#                 result_infos[phase_id]["msg_log"     ]["min"] += float(cdtree_infos[phase_id]["msg_log"     ]["min"])
#                 result_infos[phase_id]["msg_log"     ]["avg"] += float(cdtree_infos[phase_id]["msg_log"     ]["avg"])
#                 result_infos[phase_id]["msg_log"     ]["std"] += float(cdtree_infos[phase_id]["msg_log"     ]["std"])
#                 result_infos[phase_id]["total_time"  ]["max"] += float(cdtree_infos[phase_id]["total_time"  ]["max"])
#                 result_infos[phase_id]["total_time"  ]["min"] += float(cdtree_infos[phase_id]["total_time"  ]["min"])
#                 result_infos[phase_id]["total_time"  ]["avg"] += float(cdtree_infos[phase_id]["total_time"  ]["avg"])
#                 result_infos[phase_id]["total_time"  ]["std"] += float(cdtree_infos[phase_id]["total_time"  ]["std"])
#                 result_infos[phase_id]["reex_time"   ]["max"] += float(cdtree_infos[phase_id]["reex_time"   ]["max"])
#                 result_infos[phase_id]["reex_time"   ]["min"] += float(cdtree_infos[phase_id]["reex_time"   ]["min"])
#                 result_infos[phase_id]["reex_time"   ]["avg"] += float(cdtree_infos[phase_id]["reex_time"   ]["avg"])
#                 result_infos[phase_id]["reex_time"   ]["std"] += float(cdtree_infos[phase_id]["reex_time"   ]["std"])
#                 result_infos[phase_id]["sync_time"   ]["max"] += float(cdtree_infos[phase_id]["sync_time"   ]["max"])
#                 result_infos[phase_id]["sync_time"   ]["min"] += float(cdtree_infos[phase_id]["sync_time"   ]["min"])
#                 result_infos[phase_id]["sync_time"   ]["avg"] += float(cdtree_infos[phase_id]["sync_time"   ]["avg"])
#                 result_infos[phase_id]["sync_time"   ]["std"] += float(cdtree_infos[phase_id]["sync_time"   ]["std"])
#                 result_infos[phase_id]["prv_time"    ]["max"] += float(cdtree_infos[phase_id]["prv_time"    ]["max"])
#                 result_infos[phase_id]["prv_time"    ]["min"] += float(cdtree_infos[phase_id]["prv_time"    ]["min"])
#                 result_infos[phase_id]["prv_time"    ]["avg"] += float(cdtree_infos[phase_id]["prv_time"    ]["avg"])
#                 result_infos[phase_id]["prv_time"    ]["std"] += float(cdtree_infos[phase_id]["prv_time"    ]["std"])
#                 result_infos[phase_id]["rst_time"    ]["max"] += float(cdtree_infos[phase_id]["rst_time"    ]["max"])
#                 result_infos[phase_id]["rst_time"    ]["min"] += float(cdtree_infos[phase_id]["rst_time"    ]["min"])
#                 result_infos[phase_id]["rst_time"    ]["avg"] += float(cdtree_infos[phase_id]["rst_time"    ]["avg"])
#                 result_infos[phase_id]["rst_time"    ]["std"] += float(cdtree_infos[phase_id]["rst_time"    ]["std"])
#                 result_infos[phase_id]["create_time" ]["max"] += float(cdtree_infos[phase_id]["create_time" ]["max"])
#                 result_infos[phase_id]["create_time" ]["min"] += float(cdtree_infos[phase_id]["create_time" ]["min"])
#                 result_infos[phase_id]["create_time" ]["avg"] += float(cdtree_infos[phase_id]["create_time" ]["avg"])
#                 result_infos[phase_id]["create_time" ]["std"] += float(cdtree_infos[phase_id]["create_time" ]["std"])
#                 result_infos[phase_id]["destroy_time"]["max"] += float(cdtree_infos[phase_id]["destroy_time"]["max"])
#                 result_infos[phase_id]["destroy_time"]["min"] += float(cdtree_infos[phase_id]["destroy_time"]["min"])
#                 result_infos[phase_id]["destroy_time"]["avg"] += float(cdtree_infos[phase_id]["destroy_time"]["avg"])
#                 result_infos[phase_id]["destroy_time"]["std"] += float(cdtree_infos[phase_id]["destroy_time"]["std"])
#                 result_infos[phase_id]["begin_time"  ]["max"] += float(cdtree_infos[phase_id]["begin_time"  ]["max"])
#                 result_infos[phase_id]["begin_time"  ]["min"] += float(cdtree_infos[phase_id]["begin_time"  ]["min"])
#                 result_infos[phase_id]["begin_time"  ]["avg"] += float(cdtree_infos[phase_id]["begin_time"  ]["avg"])
#                 result_infos[phase_id]["begin_time"  ]["std"] += float(cdtree_infos[phase_id]["begin_time"  ]["std"])
#                 result_infos[phase_id]["compl_time"  ]["max"] += float(cdtree_infos[phase_id]["compl_time"  ]["max"])
#                 result_infos[phase_id]["compl_time"  ]["min"] += float(cdtree_infos[phase_id]["compl_time"  ]["min"])
#                 result_infos[phase_id]["compl_time"  ]["avg"] += float(cdtree_infos[phase_id]["compl_time"  ]["avg"])
#                 result_infos[phase_id]["compl_time"  ]["std"] += float(cdtree_infos[phase_id]["compl_time"  ]["std"])
#                 result_infos[phase_id]["advance_time"]["max"] += float(cdtree_infos[phase_id]["advance_time"]["max"])
#                 result_infos[phase_id]["advance_time"]["min"] += float(cdtree_infos[phase_id]["advance_time"]["min"])
#                 result_infos[phase_id]["advance_time"]["avg"] += float(cdtree_infos[phase_id]["advance_time"]["avg"])
#                 result_infos[phase_id]["advance_time"]["std"] += float(cdtree_infos[phase_id]["advance_time"]["std"])
# 
#                 trace_infos[phase_id]["exec_trace"].extend(cdtree_infos[phase_id]["exec_trace"])
#                 trace_infos[phase_id]["cdrt_trace"].extend(cdtree_infos[phase_id]["cdrt_trace"])
#                 trace_infos[phase_id]["prsv_trace"].extend(cdtree_infos[phase_id]["prsv_trace"])
#                 trace_infos[phase_id]["max_prsv"].extend(cdtree_infos[phase_id]["max_prsv"])
#                 trace_infos[phase_id]["max_cdrt"].extend(cdtree_infos[phase_id]["max_cdrt"])
#         num_results = len(cdtree_list['CD info'])
#         print 'num results:', num_results
#     
#         for phase_id in result_infos:
#             result_infos[phase_id]["execution time"] /= num_results
#             result_infos[phase_id]["current counts"] /= num_results
#             result_infos[phase_id]["input volume"]   /= num_results
#             result_infos[phase_id]["output volume"]  /= num_results
#             result_infos[phase_id]["rd_bw"]          /= num_results
#             result_infos[phase_id]["wr_bw"]          /= num_results
#             result_infos[phase_id]["rd_bw_mea"]      /= num_results
#             result_infos[phase_id]["wr_bw_mea"]      /= num_results
#             result_infos[phase_id]["fault rate"]     /= num_results
#             result_infos[phase_id]["tasksize"]      /= num_results
#             result_infos[phase_id]["max prv only bw"]/= num_results
#             result_infos[phase_id]["loc prv only bw"]/= num_results
#             result_infos[phase_id]["loc prv time"]   /= num_results
#             result_infos[phase_id]["CDrt overhead"]  /= num_results
#             result_infos[phase_id]["total preserve"]  /= num_results
#             result_infos[phase_id]["preserve time"]  /= num_results
#             result_infos[phase_id]["exec"        ]["max"] /= num_results
#             result_infos[phase_id]["exec"        ]["min"] /= num_results
#             result_infos[phase_id]["exec"        ]["avg"] /= num_results
#             result_infos[phase_id]["exec"        ]["std"] /= num_results
#             result_infos[phase_id]["reexec"      ]["max"] /= num_results
#             result_infos[phase_id]["reexec"      ]["min"] /= num_results
#             result_infos[phase_id]["reexec"      ]["avg"] /= num_results
#             result_infos[phase_id]["reexec"      ]["std"] /= num_results
#             result_infos[phase_id]["prv_copy"    ]["max"] /= num_results
#             result_infos[phase_id]["prv_copy"    ]["min"] /= num_results
#             result_infos[phase_id]["prv_copy"    ]["avg"] /= num_results
#             result_infos[phase_id]["prv_copy"    ]["std"] /= num_results
#             result_infos[phase_id]["prv_ref"     ]["max"] /= num_results
#             result_infos[phase_id]["prv_ref"     ]["min"] /= num_results
#             result_infos[phase_id]["prv_ref"     ]["avg"] /= num_results
#             result_infos[phase_id]["prv_ref"     ]["std"] /= num_results
#             result_infos[phase_id]["restore"     ]["max"] /= num_results
#             result_infos[phase_id]["restore"     ]["min"] /= num_results
#             result_infos[phase_id]["restore"     ]["avg"] /= num_results
#             result_infos[phase_id]["restore"     ]["std"] /= num_results
#             result_infos[phase_id]["msg_log"     ]["max"] /= num_results
#             result_infos[phase_id]["msg_log"     ]["min"] /= num_results
#             result_infos[phase_id]["msg_log"     ]["avg"] /= num_results
#             result_infos[phase_id]["msg_log"     ]["std"] /= num_results
#             result_infos[phase_id]["total_time"  ]["max"] /= num_results
#             result_infos[phase_id]["total_time"  ]["min"] /= num_results
#             result_infos[phase_id]["total_time"  ]["avg"] /= num_results
#             result_infos[phase_id]["total_time"  ]["std"] /= num_results
#             result_infos[phase_id]["reex_time"   ]["max"] /= num_results
#             result_infos[phase_id]["reex_time"   ]["min"] /= num_results
#             result_infos[phase_id]["reex_time"   ]["avg"] /= num_results
#             result_infos[phase_id]["reex_time"   ]["std"] /= num_results
#             result_infos[phase_id]["sync_time"   ]["max"] /= num_results
#             result_infos[phase_id]["sync_time"   ]["min"] /= num_results
#             result_infos[phase_id]["sync_time"   ]["avg"] /= num_results
#             result_infos[phase_id]["sync_time"   ]["std"] /= num_results
#             result_infos[phase_id]["prv_time"    ]["max"] /= num_results
#             result_infos[phase_id]["prv_time"    ]["min"] /= num_results
#             result_infos[phase_id]["prv_time"    ]["avg"] /= num_results
#             result_infos[phase_id]["prv_time"    ]["std"] /= num_results
#             result_infos[phase_id]["rst_time"    ]["max"] /= num_results
#             result_infos[phase_id]["rst_time"    ]["min"] /= num_results
#             result_infos[phase_id]["rst_time"    ]["avg"] /= num_results
#             result_infos[phase_id]["rst_time"    ]["std"] /= num_results
#             result_infos[phase_id]["create_time" ]["max"] /= num_results
#             result_infos[phase_id]["create_time" ]["min"] /= num_results
#             result_infos[phase_id]["create_time" ]["avg"] /= num_results
#             result_infos[phase_id]["create_time" ]["std"] /= num_results
#             result_infos[phase_id]["destroy_time"]["max"] /= num_results
#             result_infos[phase_id]["destroy_time"]["min"] /= num_results
#             result_infos[phase_id]["destroy_time"]["avg"] /= num_results
#             result_infos[phase_id]["destroy_time"]["std"] /= num_results
#             result_infos[phase_id]["begin_time"  ]["max"] /= num_results
#             result_infos[phase_id]["begin_time"  ]["min"] /= num_results
#             result_infos[phase_id]["begin_time"  ]["avg"] /= num_results
#             result_infos[phase_id]["begin_time"  ]["std"] /= num_results
#             result_infos[phase_id]["compl_time"  ]["max"] /= num_results
#             result_infos[phase_id]["compl_time"  ]["min"] /= num_results
#             result_infos[phase_id]["compl_time"  ]["avg"] /= num_results
#             result_infos[phase_id]["compl_time"  ]["std"] /= num_results
#             result_infos[phase_id]["advance_time"]["max"] /= num_results
#             result_infos[phase_id]["advance_time"]["min"] /= num_results
#             result_infos[phase_id]["advance_time"]["avg"] /= num_results
#             result_infos[phase_id]["advance_time"]["std"] /= num_results
#         
#         return result
#         #return result_infos

