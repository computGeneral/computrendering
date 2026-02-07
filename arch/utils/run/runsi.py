import os
import re
import sys
import time
import shlex
import argparse as ap
import subprocess as sp
import datetime as dt

def parseArgs():
    outdir = './_pfm_'

    argParser = ap.ArgumentParser()
    argParser.add_argument('-a', '--app', dest='app', default="none", help=('input app binary to run or trace file to replay'))
    argParser.add_argument('-g', '--group', dest='group', default="all", help=('pfm counter group for replay, default is "all" to replay group times to get all counters in desgin, or set the specific group id to run specific counter group, i.e. -g 0 # to get counter group0'))
    argParser.add_argument('-o', '--outdir', dest='output', default=outdir, help=('output dir to place the generated counter files, default is the /pfm_counter_gen in current path'))
    args = argParser.parse_args()
    if args.app == "none" or not os.path.exists(args.app):
        print("[INFO|RUNSI] Please specify the trace file which want to dump perf counter!")
        assert(0)
    return args

def replayTrace(args, scriptdir):
    #########################
    # sysmem to disk max throughput 1.5GB/s
    # each instance 128 counter * 32bit = 512byte, total 512B * 14 instance * 8 core = 56KB
    #               64 * 32 bit = 256byte total 256 * 8 instance * 8 core = 16KB
    #               total 72KB
    # interval = 400 (400 *256 = 102400 cycle interval counter capture (counter buffer write))
    # 1.5GHz GPU Freq, 102400 * 0.67ns = 68.608us (produce 72KB perf counter data)
    # PFM counter data produce rate: 1.0GB/s 
    
    # buffer size = 8MB/instance
    # 8 * 22 * 8 = 1.375GB cost for sysmem.
    # 8MB/512B = 16384 times PFM dump to make buffer full
    # 16384 / freq (64) = 256 pfm dump (256*400*256*0.67 = 0.0176s) per csv dump kick, (256*72KB = 18MB)
    # currency: 176/paralle thread number. 
    #########################
    pfmCfg = scriptdir+'/qy2_pfm_config_group'+args.group+'.yaml'
    pfmArg = r' -i 400 --buffer-size 8 --parallel 8 -c '+pfmCfg
    pfmCmd = f'mthreads-pfm-controller {pfmArg}'
    pfmCmdLine = shlex.split(pfmCmd)
    print(pfmCmdLine)
    pfm=sp.Popen(pfmCmdLine, stdin=sp.PIPE, stdout=sp.PIPE, stderr=sp.PIPE)

    #while True:
    #    print("debug1")
    #    line = pfm.stdout.readline()
    #    if 'pfm start success' in line:
    #        break

    #print("debug2")
    time.sleep(2)
    

    
    runCmd = ""
    if re.match('.*trace$', args.app):
        #runCmd='apitrace dump-images -o '+args.output+' '+args.app
        runCmd='glretrace '+args.app
    elif re.match('.*gfxr$', args.app):
        runCmd='gfxrecon-replay --remove-unsupported -m rebind --screenshot-all --screenshot-dir '+args.output+' --screenshot-prefix pfm '+args.app 
    else:
        runCmd=args.app
    os.system(runCmd)

    stdout, stderr = pfm.communicate('stdin: q'.encode('utf-8'))
    print(stdout)
    pfm.wait()


if __name__ == "__main__":
    args = parseArgs()
    scriptdir = os.path.realpath(os.path.dirname(__file__))
    appname = os.path.basename(args.app)
    args.output = args.output+'group'+args.group+'_'+appname+'_'+dt.datetime.now().strftime("%Y%m%d")+'_'

    if os.path.exists(args.output):
        os.chdir(args.output)
    else:
        os.mkdir(args.output)
        os.chdir(args.output)

    if args.group == 'all':
        for i in range(7): # group 0-6 in qy2
           args.group = str(i)
           print("[INFO]: Group"+args.group+" perf counter dumping")
           replayTrace(args, scriptdir)
    else:
        print("[INFO]: Group"+args.group+" perf counter dumping")
        replayTrace(args, scriptdir)

