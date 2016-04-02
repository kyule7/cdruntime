#!/usr/bin/python

# This is not well tested, but it will accept a list of files as an argument, 
# monitor those files and coalesce one row from each file into one printed row

import time, sys
writebuffer=[]
row=[]
something_printed = False
class TaskLog:
    def __init__(self, fileName, idx):
        self.file = fileName
        self.line = '------'
        self.idx = idx
    def SetWhere(self, thisline):
        self.where = thisline
    def SetSeek(self):
        self.file.seek(self.where)
    def UpdateLine(self):
        self.anyUpdate = False
        newline = self.file.readline().rstrip()
#        print '-------\n'
#        print '%s == %s' % (newline, self.line)
#        print '-------\n'
        if newline != self.line:
#            print '-------\n'
#            print newline, self.line
#            print '-------\n'
            something_printed = False
#            for col in row:
#                print '%6s' % col.rstrip('\n'),
#            print
            if row[self.idx] != '------':
                for i in range(len(row)):
                    if row[i] == '------':
                        row[i] = files[i].line
                    print '%6s' % row[i].rstrip('\n'),
                    if i == (len(row)/2)-1:
                        print '  ',
                    if row[i] != '------':
                        something_printed = True
                    row[i] = '------'
                if something_printed:
                    print
#                    print 'sp %d new:%s self:%s[%s %s %s %s]' % (self.idx, 
#                            newline, self.line,
#                            row[0], row[1], row[2], row[3])
#            else:
#                print 'repeat! %d something? %d' % (self.idx, something_printed)
            row[self.idx] = newline
            self.line = newline
            self.anyUpdate = True
        else:
            self.anyUpdate = False
        return self.anyUpdate

fileNames = sys.argv
fileNames.pop(0)
        
files=[]
idx=0
for fileName in fileNames:
    files.append(TaskLog(open(fileName, "r"), idx))
    row.append('------')
    idx = idx + 1

while 1:
#    print '1##########################'
    anyUpdate = False
    for log_per_task in files:
        log_per_task.SetWhere(log_per_task.file.tell())
#    print '2!!!!!!!!!!!!!!!!!!!!!!!!!!'
    while not anyUpdate:
        time.sleep(0.1)
        for log_per_task in files:
            log_per_task.SetSeek()
            anyUpdate = anyUpdate | log_per_task.UpdateLine()
#    print '3@@@@@@@@@@@@@@@@@@@@@@@@@@'
#            if something_printed:
#                print
#            else:
#                print 'no update!'



