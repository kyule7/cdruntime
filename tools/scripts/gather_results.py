#!/usr/bin/python
import json
import pandas as pd
import numpy as np
import re
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

def makeFlatCDs(flattened, cds_list):
    for cds in cds_list: # list of sequential CDs
        flattened[cds] = {}
        for item in cds_list[cds]:
            #print item
            if item != 'ChildCDs':
                flattened[cds][item] = cds_list[cds][item]
            else:
                makeFlatCDs(flattened, cds_list[cds][item])
    #print flattened

def gatherJSONObj(filelist):
    newfile = open(filelist, 'r')
    for line in newfile:
        file_list = line.split(" ")

    print file_list
    gathered = {}
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
            exec_name = eachjson['exec_name']
            fail_type = eachjson['ftype'    ]
            numTasks  = eachjson['numTasks' ]
            input_size= eachjson['input'    ]
            cd_info   = eachjson['CD info'  ]
            print '=======================\n'
            print exec_name, fail_type, numTasks, input_size
#            df = pd.DataFrame.from_dict(cd_info, orient='index')
#            print cd_info
#            df.reset_index(level=0, inplace=True)
            #print df
            print '=======================\n'
#            print cd_info
#            if gathered[each_name][fail_type][numTasks][input_size] 
            if exec_name not in gathered:
                gathered[exec_name] = {}
            if fail_type not in gathered[exec_name]:
                gathered[exec_name][fail_type] = {}
            if numTasks not in gathered[exec_name][fail_type]:
                gathered[exec_name][fail_type][numTasks] = {}
            if input_size not in gathered[exec_name][fail_type][numTasks]:
                gathered[exec_name][fail_type][numTasks][input_size] = {}
                gathered[exec_name][fail_type][numTasks][input_size]["total time"    ]   = []
                gathered[exec_name][fail_type][numTasks][input_size]["CD overhead"   ]   = []
                gathered[exec_name][fail_type][numTasks][input_size]["sync time exec"]   = []
                gathered[exec_name][fail_type][numTasks][input_size]["sync time reex"]   = []
                gathered[exec_name][fail_type][numTasks][input_size]["sync time recr"]   = []
                gathered[exec_name][fail_type][numTasks][input_size]["preserve time" ]   = []
                gathered[exec_name][fail_type][numTasks][input_size]["restore time"  ]   = []
                gathered[exec_name][fail_type][numTasks][input_size]["create time"   ]   = []
                gathered[exec_name][fail_type][numTasks][input_size]["destory time"  ]   = []
                gathered[exec_name][fail_type][numTasks][input_size]["begin time"    ]   = []
                gathered[exec_name][fail_type][numTasks][input_size]["complete time" ]   = []
                gathered[exec_name][fail_type][numTasks][input_size]["mesg logging"  ]   = []
                gathered[exec_name][fail_type][numTasks][input_size]["libc logging"  ]   = []
                gathered[exec_name][fail_type][numTasks][input_size]["mailbox overhead"] = []
                gathered[exec_name][fail_type][numTasks][input_size]['CD info'] = []
#            gathered[exec_name][fail_type][numTasks][input_size].append(cd_info)
#            print cd_info
#            df = pd.DataFrame.from_dict(cd_info, orient='index')
#            print '=======================\n'
#            print df
            flattened = {}
#            for cds in cd_info:
#                flattened[cds] = {}
#                for item in cd_info[cds]:
#                    print item
#                    if item != 'ChildCDs':
#                        flattened[cds][item] = cd_info[cds][item]
            
            makeFlatCDs(flattened, cd_info)
