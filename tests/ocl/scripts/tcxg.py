# pylint: disable = too-few-public-methods
# pylint: disable = consider-using-with - R1732
# pylint: disable = too-many-instance-attributes
"""pdump tools on xorg
"""
from multiprocessing import Process, JoinableQueue
import subprocess
import logging
import os
import getopt
import sys
import datetime
import re
import time

class XorgParams:
    """Params class , collect all parameters
    """
    start_frame = -1
    end_frame = -1
    application = ''
    num_frames = -1
    pid = 0
    xorg_pid = 0
    output_dir = ''
    path = ''
    sleep = 0
    join_queue_2=JoinableQueue()
    join_queue_3=JoinableQueue()

Xorg_params = XorgParams()

def run_app():
    """run_app function"""
    while True:
        if Xorg_params.join_queue_2.empty():
            continue
        data = Xorg_params.join_queue_2.get_nowait()
        #logging.info("run_app从join_queue_2中取到一个data="+str(data))
        if data is not None:
            Xorg_params.xorg_pid = data
            Xorg_params.join_queue_2.task_done()
        if data is None:
            Xorg_params.join_queue_2.task_done()
            #logging.info("run_app取到None中止取数据，开始去run_app")
            break

    if Xorg_params.num_frames != -1:
        Xorg_params.application = Xorg_params.application + ' -f '+str(Xorg_params.num_frames)
    logging.info('run %s started', Xorg_params.application)
    time.sleep(0.5)
    ret_run_application = subprocess.Popen(Xorg_params.application, shell=True)
    ret_run_application.communicate()

    time.sleep(2)
    if Xorg_params.num_frames==-1:
        command = "echo x > /root/xorg/pdump/x.txt"
        x_result = subprocess.Popen(command, shell=True)
        x_result.communicate()
        logging.info('pdump  with x finished')


def run_pdump(join_queue):
    """run pdump function
    """
    #command = "/etc/init.d/rc.pvr start"
    #server_start = subprocess.Popen(command, shell=True)
    # server_start.communicate()
    # logging.info('/etc/init.d/rc.pvr start')
    command = "killall Xorg"
    kill_res = subprocess.Popen(command, shell=True)
    kill_res.communicate()
    logging.info(command)
    time.sleep(2)
    pdump_command='pdump'
    if Xorg_params.start_frame!=-1:
        pdump_command=pdump_command+' -c'+str(Xorg_params.start_frame)
    if Xorg_params.end_frame!=-1:
        pdump_command=pdump_command+'-'+str(Xorg_params.end_frame)
    #command = 'pdump -c'+str(start_frame)+'-'+str(end_frame)
    if pdump_command=='pdump':
        #####need x to finish
        pdump_command = 'pdump -c 0<x.txt'
    else:
        pdump_command = pdump_command + ' 0<x.txt'
    x_cmd="cd /root/xorg/pdump;rm -rf  x.txt"
    x_res=subprocess.Popen(x_cmd, shell=True)
    x_res.communicate()
    x_cmd = "cd /root/xorg/pdump;touch x.txt"
    x_res=subprocess.Popen(x_cmd, shell=True)
    x_res.communicate()
    logging.info('touch  x.txt')
    pdump_cmd = 'cd /root/xorg/pdump;' + pdump_command
    ret_pdump = subprocess.Popen(pdump_cmd, shell=True)
    logging.info("pdump的pid= %s",str(ret_pdump.pid))
    logging.info(pdump_cmd)
    Xorg_params.pid =  ret_pdump.pid
    join_queue.put_nowait(Xorg_params.pid)
    logging.info("put pdump pid %s",str(Xorg_params.pid))
    join_queue.join()

    logging.info("pdump process is blocking")
    join_queue.put_nowait(None)
    ret_pdump.communicate()
    logging.info("pdump process is ending")


def run_xorg(join_queue):
    """run xorg function"""
    while True:
        if join_queue.empty():
            continue
        data = join_queue.get_nowait()
        if data is not None:
            join_queue.task_done()
        if data is None:
            join_queue.task_done()
            break
    command="Xorg"
    xorg_result = subprocess.Popen(command,shell = True)
    logging.info('Xorg')
    Xorg_params.xorg_pid=xorg_result.pid
    Xorg_params.join_queue_2.put_nowait(Xorg_params.xorg_pid)
    Xorg_params.join_queue_2.join()
    Xorg_params.join_queue_2.put_nowait(None)
    xorg_result.communicate()

