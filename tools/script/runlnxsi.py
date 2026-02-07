import os
import re
import sys
import shlex
import argparse as ap
import subprocess as sp
import datetime as dt

def parseArgs():
    argParser = ap.ArgumentParser()
    argParser.add_argument('-t', '--trace', dest='trace', default="", help=('input trace file for replay'))
    argParser.add_argument('-g', '--group', dest='group', default="", help=('input trace file for replay'))
    argParser.add_argument('-o', '--output', dest='output', default="./pfm_counter_gen", help=('input trace file for replay'))
    args = argParser.parse_args()
    if args.trace == "" or not os.path.exists(args.trace):
        print("[info] Please specify the trace file which want to dump perf counter!")
        assert(0)
    if args.output == "":
        args.output = './pfm_repaly_'+dt.datetime.now().strftime("%Y%m%d")
    return args

def replayTrace(args):
    if os.path.exists(args.output):
        os.chdir(args.output)
    else:
        os.mkdir(args.output)
        os.chdir(args.output)
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
    pfmCfg = r'/home/mtarch/gene/perf/utils/config/qy2_pfm_config_group'+args.group+'.yaml'
    pfmArg = r' -i 400 --buffer-size 8 --parallel 8 -c '+pfmCfg
    pfmCmd = f'/home/mtarch/gene/perf/utils/pfm-tool/mthreads-pfm-controller {pfmArg}'
    pfmCmdLine = shlex.split(pfmCmd)
    print(pfmCmdLine)
    
    replayCmd = ""
    if re.match('.*trace$', args.trace):
        replayCmd='apitrace dump-images -o '+args.output+' '+args.trace
    elif re.match('.*gfxr$', args.trace):
        replayCmd='gfxrecon-replay --remove-unsupported -m rebind --screenshot-all --screenshot-dir '+args.output+' --screenshot-prefix pfm '+args.trace 

    pfm = sp.Popen(pfmCmdLine, stdin=sp.PIPE, stdout=sp.PIPE, stderr=sp.PIPE)
    os.system(replayCmd)
    stdout, stderr = pfm.communicate('stdin: q'.encode('utf-8'))
    print(stdout)
    pfm.wait()
    


if __name__ == "__main__":
    args = parseArgs()
    replayTrace(args)

