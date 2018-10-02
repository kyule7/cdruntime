#python
import json
import numpy as np
import argparse as ap

prologue = r"""
{
  // global parameters
  "global param" : {
    "max error" : 20
  },

  // CD hierarchy
  "CD info" : {
"""


epilouge = r"""
  }, // CD info
  "sweep" : {
    "global param.max error" : {
      "sweep_type" : "list",
      "data_type" : "uint",
      "space" : [30]
    }
  }
}
"""


  
lv = 0
prsv_post = r"""},
  "prod" : {
    "c" : 6000
  }
},
"child CDs" : {
"""

def getCDBody(param):
    phase = 0
    cdstr = ""
    #for e in param:
    #    print(e)
    total_lv = param["total_lv"]
    prsv_cnt  = param["prsv_cnt"]
    prsv_vol  = param["prsv_vol"]
    error_vector= param["error vector"]
    iterations  = param["iterations"  ]
    exec_avg_avg= param["exec avg avg"]
    exec_avg_std= param["exec avg std"]
    exec_std_avg= param["exec std avg"]
    exec_std_std= param["exec std std"]
    avg = np.random.normal(exec_avg_avg, exec_avg_std)
    std = np.random.normal(exec_std_avg, exec_std_std)
    #print(total_lv) 
    def CDBody(cdstr, lv, phase, total_lv):
        indent1 = '  ' * (lv + 1) + '  '
        indent2 = indent1 + '    '
        indentp = indent1 + '  '
        if lv == total_lv:
            return cdstr
        else:
            cd_id = 'CD' + str(lv) + "_" + str(phase)
            cd_label = 'Label_' + str(lv) + "_" + str(phase)
            cd_first = indent1 + "\"" + cd_id + "\""
            cd_detail = " : {\n" \
                  + indentp + "\"profile\" : {\n" \
                  + indent2 + "\"iterations\" : " + str(iterations) + ",\n" \
                  + indent2 + "\"label\" : \"" + cd_label + "\",\n" \
        					+ indent2 + "\"type\"  : \"hmcd\", // heterogeneous CDs\n" \
                  + indent2 + "\"child siblings\" : 1,\n" \
        					+ indent2 + "\"fault rate\" : 0.00000001,\n" 
            exec_str =  indent2 + "\"exec time\": " + str(avg) +",\n" \
                      + indent2 + "\"exec time std\": " + str(std) + ",\n"  \
                      + indent2 + "\"prsv type\" : \"LocalMemory\",\n" \
                      + indent2 + "\"preserve vol\" : " + str(prsv_vol) + ",\n" \
                      + indent2 + "\"comm payload\" : " + str(50) + ",\n" \
                      + indent2 + "\"comm type\" : \"nonblocking\",\n" \
                      + indent2 + "\"error vector\" : " + str(error_vector) + ",\n" \
                			+ indent2 + "\"cons\" : {\n"
            prsv = ""
            for i in range(0, prsv_cnt): 
                prsv += indent2 + '    ' + "\"" + cd_id + "_prv_" + str(i) + "\" : " + str(float(prsv_vol) / prsv_cnt)
                if i < (prsv_cnt - 1):
                    prsv += ",\n"
                else:
                    prsv += "\n"+indent2+"},\n" + indent2 + "\"prod\" : {}\n" + \
                    indentp + "}, // profile ends\n"
            cdchild = indentp + "\"child CDs\" : { // child\n" 
            cd = cd_first + cd_detail + exec_str + prsv + cdchild
            print(cd)
            cdstr += cd
            CDBody(cdstr, lv+1, phase+1, total_lv)
            postfix = "\n"+ indentp + "} // child ends\n" + indent1 + "} // CD " + str(lv) + " ends"
            print(postfix)
            cdstr += postfix
            return cdstr


    cdret = CDBody(cdstr, lv, phase, total_lv)
    return cdret

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
    return json_str
######################################################################
# Initialization
######################################################################

parser = ap.ArgumentParser(description="CD descritor generator")
parser.add_argument('file', help='file to process')
parser.add_argument('-i', '--input-files', dest='input_files', help="any input files", nargs='*')
args = parser.parse_args()

print(args.file)

cdbody = ""
with open(args.file, 'r') as each:
    jsonfile = getJsonString(each)
    param = json.loads(jsonfile)
    print prologue
    cdbody = getCDBody(param)
    print epilouge