def run_over(join_queue_3):
    """killall Xorg function
    """
    while True:
        if join_queue_3.empty():
            continue
        data = join_queue_3.get_nowait()
        if data is not None:
            join_queue_3.task_done()
        if data is None:
            join_queue_3.task_done()
            break
    command = "killall Xorg"
    kill_res = subprocess.Popen(command, shell=True)
    kill_res.communicate()
    logging.info("%s over",command)

def get_out2():
    """capture pdump and generate out2 files
    """
    logging.basicConfig(filename='tcxg_output.log',level=logging.INFO)
    join_queue = JoinableQueue()
    pro1 = Process(target=run_pdump, args={join_queue})
    pro1.start()
    pro2=Process(target=run_xorg,args={join_queue})
    pro2.start()
    pro3=Process(target=run_over,args={Xorg_params.join_queue_3})
    pro3.start()
    run_app()
    pro1.join()
    command = "cd /root/xorg/pdump;rm -rf ./x.txt"
    x_result = subprocess.Popen(command, shell=True)
    x_result.communicate()
    logging.info('rm x.txt')
    flag = 1
    Xorg_params.join_queue_3.put_nowait(flag)
    Xorg_params.join_queue_3.join()
    Xorg_params.join_queue_3.put_nowait(None)
    pro3.join()
    logging.info("produce out2.prm and out2.txt")
    print("produce out2.prm and out2.txt")


def get_params():
    """collect all parameters
    """
    opts, _ =getopt.getopt(sys.argv[1:],'-a:-s:-e:-f:-l:-o:-p:-h',
    ['application','start_frame','end_frame','num_frames','sleep','output_dir','path','help'])
    for opt_name,opt_value in opts:
        if opt_name in ('-a','--application'):
            Xorg_params.application=opt_value
        if opt_name in ('-s','--start_frame'):
            Xorg_params.start_frame=opt_value
        if opt_name in ('-e','--end_frame'):
            Xorg_params.end_frame=opt_value
        if opt_name in ('-f','--num_frames'):
            Xorg_params.num_frames=opt_value
        if opt_name in ('-l','--sleep'):
            Xorg_params.sleep=opt_value
        if opt_name in ('-o','--output_dir'):
            Xorg_params.output_dir=opt_value
        if opt_name in ('-p','--path'):
            Xorg_params.path=opt_value
        if opt_name in ('-h','--help'):
            usage()
            sys.exit()

def usage():
    """usage() function for users"""
    print(
"""
usage: python [{0}] ... [-a application | -s start_frame |
                         -e end_frame   | -f num_frames  |
                         -l sleep       | -o output_dir  |
                         -p  .txt path  | -h help        ]  ...
-a     : application
-s     : start_frame
-e     : end_frame
-f     : num_frames
-l     : sleep
-o     : output_dir
-p     : .txt path
-h     : help
""")


def save_out2():
    """save out2.prm and out*.txt  to a output_dir
    """
    if os.path.exists(Xorg_params.output_dir) is False:
        logging.info("create output_dir:%s",Xorg_params.output_dir)
        os.makedirs(Xorg_params.output_dir)
    app_name=re.sub("([^\u4e00-\u9fa5\u0030-\u0039\u0041-\u005a\u0061-\u007a])","",
    Xorg_params.application)
    next_dir=Xorg_params.output_dir

    if os.path.exists(next_dir) is False:
        logging.info("create next_dir:%s",next_dir)
        os.makedirs(next_dir)
    command = "mv /root/xorg/pdump/out2.prm "+next_dir+"/"
    prm_res = subprocess.Popen(command, shell=True)
    prm_res.communicate()
    logging.info(command)
    logging.info('save out2.prm succeeded')
    command = "mv /root/xorg/pdump/out2.txt "+next_dir+"/"
    txt_result = subprocess.Popen(command, shell=True)
    txt_result.communicate()
    logging.info(command)
    logging.info('save out2.txt succeeded')
    logging.info(datetime.datetime.now())
    logging.info("###############################################")

def run_one():
    """run one pdump"""
    get_out2()
    logging.info("begin to mov out2.prm and out2.txt to output_dir")
    save_out2()

if __name__ == '__main__':
    get_params()
    if Xorg_params.path:
        if os.path.exists('./pdump_output') is False and Xorg_params.output_dir=="":
            os.makedirs('pdump_output')
        rfile = open(Xorg_params.path,'r',encoding='utf-8')
        array = list(rfile)
        size = len(array)
        i=0
        while i<size:
            Xorg_params.application=array[i]
            run_one()
            i+=1

    else:
        run_one()

    print("application=" + Xorg_params.application)
    print("output_dir=" + Xorg_params.output_dir)