#            print '~!~!~!~!~!~!~!~!'
#            print exec_name, fail_type, numTasks, input_size
#            print '~!~!~!~!~!~!~!~!'
#            print "total time"    , eachjson["total time"    ][0]
#            print "CD overhead"   , eachjson["CD overhead"   ][0]
#            print "sync time exec", eachjson["sync time exec"][0]
#            print "sync time reex", eachjson["sync time reex"][0]
#            print "sync time recr", eachjson["sync time recr"][0]
#            print "preserve time" , eachjson["preserve time" ][0]
#            print "restore time"  , eachjson["restore time"  ][0]
#            print "create time"   , eachjson["create time"   ][0]
#            print "destory time"  , eachjson["destory time"  ][0]
#            print "begin time"    , eachjson["begin time"    ][0]
#            print "complete time" , eachjson["complete time" ][0]
#            print "mesg logging"  , eachjson["mesg logging"  ][0]
#            print "libc logging"  , eachjson["libc logging"  ][0]
#            raw_input('..............................')

            gathered[exec_name][fail_type][numTasks][input_size]["total time"    ].append(eachjson["total time"    ][0])
            gathered[exec_name][fail_type][numTasks][input_size]["CD overhead"   ].append(eachjson["CD overhead"   ][0])
            gathered[exec_name][fail_type][numTasks][input_size]["sync time exec"].append(eachjson["sync time exec"][0])
            gathered[exec_name][fail_type][numTasks][input_size]["sync time reex"].append(eachjson["sync time reex"][0])
            gathered[exec_name][fail_type][numTasks][input_size]["sync time recr"].append(eachjson["sync time recr"][0])
            gathered[exec_name][fail_type][numTasks][input_size]["preserve time" ].append(eachjson["preserve time" ][0])
            gathered[exec_name][fail_type][numTasks][input_size]["restore time"  ].append(eachjson["restore time"  ][0])
            gathered[exec_name][fail_type][numTasks][input_size]["create time"   ].append(eachjson["create time"   ][0])
            gathered[exec_name][fail_type][numTasks][input_size]["destory time"  ].append(eachjson["destory time"  ][0])
            gathered[exec_name][fail_type][numTasks][input_size]["begin time"    ].append(eachjson["begin time"    ][0])
            gathered[exec_name][fail_type][numTasks][input_size]["complete time" ].append(eachjson["complete time" ][0])
            gathered[exec_name][fail_type][numTasks][input_size]["mesg logging"  ].append(eachjson["mesg logging"  ][0])
            gathered[exec_name][fail_type][numTasks][input_size]["libc logging"  ].append(eachjson["libc logging"  ][0])
            gathered[exec_name][fail_type][numTasks][input_size]["mailbox overhead"].append(eachjson["mailbox overhead"])
            gathered[exec_name][fail_type][numTasks][input_size]['CD info'].append(flattened)
    return gathered

def averageCDTree(cdtree_list):
#    print cdtree_list
    if len(cdtree_list['CD info']) == 1:
        result = {}
        result['CD info'] = cdtree_list['CD info'][0]
        result["total time"    ]   = cdtree_list["total time"    ][0]
        result["CD overhead"   ]   = cdtree_list["CD overhead"   ][0]
        result["sync time exec"]   = cdtree_list["sync time exec"][0]
        result["sync time reex"]   = cdtree_list["sync time reex"][0]
        result["sync time recr"]   = cdtree_list["sync time recr"][0]
        result["preserve time" ]   = cdtree_list["preserve time" ][0]
        result["restore time"  ]   = cdtree_list["restore time"  ][0]
        result["create time"   ]   = cdtree_list["create time"   ][0]
        result["destory time"  ]   = cdtree_list["destory time"  ][0]
        result["begin time"    ]   = cdtree_list["begin time"    ][0]
        result["complete time" ]   = cdtree_list["complete time" ][0]
        result["mesg logging"  ]   = cdtree_list["mesg logging"  ][0]
        result["libc logging"  ]   = cdtree_list["libc logging"  ][0]
        result["mailbox overhead"] = cdtree_list["mailbox overhead"][0]
        return result
    else:
        result = {}
        result['CD info'] = cdtree_list['CD info'][0]
        result["total time"    ]   = np.mean(cdtree_list["total time"    ]  )
        result["CD overhead"   ]   = np.mean(cdtree_list["CD overhead"   ]  )
        result["sync time exec"]   = np.mean(cdtree_list["sync time exec"]  )
        result["sync time reex"]   = np.mean(cdtree_list["sync time reex"]  )
        result["sync time recr"]   = np.mean(cdtree_list["sync time recr"]  )
        result["preserve time" ]   = np.mean(cdtree_list["preserve time" ]  )
        result["restore time"  ]   = np.mean(cdtree_list["restore time"  ]  )
        result["create time"   ]   = np.mean(cdtree_list["create time"   ]  )
        result["destory time"  ]   = np.mean(cdtree_list["destory time"  ]  )
        result["begin time"    ]   = np.mean(cdtree_list["begin time"    ]  )
        result["complete time" ]   = np.mean(cdtree_list["complete time" ]  )
        result["mesg logging"  ]   = np.mean(cdtree_list["mesg logging"  ]  )
        result["libc logging"  ]   = np.mean(cdtree_list["libc logging"  ]  )
        result["mailbox overhead"] = np.mean(cdtree_list["mailbox overhead"])
        result_infos = result['CD info']
        for cdtree in cdtree_list['CD info'][1:]:
            cdtree_infos = cdtree
            for phase_id in cdtree_infos:
#                print "phase : ", phase_id, len(cdtree_list)
#                raw_input("@@@@")
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
        num_results = len(cdtree_list)
    
        for phase_id in result_infos:
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
    labels = ['bare', 'noprv', 'errfree', 'total', 'preserve', 'rollback', 'runtime']
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
result = gatherJSONObj('file_list.txt')
#df = pd.DataFrame.from_dict(result, orient='index')
apps = {}
ftypes = {}
nTasks = {}
inputsizes = {}

