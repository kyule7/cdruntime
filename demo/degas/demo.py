#!/usr/bin/python

# This is not well tested, but it will accept a list of files as an argument, 
# monitor those files and coalesce one writeBuffer from each file into one printed writeBuffer

import time, sys
writeBuffer=[]
commitBuffer=[]
commited=[]
emptyBuffer = []
#something_printed = False
class TaskLog:
    def __init__(self, fileName, idx):
        self.file = fileName
        self.line = 'Task'+str(idx)
        self.idx = idx
        self.dirty = False
        self.needFlush = False
    def SetWhere(self, thisline):
        self.where = thisline
    def SetSeek(self):
        self.file.seek(self.where)

    def UpdateLine(self):
        newline = self.file.readline().rstrip()
        if newline != '': # newline is something meaningful
            if self.dirty == False:
                writeBuffer[self.idx] = newline
                self.dirty = True
            else: # already dirty
                commitBuffer[self.idx] = writeBuffer[self.idx]
                self.needFlush = True
        else:
            writeBuffer[self.idx] = ''
            self.dirty = False
#            if self.dirty == False:
#                print '?%d?' % self.idx,
#            else:
#                print '!%d!' % self.idx,
        self.line = newline
            
        return self.needFlush

fileNames = sys.argv
fileNames.pop(0)
        
files=[]
idx=0
for fileName in fileNames:
    files.append(TaskLog(open(fileName, "r"), idx))
    writeBuffer.append('')
    commitBuffer.append('')
    commited.append('commit')
    emptyBuffer.append('')
    print '%6s' % files[idx].line,
    if idx == (len(fileNames)/2)-1:
        print '  ',
    idx = idx + 1
print

while 1:
    anyUpdate = False
    for log_per_task in files:
        log_per_task.SetWhere(log_per_task.file.tell())
    while not anyUpdate:
        time.sleep(0.35)
        for log_per_task in files:
            log_per_task.SetSeek()
            anyUpdate = anyUpdate | log_per_task.UpdateLine()

        if anyUpdate:
            for i in range(len(commitBuffer)):
                if files[i].needFlush == False:
                    commitBuffer[i] = writeBuffer[i]
                else:
                    files[i].dirty = False

            if(writeBuffer != emptyBuffer) :
                for i in range(len(commitBuffer)):
                    print '%6s' % commitBuffer[i].rstrip('\n'),
                    if i == (len(commitBuffer)/2)-1:
                        print '  ',
            # Initialize
            for i in range(len(commitBuffer)):
                commited[i] = commitBuffer[i]
                commitBuffer[i] = ''
                files[i].needFlush = False
                
#            print '  WB',
#            for i in range(len(writeBuffer)):
#                print '%6s' % writeBuffer[i].rstrip('\n'),
##                writeBuffer[i] = ''
#                if i == (len(writeBuffer)/2)-1:
#                    print '  ',
#
#            print '  SL',
#            for i in range(len(files)):
#                print '%6s' % files[i].line.rstrip('\n'),
##                writeBuffer[i] = ''
#                if i == (len(files)/2)-1:
#                    print '  ',

            print




#        if writeBuffer != emptyBuffer:
#            for col in writeBuffer:
#                print '%6s' % col,
#                col = ''
#            print
#        if anyUpdate:
#            for i in range(len(commitBuffer)):
#                if commitBuffer[i] == '':
#                    commitBuffer[i] = '###'
#                if writeBuffer[i] == '':
#                    writeBuffer[i] = '###'


#        if(commitBuffer != emptyBuffer and commitBuffer != commited):
#            
#            for i in range(len(commitBuffer)):
#                if files[i].needFlush == False:
#                    if files[i].line != commited[i]:
#                        commitBuffer[i] = writeBuffer[i]
#                    else:
#                        commitBuffer[i] = writeBuffer[i]
#                else:
#                    files[i].needFlush = False
#                #writeBuffer[i] = '------'
#                commited[i] = commitBuffer[i]
#                print '%6s' % commitBuffer[i].rstrip('\n'),
#                commitBuffer[i] = ''
#                if i == (len(commitBuffer)/2)-1:
#                    print '  ',
#            print '  WB',
#            for i in range(len(writeBuffer)):
#                print '%6s' % writeBuffer[i].rstrip('\n'),
#                writeBuffer[i] = ''
#                if i == (len(writeBuffer)/2)-1:
#                    print '  ',
#
#            print '  FILE',
#            for i in range(len(files)):
#                print '%6s' % files[i].line.rstrip('\n'),
#                if i == (len(files)/2)-1:
#                    print '  ',
#            print 

