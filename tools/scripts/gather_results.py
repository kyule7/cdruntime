#!/usr/bin/python
import json
import pandas as pd

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
            print item
            if item != 'ChildCDs':
                flattened[cds][item] = cds_list[cds][item]
            else:
                makeFlatCDs(flattened, cds_list[cds][item])
    print flattened

def gatherJSONObj(filelist):
    newfile = open(filelist, 'r')
    for line in newfile:
        file_list = line.split(" ")

    print file_list
    gathered = {}
    tg = []
    for each_filename in file_list:
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
                gathered[exec_name][fail_type][numTasks][input_size] = []
#            gathered[exec_name][fail_type][numTasks][input_size].append(cd_info)
            print cd_info
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
            print '~!~!~!~!~!~!~!~!'
            print flattened

            gathered[exec_name][fail_type][numTasks][input_size].append(flattened)
    return gathered

def averageCDTree(cdtree_list):
    result = cdtree_list[0]
    for cdtree in cdtree_list[1:]:
        for phase_id in cdtree:
            print "phase : ", phase_id
            result[phase_id]["exec"        ]["max"] += float(cdtree[phase_id]["exec"        ]["max"])
            result[phase_id]["exec"        ]["min"] += float(cdtree[phase_id]["exec"        ]["min"])
            result[phase_id]["exec"        ]["avg"] += float(cdtree[phase_id]["exec"        ]["avg"])
            result[phase_id]["exec"        ]["std"] += float(cdtree[phase_id]["exec"        ]["std"])
            result[phase_id]["reexec"      ]["max"] += float(cdtree[phase_id]["reexec"      ]["max"])
            result[phase_id]["reexec"      ]["min"] += float(cdtree[phase_id]["reexec"      ]["min"])
            result[phase_id]["reexec"      ]["avg"] += float(cdtree[phase_id]["reexec"      ]["avg"])
            result[phase_id]["reexec"      ]["std"] += float(cdtree[phase_id]["reexec"      ]["std"])
            result[phase_id]["prv_copy"    ]["max"] += float(cdtree[phase_id]["prv_copy"    ]["max"])
            result[phase_id]["prv_copy"    ]["min"] += float(cdtree[phase_id]["prv_copy"    ]["min"])
            result[phase_id]["prv_copy"    ]["avg"] += float(cdtree[phase_id]["prv_copy"    ]["avg"])
            result[phase_id]["prv_copy"    ]["std"] += float(cdtree[phase_id]["prv_copy"    ]["std"])
            result[phase_id]["prv_ref"     ]["max"] += float(cdtree[phase_id]["prv_ref"     ]["max"])
            result[phase_id]["prv_ref"     ]["min"] += float(cdtree[phase_id]["prv_ref"     ]["min"])
            result[phase_id]["prv_ref"     ]["avg"] += float(cdtree[phase_id]["prv_ref"     ]["avg"])
            result[phase_id]["prv_ref"     ]["std"] += float(cdtree[phase_id]["prv_ref"     ]["std"])
            result[phase_id]["restore"     ]["max"] += float(cdtree[phase_id]["restore"     ]["max"])
            result[phase_id]["restore"     ]["min"] += float(cdtree[phase_id]["restore"     ]["min"])
            result[phase_id]["restore"     ]["avg"] += float(cdtree[phase_id]["restore"     ]["avg"])
            result[phase_id]["restore"     ]["std"] += float(cdtree[phase_id]["restore"     ]["std"])
            result[phase_id]["msg_log"     ]["max"] += float(cdtree[phase_id]["msg_log"     ]["max"])
            result[phase_id]["msg_log"     ]["min"] += float(cdtree[phase_id]["msg_log"     ]["min"])
            result[phase_id]["msg_log"     ]["avg"] += float(cdtree[phase_id]["msg_log"     ]["avg"])
            result[phase_id]["msg_log"     ]["std"] += float(cdtree[phase_id]["msg_log"     ]["std"])
            result[phase_id]["total_time"  ]["max"] += float(cdtree[phase_id]["total_time"  ]["max"])
            result[phase_id]["total_time"  ]["min"] += float(cdtree[phase_id]["total_time"  ]["min"])
            result[phase_id]["total_time"  ]["avg"] += float(cdtree[phase_id]["total_time"  ]["avg"])
            result[phase_id]["total_time"  ]["std"] += float(cdtree[phase_id]["total_time"  ]["std"])
            result[phase_id]["reex_time"   ]["max"] += float(cdtree[phase_id]["reex_time"   ]["max"])
            result[phase_id]["reex_time"   ]["min"] += float(cdtree[phase_id]["reex_time"   ]["min"])
            result[phase_id]["reex_time"   ]["avg"] += float(cdtree[phase_id]["reex_time"   ]["avg"])
            result[phase_id]["reex_time"   ]["std"] += float(cdtree[phase_id]["reex_time"   ]["std"])
            result[phase_id]["sync_time"   ]["max"] += float(cdtree[phase_id]["sync_time"   ]["max"])
            result[phase_id]["sync_time"   ]["min"] += float(cdtree[phase_id]["sync_time"   ]["min"])
            result[phase_id]["sync_time"   ]["avg"] += float(cdtree[phase_id]["sync_time"   ]["avg"])
            result[phase_id]["sync_time"   ]["std"] += float(cdtree[phase_id]["sync_time"   ]["std"])
            result[phase_id]["prv_time"    ]["max"] += float(cdtree[phase_id]["prv_time"    ]["max"])
            result[phase_id]["prv_time"    ]["min"] += float(cdtree[phase_id]["prv_time"    ]["min"])
            result[phase_id]["prv_time"    ]["avg"] += float(cdtree[phase_id]["prv_time"    ]["avg"])
            result[phase_id]["prv_time"    ]["std"] += float(cdtree[phase_id]["prv_time"    ]["std"])
            result[phase_id]["rst_time"    ]["max"] += float(cdtree[phase_id]["rst_time"    ]["max"])
            result[phase_id]["rst_time"    ]["min"] += float(cdtree[phase_id]["rst_time"    ]["min"])
            result[phase_id]["rst_time"    ]["avg"] += float(cdtree[phase_id]["rst_time"    ]["avg"])
            result[phase_id]["rst_time"    ]["std"] += float(cdtree[phase_id]["rst_time"    ]["std"])
            result[phase_id]["create_time" ]["max"] += float(cdtree[phase_id]["create_time" ]["max"])
            result[phase_id]["create_time" ]["min"] += float(cdtree[phase_id]["create_time" ]["min"])
            result[phase_id]["create_time" ]["avg"] += float(cdtree[phase_id]["create_time" ]["avg"])
            result[phase_id]["create_time" ]["std"] += float(cdtree[phase_id]["create_time" ]["std"])
            result[phase_id]["destroy_time"]["max"] += float(cdtree[phase_id]["destroy_time"]["max"])
            result[phase_id]["destroy_time"]["min"] += float(cdtree[phase_id]["destroy_time"]["min"])
            result[phase_id]["destroy_time"]["avg"] += float(cdtree[phase_id]["destroy_time"]["avg"])
            result[phase_id]["destroy_time"]["std"] += float(cdtree[phase_id]["destroy_time"]["std"])
            result[phase_id]["begin_time"  ]["max"] += float(cdtree[phase_id]["begin_time"  ]["max"])
            result[phase_id]["begin_time"  ]["min"] += float(cdtree[phase_id]["begin_time"  ]["min"])
            result[phase_id]["begin_time"  ]["avg"] += float(cdtree[phase_id]["begin_time"  ]["avg"])
            result[phase_id]["begin_time"  ]["std"] += float(cdtree[phase_id]["begin_time"  ]["std"])
            result[phase_id]["compl_time"  ]["max"] += float(cdtree[phase_id]["compl_time"  ]["max"])
            result[phase_id]["compl_time"  ]["min"] += float(cdtree[phase_id]["compl_time"  ]["min"])
            result[phase_id]["compl_time"  ]["avg"] += float(cdtree[phase_id]["compl_time"  ]["avg"])
            result[phase_id]["compl_time"  ]["std"] += float(cdtree[phase_id]["compl_time"  ]["std"])
            result[phase_id]["advance_time"]["max"] += float(cdtree[phase_id]["advance_time"]["max"])
            result[phase_id]["advance_time"]["min"] += float(cdtree[phase_id]["advance_time"]["min"])
            result[phase_id]["advance_time"]["avg"] += float(cdtree[phase_id]["advance_time"]["avg"])
            result[phase_id]["advance_time"]["std"] += float(cdtree[phase_id]["advance_time"]["std"])
    num_results = len(cdtree)
    for phase_id in result:
        result[phase_id]["exec"        ]["max"] /= num_results
        result[phase_id]["exec"        ]["min"] /= num_results
        result[phase_id]["exec"        ]["avg"] /= num_results
        result[phase_id]["exec"        ]["std"] /= num_results
        result[phase_id]["reexec"      ]["max"] /= num_results
        result[phase_id]["reexec"      ]["min"] /= num_results
        result[phase_id]["reexec"      ]["avg"] /= num_results
        result[phase_id]["reexec"      ]["std"] /= num_results
        result[phase_id]["prv_copy"    ]["max"] /= num_results
        result[phase_id]["prv_copy"    ]["min"] /= num_results
        result[phase_id]["prv_copy"    ]["avg"] /= num_results
        result[phase_id]["prv_copy"    ]["std"] /= num_results
        result[phase_id]["prv_ref"     ]["max"] /= num_results
        result[phase_id]["prv_ref"     ]["min"] /= num_results
        result[phase_id]["prv_ref"     ]["avg"] /= num_results
        result[phase_id]["prv_ref"     ]["std"] /= num_results
        result[phase_id]["restore"     ]["max"] /= num_results
        result[phase_id]["restore"     ]["min"] /= num_results
        result[phase_id]["restore"     ]["avg"] /= num_results
        result[phase_id]["restore"     ]["std"] /= num_results
        result[phase_id]["msg_log"     ]["max"] /= num_results
        result[phase_id]["msg_log"     ]["min"] /= num_results
        result[phase_id]["msg_log"     ]["avg"] /= num_results
        result[phase_id]["msg_log"     ]["std"] /= num_results
        result[phase_id]["total_time"  ]["max"] /= num_results
        result[phase_id]["total_time"  ]["min"] /= num_results
        result[phase_id]["total_time"  ]["avg"] /= num_results
        result[phase_id]["total_time"  ]["std"] /= num_results
        result[phase_id]["reex_time"   ]["max"] /= num_results
        result[phase_id]["reex_time"   ]["min"] /= num_results
        result[phase_id]["reex_time"   ]["avg"] /= num_results
        result[phase_id]["reex_time"   ]["std"] /= num_results
        result[phase_id]["sync_time"   ]["max"] /= num_results
        result[phase_id]["sync_time"   ]["min"] /= num_results
        result[phase_id]["sync_time"   ]["avg"] /= num_results
        result[phase_id]["sync_time"   ]["std"] /= num_results
        result[phase_id]["prv_time"    ]["max"] /= num_results
        result[phase_id]["prv_time"    ]["min"] /= num_results
        result[phase_id]["prv_time"    ]["avg"] /= num_results
        result[phase_id]["prv_time"    ]["std"] /= num_results
        result[phase_id]["rst_time"    ]["max"] /= num_results
        result[phase_id]["rst_time"    ]["min"] /= num_results
        result[phase_id]["rst_time"    ]["avg"] /= num_results
        result[phase_id]["rst_time"    ]["std"] /= num_results
        result[phase_id]["create_time" ]["max"] /= num_results
        result[phase_id]["create_time" ]["min"] /= num_results
        result[phase_id]["create_time" ]["avg"] /= num_results
        result[phase_id]["create_time" ]["std"] /= num_results
        result[phase_id]["destroy_time"]["max"] /= num_results
        result[phase_id]["destroy_time"]["min"] /= num_results
        result[phase_id]["destroy_time"]["avg"] /= num_results
        result[phase_id]["destroy_time"]["std"] /= num_results
        result[phase_id]["begin_time"  ]["max"] /= num_results
        result[phase_id]["begin_time"  ]["min"] /= num_results
        result[phase_id]["begin_time"  ]["avg"] /= num_results
        result[phase_id]["begin_time"  ]["std"] /= num_results
        result[phase_id]["compl_time"  ]["max"] /= num_results
        result[phase_id]["compl_time"  ]["min"] /= num_results
        result[phase_id]["compl_time"  ]["avg"] /= num_results
        result[phase_id]["compl_time"  ]["std"] /= num_results
        result[phase_id]["advance_time"]["max"] /= num_results
        result[phase_id]["advance_time"]["min"] /= num_results
        result[phase_id]["advance_time"]["avg"] /= num_results
        result[phase_id]["advance_time"]["std"] /= num_results
    return result                                                                                   