for app in result:
    apps[app] = 0
    for ftype in result[app]:
        ftypes[ftype] = 0
        for nTask in result[app][ftype]:
            nTasks[nTask] = 0
            for inputsize in result[app][ftype][nTask]:
                inputsizes[inputsize] = 0
app_list = []
ftype_list = []
nTask_list = []
inputsize_list = []
for app in apps:
    print app
    app_type = re.split(r'_', app)[1]
    if app_type == 'errfree' or app_type == 'bare' or app_type == 'noprv':
        continue
    else:
        app_list.append(app)
for ftype in ftypes:
    print ftype
    ftype_list.append(ftype)
for nTask in nTasks:
    print nTask
    nTask_list.append(str(nTask))
for inputsize in inputsizes:
    print inputsize
    inputsize_list.append(inputsize)
dfmi = genEmptyDF(app_list, inputsize_list, nTask_list, ftype_list)

#print dfmi
print dfmi['bare'].loc['lulesh_opt','40','216']
#for app in result:
#    for ftype in result[app]:
#        for nTask in result[app][ftype]:
#            for inputsize in result[app][ftype][nTask]:
#                print dfmi['bare'].loc[app][inputsize][nTask]
#                raw_input('3333333333333333333333333333333333')

for app in result:
    for ftype in result[app]:
        for nTask in result[app][ftype]:
            for inputsize in result[app][ftype][nTask]:
                # Now we have all the measurements for the same configs
#                avg_cdinfo = {}
                result[app][ftype][nTask][inputsize] = averageCDTree(result[app][ftype][nTask][inputsize])
#                print "\n\n\n Final %s %s %s~~~~~~~~~~~~~" % (app, nTask, inputsize)
#                printResults(avg_result['CD info'])
print dfmi['bare'].loc['lulesh_opt','40','216']
print dfmi['errfree'].loc['lulesh_opt','60','216'] 

for app in result:
    app_type = re.split(r'_', app)[1]
    app_name = re.split(r'_', app)[0]
    if app_type == 'errfree' or app_type == 'bare' or app_type == 'noprv':
        continue
    else:
        for ftype in result[app]:
            for nTask in result[app][ftype]:
                for inputsize in result[app][ftype][nTask]:
#                    print (app_name + '_errfree'), ftype, nTask, inputsize
#                    print result[app_name + '_errfree'][ftype][nTask][inputsize]
                    print app, inputsize, nTask
                    print dfmi
                    noprv    = result[app_name + '_noprv'][ftype][nTask][inputsize]['total time']
                    errfree  = result[app_name + '_errfree'][ftype][nTask][inputsize]['total time']
                    preserve = result[app_name + '_errfree'][ftype][nTask][inputsize]['preserve time']
                    total_w_err    = result[app][ftype][nTask][inputsize]['total time']
                    dfmi['noprv'    ].loc[app,str(inputsize),str(nTask)] = noprv 
                    dfmi['errfree'  ].loc[app,str(inputsize),str(nTask)] = errfree
                    dfmi['preserve'].loc[app,str(inputsize),str(nTask)]  = preserve
                    dfmi['total'].loc[app,str(inputsize),str(nTask)] = total_w_err
                    total    = result[app][ftype][nTask][inputsize]['total time']
                    runtime_overhead = result[app][ftype][nTask][inputsize]['begin time'] \
                                     + result[app][ftype][nTask][inputsize]['complete time'] \
                                     + result[app][ftype][nTask][inputsize]['create time'] \
                                     + result[app][ftype][nTask][inputsize]['destory time'] \
                                     + result[app][ftype][nTask][inputsize]['sync time exec']
#                                    + result[app][ftype][nTask][inputsize]['sync time reex']
#                                    + result[app][ftype][nTask][inputsize]['sync time recr']
                    rt_overhead = errfree - preserve - noprv
                    if rt_overhead > 0:
                        dfmi['runtime'].loc[app,str(inputsize),str(nTask)] = rt_overhead
                    else:
                        dfmi['runtime'].loc[app,str(inputsize),str(nTask)] = 0
                    dfmi['rollback'].loc[app,str(inputsize),str(nTask)] = total_w_err - errfree

print dfmi



#                for cdinfo in result[app][ftype][nTasks][inputsize]:
#
#                    print '=======================\n'
#                    print cdinfo
#                    print '=======================\n'
#                    for root in cdinfo:
#                        if root[0] == 'C' and root[1] == 'D':
#                            print root, len(cdinfo[root])
#                            for elem in cdinfo[root]:
#                                print cdinfo[root]["exec"]["max"]
#






