def printResults(result):
    for phase_id in result:
        print "   max    min    avg    std   ", phase_id
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["exec"        ]["max"], result[phase_id]["exec"        ]["min"], result[phase_id]["exec"        ]["avg"], result[phase_id]["exec"        ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["reexec"      ]["max"], result[phase_id]["reexec"      ]["min"], result[phase_id]["reexec"      ]["avg"], result[phase_id]["reexec"      ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["prv_copy"    ]["max"], result[phase_id]["prv_copy"    ]["min"], result[phase_id]["prv_copy"    ]["avg"], result[phase_id]["prv_copy"    ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["prv_ref"     ]["max"], result[phase_id]["prv_ref"     ]["min"], result[phase_id]["prv_ref"     ]["avg"], result[phase_id]["prv_ref"     ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["restore"     ]["max"], result[phase_id]["restore"     ]["min"], result[phase_id]["restore"     ]["avg"], result[phase_id]["restore"     ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["msg_log"     ]["max"], result[phase_id]["msg_log"     ]["min"], result[phase_id]["msg_log"     ]["avg"], result[phase_id]["msg_log"     ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["total_time"  ]["max"], result[phase_id]["total_time"  ]["min"], result[phase_id]["total_time"  ]["avg"], result[phase_id]["total_time"  ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["reex_time"   ]["max"], result[phase_id]["reex_time"   ]["min"], result[phase_id]["reex_time"   ]["avg"], result[phase_id]["reex_time"   ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["sync_time"   ]["max"], result[phase_id]["sync_time"   ]["min"], result[phase_id]["sync_time"   ]["avg"], result[phase_id]["sync_time"   ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["prv_time"    ]["max"], result[phase_id]["prv_time"    ]["min"], result[phase_id]["prv_time"    ]["avg"], result[phase_id]["prv_time"    ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["rst_time"    ]["max"], result[phase_id]["rst_time"    ]["min"], result[phase_id]["rst_time"    ]["avg"], result[phase_id]["rst_time"    ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["create_time" ]["max"], result[phase_id]["create_time" ]["min"], result[phase_id]["create_time" ]["avg"], result[phase_id]["create_time" ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["destroy_time"]["max"], result[phase_id]["destroy_time"]["min"], result[phase_id]["destroy_time"]["avg"], result[phase_id]["destroy_time"]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["begin_time"  ]["max"], result[phase_id]["begin_time"  ]["min"], result[phase_id]["begin_time"  ]["avg"], result[phase_id]["begin_time"  ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["compl_time"  ]["max"], result[phase_id]["compl_time"  ]["min"], result[phase_id]["compl_time"  ]["avg"], result[phase_id]["compl_time"  ]["std"])
        print "%6.3f %6.3f %6.3f %6.3f" % (result[phase_id]["advance_time"]["max"], result[phase_id]["advance_time"]["min"], result[phase_id]["advance_time"]["avg"], result[phase_id]["advance_time"]["std"])
                                                                                       
                                                                                       
result = gatherJSONObj('file_list.txt')
#df = pd.DataFrame.from_dict(result, orient='index')
for app in result:
    for ftype in result[app]:
        for nTasks in result[app][ftype]:
            for inputsize in result[app][ftype][nTasks]:
                # Now we have all the measurements for the same configs
#                avg_cdinfo = {}
                avg_result = averageCDTree(result[app][ftype][nTasks][inputsize])
                print "\n\n\n Final~~~~~~~~~~~~~``"
                printResults(avg_result)
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






















