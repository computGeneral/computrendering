from ast import arg
from ctypes import alignment
import io
from itertools import count
import re
import sys
import os
import math
from tabnanny import check
import argparse
instr_trace_list = []


def parse_cmd():
    cmdparser = argparse.ArgumentParser()
    cmdparser.add_argument("--specify-case-name", dest='specify_case_name', default="", help=('add specify full case path and case name to run, used for single specify case'))
    cmdparser.add_argument("--case-path", dest='case_path', default="", help=('add case path to run, it will read all txt to parse in the directory'))
    cmdparser.add_argument("--out-path", dest='out_path', default="", help=('add output log path, it use to specify output log path. if not specified, it will output into the case directory'))
    cmdparser.add_argument("--depend-depth", dest='depend_depth', default="", help=('add instruction tracing depth, default is full path, like depend-depth=1'))
    cmdparser.add_argument("--instr-kind", dest='instr_kind', default="", help=('add tracing instr types, it can be all instruction, single instruction type, part set like --instr-kind=longlatency'))
    cmdparser.add_argument("--max-instance", dest='max_instance', default="128", help=('add the current execution mode (max instance number) of current shader, like --max-instance=32'))
    args = cmdparser.parse_args()
    return args


class cmd_info:
    def __init__(self) -> None:
        self.case_name_list = []
        self.case_path = ""
        self.case_log_path = ""
        self.depend_depth = 1
        self.instr_kinds = []
        self.max_instance = 128
    
    def parse_cmd_detail_info(self, args):
        if args.specify_case_name == "" or not os.path.exists(args.specify_case_name):
            if not self.check_if_has_files(args):
                print("please add case or specify the case path to run, otherwise no results!")
                assert(0)
                return False
        else:
            args.specify_case_name = args.specify_case_name.replace("\\","/")
            self.case_name_list.append(args.specify_case_name[args.specify_case_name.rfind("/") + 1:])
            self.case_path = args.specify_case_name[: args.specify_case_name.rfind("/") + 1]
        if args.out_path == "" or not os.path.exists(args.out_path):
            self.case_log_path = self.case_path
        else:
            self.case_log_path = args.out_path + "/"
        
        if args.depend_depth != "":
            if args.depend_depth.isdecimal():
                self.depend_depth = int(args.depend_depth)
        if args.instr_kind != "":
            kindlist = [instruction_type.FOP, instruction_type.DOT, instruction_type.INT, instruction_type.BIT, instruction_type.MOV, instruction_type.DMA, instruction_type.SMP, instruction_type.TAP, instruction_type.EMI, instruction_type.PSBRD, instruction_type.PSBWR, instruction_type.ITR, instruction_type.AF32, instruction_type.AP, instruction_type.COND, instruction_type.CTRL]
            if "longlatency" == args.instr_kind.lower():
                self.instr_kinds.append(instruction_type.DMA)
                self.instr_kinds.append(instruction_type.SMP)
            elif "all" == args.instr_kind.lower():
                self.instr_kinds = kindlist
            else:
                for item in kindlist:
                    if item in args.instr_kind:
                        self.instr_kinds.append(item)
        if args.max_instance != "":
            self.max_instance = int(args.max_instance)
        return True

    def check_if_has_files(self, args):
        if args.case_path == "":
            return False
        args.case_path = args.case_path.replace("\\","/")
        self.case_path = args.case_path + "/"
        if not os.path.exists(self.case_path):
            return False
        filelists = os.listdir(self.case_path)
        for fileitem in filelists:
            if "output.txt" in fileitem or ("shader" in fileitem and ".txt" in fileitem):
                self.case_name_list.append(fileitem)
        return True

    def process_files(self):
        global instr_trace_list
        count = 0
        for fileitem in self.case_name_list:
            filein = self.case_path + fileitem
            outfile = fileitem.replace(".txt", "_log.txt")
            fileout = self.case_log_path + outfile
            print("processing file=",filein)
            parser_file(filein, fileout, self)
            instr_trace_list.clear()
            count = count + 1
            print("processing file count=", count)



def add_instr_into_trace_list(instr_trace):
    global instr_trace_list
    instr_trace_list.append(instr_trace)

def output_shader_info(fileout, line):
    log_file = open(fileout, 'a+', encoding='utf-8')
    shaderModel = re.findall("SM:(\w+)", line)
    if shaderModel:
        print(100*"/", "\nSM:"+shaderModel[0]+" including:",end=" ", file=log_file)
    programType = re.findall("==(\w+.*) Program==", line)
    if programType:
        print(" "+programType[0]+" Program,", end=" ", file=log_file)


def output_instr_into_trace_list(fileout, cmdargs):
    global instr_trace_list
    log_file = open(fileout, 'a+', encoding='utf-8')
    print("\n", 50*"=", "\nStart to process shader...", file=log_file)

    shaderinfo = shader_level_info()
    shaderinfo.set_tracing_type_depth(cmdargs)
    shaderinfo.check_info_in_shader_trace(log_file, cmdargs)
    if shaderinfo.warninginfo != "":
        print(shaderinfo.warninginfo, file=log_file)
#        print(50*"*", "\nAnalysis shader info...", file=log_file)
    shaderSize = 0
    for instr in instr_trace_list:
        shaderSize += instr.bytes
    print("instruction count:", len(instr_trace_list), " in shader, ", str(shaderSize), "Bytes total\n", 50*"=", "\n", file=log_file)
    for item in instr_trace_list:
        strtab = ""
        if item.check_instr_long_letancy():
            strtab = strtab + "*"
        if item.in_br_range:
            for tabidx in range(item.in_br_range):
                strtab = strtab + "\t"
        print(strtab, "instr ", item.instrid,", bytes ", item.bytes, ", instr info:", item.strinfo, file=log_file)
        
        #shaderinfo.check_instr_in_trace_list(item, log_file)
        #print("instr=", item.instrid, ",bytes=", item.bytes, ",instr src=",item.src, ",instr src type=",item.src_type, ",instr dst=",item.dest, ",instr dst type=",item.dest_type, file=log_file)
        #print("instr=", item.instrid, ",bytes=", item.bytes, ",wfc=",item.wfc, ",wdfc=",item.wdfc, ", setwfc=",item.setwfc, ",setdfc=",item.setdfc, "instr info:", item.strinfo, file=log_file)
        if item.instr_type in cmdargs.instr_kinds: 
            if len(item.tracesrclist):
                print(strtab.replace("*", "") + "\t\t\t--->", " src trace...", file=log_file)
                for tracedestidx in item.tracesrclist:
                    print(strtab.replace("*", "")+ "\t\t\tinstr ", tracedestidx,", bytes ", instr_trace_list[tracedestidx].bytes, ", instr info:", instr_trace_list[tracedestidx].strinfo, file=log_file)
            if len(item.tracedestlist):
                print(strtab.replace("*", "") + "\t\t\t===>", " dest trace...", file=log_file)
                for tracedestidx in item.tracedestlist:
                    print(strtab.replace("*", "")+ "\t\t\tinstr ", tracedestidx,", bytes ", instr_trace_list[tracedestidx].bytes, ", instr info:", instr_trace_list[tracedestidx].strinfo, file=log_file)
        shaderinfo.clean_depend_list()

    log_file.close()

def output_specify_instr(instrid, filelog):
    global instr_trace_list
    print("\t\t\t  depends on instr ", instr_trace_list[instrid].instrid, ", info:", instr_trace_list[instrid].strinfo, file=filelog)


class shader_level_info:
    def __init__(self ) -> None:
        self.origininstrid = -1
        self.internalnum = 0
        self.arnum = 0
        self.tempnum = 0
        self.vtxoutsize = 0
        self.cfssize = 0
        self.shsize = 0
        self.depend_depth = 0
        self.maxactivewave = 48
        self.maxconstwaves = 48
        self.longlatencyvalue = 300
        self.instr_kinds = []
        self.shader_proj_ver = "qy1"
        self.res_typeobj = res_type()
        self.dependset = set()
        self.donedependset = set()
        self.warninginfo = ""

    def clean_depend_list(self):
        self.dependset = set()
        self.donedependset = set()

    def set_tracing_type_depth(self, cmdargs): 
        self.instr_kinds = cmdargs.instr_kinds 
        self.depend_depth = cmdargs.depend_depth 


    def check_internal_usage(self):
        if self.shader_proj_ver.lower() == "qy2":
            if self.internalnum < 4:
                self.warninginfo = self.warninginfo + "max internal reg is 4 per instance, but only use " + str(self.internalnum) + "\n"
        elif self.shader_proj_ver.lower() == "qy1" or  self.shader_proj_ver.lower() == "sudi":
            if self.internalnum < 2:
                self.warninginfo = self.warninginfo + "max internal reg is 2 per instance, but only use " + str(self.internalnum) + "\n"


    def check_internal_numb(self):
        global instr_trace_list
        maxinternal = -1
        internalinstridlist = [idx for idx, instritem in enumerate(instr_trace_list) if instritem.check_instr_has_internal()]
        for instridx in internalinstridlist:
            srcidx = [idx for idx, srcitem in enumerate(instr_trace_list[instridx].src_type) if srcitem == self.res_typeobj.intreg]
            for singleidx in srcidx:
                #print("error instr index = ", instridx)
                if int(instr_trace_list[instridx].src[singleidx]) > maxinternal:
                    maxinternal = int(instr_trace_list[instridx].src[singleidx])
        self.internalnum = maxinternal + 1
        self.check_internal_usage()

    
    def check_branch_instr_info(self):
        global instr_trace_list
        brinstridlist = [idx for idx, instritem in enumerate(instr_trace_list) if instritem.check_instr_is_branch()]
        for instridx in brinstridlist:
            if self.res_typeobj.imm in instr_trace_list[instridx].dest_type:
                branch_value = int(instr_trace_list[instridx].dest[0])
                brdirection = 1 if branch_value >= 0 else -1
                if brdirection and instr_trace_list[instridx].check_instr_is_branch():
                    branch_value = branch_value - instr_trace_list[instridx].get_instr_bytes()
                while branch_value > 0 and instridx > 0 and instridx < len(instr_trace_list):
                    instridx = instridx + brdirection
                    branch_value = branch_value - instr_trace_list[instridx].get_instr_bytes()
                    instr_trace_list[instridx].set_branch_range()


    def check_cfs_usage(self):
        activeslots = int(self.res_typeobj.cfs_size / (((self.cfssize + 3) >> 2) << 2))
        self.maxactivewave = int(activeslots) if activeslots < self.maxactivewave else self.maxactivewave
        if self.maxactivewave < self.maxconstwaves:
            self.warninginfo = self.warninginfo + "\ncoeffs supports waves number is < " + str(self.maxactivewave) #+ str(self.maxactivewave >> 3) + " ~ " 


    def check_coeffs_size(self):
        global instr_trace_list
        cfs_instridlist = [idx for idx, instritem in enumerate(instr_trace_list) if instritem.check_instr_has_cfs()]
        maxaddr = 0
        for instridx in cfs_instridlist:
            if instr_trace_list[instridx].src_type[0] == self.res_typeobj.cf:
                if maxaddr < int(instr_trace_list[instridx].src[0]):
                    maxaddr = int(instr_trace_list[instridx].src[0])
        self.cfssize = maxaddr + 1
        self.check_cfs_usage()


    def check_uvb_usage(self, cmdargs):
        instnum = [128, 96, 64, 48, 32]
        bout = False
        activeslots = 0
        for item in instnum:
            activeslots = self.res_typeobj.uvb_size / (2 * item * (self.vtxoutsize + 5))
            if activeslots < 7:
                if not bout:
                    self.warninginfo = self.warninginfo + "\n    uvb size limited in " + str(item) + " instance mode (non-empty slot:"+str(activeslots)+"), consider to set the max instance down to "
                    bout = True
                if item == 32:
                    self.warninginfo = self.warninginfo + str(item) + "\n"
            else:
                if item == 128:
                    break
                self.warninginfo = self.warninginfo + str(item) + "\n"
                break

        currentactiveslots = self.res_typeobj.uvb_size / (2 * int(args.max_instance) * (self.vtxoutsize + 5))
        self.warninginfo = self.warninginfo + "    current "+args.max_instance+" instance mode (non-empty slot:"+str(currentactiveslots)+")"
        self.maxactivewave = int(currentactiveslots) if currentactiveslots < self.maxactivewave else self.maxactivewave

    def check_uvb_size(self, cmdargs):
        global instr_trace_list
        uvbwr_instridlist = [idx for idx, instritem in enumerate(instr_trace_list) if instritem.check_instr_is_uvb_write()]
        maxaddr = 0
        for instridx in uvbwr_instridlist:
            if instr_trace_list[instridx].src_type[0] == self.res_typeobj.imm:
                if maxaddr < int(instr_trace_list[instridx].src[0]):
                    maxaddr = int(instr_trace_list[instridx].src[0])
        self.vtxoutsize = maxaddr + 1
        self.warninginfo = self.warninginfo + "\nuvb size = " + str(self.vtxoutsize + 5) + " "
        self.check_uvb_usage(cmdargs)


    def check_us_usage(self):
        self.warninginfo = self.warninginfo + "\nattr size = " + str(self.arnum) + ", temp size = " + str(self.tempnum)
        us_size = ((self.arnum + 1) >> 1) + ((self.tempnum + 1) >> 1) 
        maxactiveslots = self.maxactivewave
        if us_size != 0:
            maxactiveslots = math.floor(self.res_typeobj.us_size / (2 * (128 * us_size))) 
        if maxactiveslots < 12:
            self.warninginfo = self.warninginfo + "us size limited, max active slot <= " + str(maxactiveslots) +", please consider to use private memory..."
        self.maxactivewave = int(maxactiveslots) if maxactiveslots < self.maxactivewave else self.maxactivewave


    def check_temp_nested(self, nested, rangestart, rangeend):
        global instr_trace_list
        maxtempnumb = -1
        inputlist = set()
        outputlist = set()
        templifedict = {}
        destlist = []
        

    def check_temp_utility(self, nested, offset, rangestart, rangeend):
        global instr_trace_list
        maxtempnumb = -1
        inputlist = set()
        outputlist = set()
        templifedict = {}
        destlist = []
        #maxnested = max([instritem.in_br_range for instritem in instr_trace_list if instritem.in_br_range > 0])
        
        #srctemp_instridlist = [idx for idx, instritem in enumerate(instr_trace_list) if (instritem.check_instr_has_temp() and (idx >= rangestart) and (idx <= rangeend))]
        srctemp_instridlist = [idx for idx, instritem in enumerate(instr_trace_list) if instritem.check_instr_has_temp()]
        for tempidx in range(self.tempnum):
            templifedict[tempidx] = []
        for instridx in srctemp_instridlist:
            srcidx = [idx for idx, srcitem in enumerate(instr_trace_list[instridx].src_type) if srcitem == self.res_typeobj.temp]
            for srcitem in srcidx:
                if instr_trace_list[instridx].src_type[srcitem] == self.res_typeobj.temp:
                    srctempaddr = int(instr_trace_list[instridx].src[srcitem])
                    if srctempaddr not in destlist:
                        templifedict[srctempaddr].append(rangestart)
                        inputlist.add(srctempaddr)
                    templifedict[srctempaddr].append(instridx)

            destidx = [idx for idx, destitem in enumerate(instr_trace_list[instridx].dest_type) if destitem == self.res_typeobj.temp]
            for destitem in destidx:
                if instr_trace_list[instridx].dest_type[destitem] == self.res_typeobj.temp:
                    desttempaddr = int(instr_trace_list[instridx].dest[destitem])
                    destlist.append(desttempaddr)
                    templifedict[desttempaddr].append(instridx)
                    outputlist.add(desttempaddr)
        
        self.warninginfo = self.warninginfo + " input temp reg: "
        for item in inputlist:
            self.warninginfo = self.warninginfo + " r" + str(item) + ", "
        self.warninginfo = self.warninginfo + "\n   output temp reg: "
        for item in outputlist:
            self.warninginfo = self.warninginfo + " r" + str(item) + ", "

        count_max_lifetime = -1
        max_count_pos = -1
        for instrlife in range(len(instr_trace_list)):
            current_lifetime = 0
            for tempaddr in range(self.tempnum):
                if len(templifedict[tempaddr]) == 0:
                    continue
                lifestart = min(templifedict[tempaddr])
                lifeend = max(templifedict[tempaddr])
                if instrlife >= lifestart and instrlife <= lifeend:
                    current_lifetime = current_lifetime + 1
            if count_max_lifetime < current_lifetime:
                count_max_lifetime = current_lifetime
                max_count_pos = instrlife
        self.warninginfo = self.warninginfo + "\n   max temp position is " + str(max_count_pos) + "\n   needed max temp reg number is: " + str(count_max_lifetime)
        aligned_max_temp = (((count_max_lifetime + 3) >> 2) << 2)
        aligned_use_temp = (((self.tempnum + 3) >> 2) << 2)
        if aligned_max_temp < aligned_use_temp:
            self.warninginfo = self.warninginfo + "\n   compiler can reduce the temp usage to = " + str(aligned_max_temp)
        return count_max_lifetime


    def check_us_resources(self):
        global instr_trace_list
        maxar = -1
        maxtemp = -1
        srcus_instridlist = [idx for idx, instritem in enumerate(instr_trace_list) if instritem.check_instr_has_us()]
        for instridx in srcus_instridlist:
            srcidx = [idx for idx, srcitem in enumerate(instr_trace_list[instridx].src_type) if (srcitem == self.res_typeobj.ar or srcitem == self.res_typeobj.temp)]
            for srcitem in srcidx:
                if instr_trace_list[instridx].src_type[srcitem] == self.res_typeobj.ar:
                    if maxar < int(instr_trace_list[instridx].src[srcitem]):
                        maxar = int(instr_trace_list[instridx].src[srcitem])
                elif instr_trace_list[instridx].src_type[srcitem] == self.res_typeobj.temp:
                    if maxtemp < int(instr_trace_list[instridx].src[srcitem]):
                        maxtemp = int(instr_trace_list[instridx].src[srcitem])
            destidx = [idx for idx, destitem in enumerate(instr_trace_list[instridx].dest_type) if (destitem == self.res_typeobj.ar or destitem == self.res_typeobj.temp)]
            for destitem in destidx:
                if instr_trace_list[instridx].dest_type[destitem] == self.res_typeobj.ar:
                    if maxar < int(instr_trace_list[instridx].dest[destitem]):
                        maxar = int(instr_trace_list[instridx].dest[destitem])
                elif instr_trace_list[instridx].dest_type[destitem] == self.res_typeobj.temp:
                    if maxtemp < int(instr_trace_list[instridx].dest[destitem]):
                        maxtemp = int(instr_trace_list[instridx].dest[destitem])
        self.arnum = maxar + 1
        self.tempnum = maxtemp + 1
        self.check_us_usage()

    def check_sh_resources(self):
        global instr_trace_list
        srcsh_instridlist = [idx for idx, instritem in enumerate(instr_trace_list) if instritem.check_instr_has_sh()]
        maxaddr = 0
        for instridx in srcsh_instridlist:
            srcidx = [idx for idx, srcitem in enumerate(instr_trace_list[instridx].src_type) if (srcitem == self.res_typeobj.sh)]
            for srcitem in srcidx:
                if instr_trace_list[instridx].src_type[srcitem] == self.res_typeobj.sh:
                    if maxaddr < int(instr_trace_list[instridx].src[srcitem]):
                        maxaddr = int(instr_trace_list[instridx].src[srcitem])
        self.shsize = maxaddr + 1
        self.warninginfo = self.warninginfo + "max shared reg size is " + str(self.shsize)


    def check_instr_types_info(self):
        global instr_trace_list
        totalinstrs = len(instr_trace_list)
        dma_instrs = len([instritem for instritem in instr_trace_list if instritem.check_instr_is_dma()])
        dmaatm_instrs = len([instritem for instritem in instr_trace_list if instritem.check_instr_is_dma_atm()])
        smp_instrs = len([instritem for instritem in instr_trace_list if instritem.check_instr_is_smp()])
        psbrd_instrs = len([instritem for instritem in instr_trace_list if instritem.check_instr_is_psbrd()])
        mix_instrslist = [instritem.instrid for instritem in instr_trace_list if instritem.check_instr_long_letancy()]
        
        if len(mix_instrslist) < 1:
            return
        min_gap = 0xFFFFFFFF
        min_gap_pos = 0xFFFFFFFF
        for instridx in mix_instrslist:
            if len(instr_trace_list[instridx].tracedestlist) == 0:
                continue
            gap = min(instr_trace_list[instridx].tracedestlist) - instridx
            if gap < min_gap:
                min_gap = gap
                min_gap_pos = instridx
        self.warninginfo = self.warninginfo + "\nactive wave number is about " + str(self.maxactivewave)
        if (self.maxactivewave * min_gap) < self.longlatencyvalue:
            self.warninginfo = self.warninginfo + "\nwarning: long latency can't be hidded..." + str(min_gap_pos)
        longlatency = dma_instrs + dmaatm_instrs + smp_instrs + psbrd_instrs
        if longlatency > (totalinstrs * 0.02):
            self.warninginfo = self.warninginfo + "\ntotal instructions number is " + str(totalinstrs) + ",\t" + "long latency number is " + str(longlatency) + "\n"


    def check_info_in_shader_trace(self, log_file, cmdargs):
        global instr_trace_list
        bhasuvbwr = False
        bhasitr = False
        bhasbr = False
        bhaslonglatency = False
        for item in instr_trace_list:
            if item.check_instr_is_uvb_write() and not bhasuvbwr:
                bhasuvbwr = True
            if item.check_instr_is_itr() and not bhasitr:
                bhasitr = True
            if item.check_instr_is_branch() and not bhasbr:
                bhasbr = True
            if item.check_instr_long_letancy() and not bhaslonglatency:
                bhaslonglatency = True

        self.check_internal_numb()
        self.check_sh_resources()
        if bhasuvbwr:
            self.check_uvb_size(cmdargs)
        if bhasitr:
            self.check_coeffs_size()
        self.check_us_resources()
        if bhasbr:
            self.check_branch_instr_info()
        for item in instr_trace_list:
            self.check_instr_src_dest_trace_list(item, log_file)
        self.check_instr_types_info()
        self.check_temp_utility(0, 0, 0, 0)

    def check_instr_src_dest_trace_list(self, item, log_file):
        global instr_trace_list
        self.check_src_depends(item.instrid, log_file)
        self.check_dest_been_depended(item.instrid, len(instr_trace_list), log_file)


    def check_instr_in_trace_list(self, item, filelog):
        global instr_trace_list
        if item.instr_type in self.instr_kinds: 
            self.origininstrid = item.instrid
            self.check_src_depends(item.instrid, filelog)
            self.iteration_check_depend(filelog)
            self.check_dest_been_depended(item.instrid, len(instr_trace_list), filelog)


    def iteration_check_depend(self, filelog):
        if len(self.dependset) == 0:
            return
        waitprocesslist = []
        for item in self.dependset:
            #if item in self.donedependset:
            #    continue
            waitprocesslist.append(item)
        self.dependset.clear()
        for processid in waitprocesslist:
            self.check_src_depends(processid, filelog)
            self.donedependset.add(processid)
        waitprocesslist = []
        if len(self.dependset) != 0:
            self.iteration_check_depend(filelog)


    def check_instr_in_done_list(self, instrid):
        if len(self.donedependset) == 0:
            return False
        for item in self.donedependset:
            if item == instrid:
                return True
        return False


    def check_src_depends(self, instr_id, filelog):
        src_depend_type = instr_trace_list[instr_id].src_type
        src_depend_addr = instr_trace_list[instr_id].src
        for srcidx in range(len(src_depend_type)):
            srcaddr = src_depend_addr[srcidx]
            srctype = src_depend_type[srcidx]
            if self.res_typeobj.check_if_src_can_has_dependency(srctype):
                for instrindex in range(instr_id):
                    if instr_id < 1:
                        break
                    realinstrid = instr_id - instrindex - 1
                    if self.origininstrid != -1:
                        if instr_trace_list[self.origininstrid].check_instr_is_branch() or instr_trace_list[self.origininstrid].get_branch_range() != instr_trace_list[realinstrid].get_branch_range():
                            break
                    if self.check_instr_in_done_list(realinstrid):
                        continue
                    
                    if instr_trace_list[realinstrid].check_occur_resource(srcaddr, srctype):
                        #output_specify_instr(realinstrid, filelog)
                        instr_trace_list[instr_id].add_src_trace_info(realinstrid, True)
                        if self.depend_depth > 1:
                            self.dependset.add(realinstrid)
                            self.donedependset.add(realinstrid)
                        break

    def check_dest_been_depended(self, instr_id, instr_end_id, filelog):
        dest_type = instr_trace_list[instr_id].dest_type
        dest_addr = instr_trace_list[instr_id].dest
        for destidx in range(len(dest_type)):
            destaddr = dest_addr[destidx]
            desttype = dest_type[destidx]
            if not self.res_typeobj.check_if_src_can_has_dependency(desttype):
                continue
            for instrindex in range(instr_id + 1, instr_end_id):
                src_depend_type = instr_trace_list[instrindex].src_type
                if desttype not in src_depend_type:
                    continue
                srctypeindexlist = [idx for idx, instritem in enumerate(src_depend_type) if instritem == desttype]
                bfind = False
                for srcidx in srctypeindexlist:
                    srcaddr = int(instr_trace_list[instrindex].src[srcidx])
                    srctype = src_depend_type[srcidx]
                    if self.res_typeobj.check_if_src_can_has_dependency(srctype):
                        if srcaddr == destaddr and srctype == desttype:
                            instr_trace_list[instr_id].add_src_trace_info(instrindex, False)
                            bfind = True
                            break
                if bfind:
                    break
        #print("instrid = ", instr_id, "tracelist dest", instr_trace_list[instr_id].tracedestlist)
                    # realinstrid = instr_id - instrindex - 1
                    # if instr_trace_list[self.origininstrid].check_instr_is_branch() or instr_trace_list[self.origininstrid].get_branch_range() != instr_trace_list[realinstrid].get_branch_range():
                    #     break
                    # if self.check_instr_in_done_list(realinstrid):
                    #     continue
                    
                    # if instr_trace_list[realinstrid].check_occur_resource(srcaddr, srctype):
                    #     output_specify_instr(realinstrid, filelog)
                    #     self.tracesrclist.append(realinstrid)
                    #     if self.depend_depth > 1:
                    #         self.dependset.add(realinstrid)
                    #         self.donedependset.add(realinstrid)
                    #     break

#instr type
class instruction_type:
    FOP = "fop"
    DOT = "dot"
    INT = "int"
    BIT = "bit"
    MOV = "mov"
    DMA = "dma"
    DMA_BARRIER = "barrier"
    DMA_LD = "dmald"
    DMA_ATOM = "dmald"
    SMP = "smp"
    SMP_LD = "smpld"
    TAP = "tap"
    EMI = "emi"
    EMI_UVB = "uvb_write"
    EMI_ISP = "isp"
    ITR = "itr"
    PSBWR = "psb_wr"
    PSBRD = "psb_rd"
    AF32 = "af32"
    AP = "ap"
    CTRL = "ctrl"
    CTRL_BRANCH = "branch"
    COND = "cond"


#resource type
class res_type:
    ar = 0
    temp = 1
    sh = 2
    cf = 3
    lm=4
    intreg=5
    sl=6
    constreg=7
    sr=8
    emc=9
    pout=10
    pred=11
    imm=12
    index=13 
    err_type = 14

    uvb_size = 18432
    lms_size1 = 7168
    lms_size2 = 18432
    us_size = 131072
    cfs_size = 488

    def parse_direct_res_type(self, res_info_str):
        if "ar" in res_info_str:
            return res_type.ar, res_info_str[res_info_str.find("ar") + len("ar"):]
        elif "r" in res_info_str:
            return res_type.temp, res_info_str[res_info_str.find("r") + len("r"):]
        elif "sh" in res_info_str:
            return res_type.sh, res_info_str[res_info_str.find("sh") + len("sh"):]
        elif "cf" in res_info_str:
            return res_type.cf, res_info_str[res_info_str.find("cf") + len("sh"):]
        elif "lm" in res_info_str:
            return res_type.lm, res_info_str[res_info_str.find("lm") + len("lm"):]
        elif "i" in res_info_str:
            return res_type.intreg, res_info_str[res_info_str.find("i") + len("i"):] 
        elif "sl" in res_info_str:
            return res_type.sl, res_info_str[res_info_str.find("sl") + len("sh"):]
        elif "c" in res_info_str:
            return res_type.constreg, res_info_str[res_info_str.find("c") + len("c"):]
        elif "sr" in res_info_str:
            return res_type.sr, res_info_str[res_info_str.find("sr") + len("sr"):]
        elif "emc" in res_info_str:
            return res_type.emc, res_info_str[res_info_str.find("emc") + len("emc"):] 
        elif "o" in res_info_str:
            return res_type.pout, res_info_str[res_info_str.find("o") + len("o"):]
        elif "p" in res_info_str:
            return res_type.pred, res_info_str[res_info_str.find("p") + len("p"):]
        elif "0x" in res_info_str or "f" in res_info_str:
            res_info_str = res_info_str.replace("f", "")
            return res_type.imm, res_info_str
        else:
            if res_info_str.isnumeric():
                return res_type.imm, res_info_str
            return res_type.err_type, -1

    def parse_res_type(self, res_info_str):
        if "sr[" in res_info_str and "]" in res_info_str:
            return res_type.sr, 0
        elif "[" in res_info_str and "]" in res_info_str:
            index_str = res_info_str[res_info_str.find("[") + len("[") : res_info_str.find("]")]
            res, value = self.parse_direct_res_type(index_str)
            return res_type.index, value
        else:
            res, value = self.parse_direct_res_type(res_info_str)
            return res, value

    def check_index_mode(self, res_info_str):
        if "[" and "]" in res_info_str:
            return True
        return False    
        

    def check_if_src_can_has_dependency(self, src_type):
        if src_type == res_type.ar or src_type == res_type.temp or src_type == res_type.lm or src_type == res_type.intreg or src_type == res_type.sl or src_type == res_type.pred or src_type == res_type.sh:
            return True
        return False


#instr tracing
class instr_trace:
    def __init__(self ) -> None:
        self.instrid = -1
        self.instr_type = ""
        self.instr_detail = ""
        self.src = []
        self.src_type = []
        self.src_trace = []
        self.src_vld = []
        self.dest = []
        self.dest_type = []
        self.dest_vld = []
        self.src_numb = 0
        self.dest_numb = 0
        self.wfc = []
        self.wdfc = []
        self.setwfc = []
        self.setdfc = []
        self.tracesrclist = []
        self.tracedestlist = []
        #self.ndfc = []
        #self.pos = 0xFFFFFFFF
        self.bytes = 0
        self.has_internal = False
        self.has_shared = False
        self.has_cfs = False
        self.has_us = False
        self.is_branch = False
        self.in_br_range = 0
        self.strinfo = ""


    def set_instrid_bytes(self, instrid, instrbytes):
        self.instr_str = []
        self.instrid = instrid
        self.bytes = instrbytes

    def add_src_trace_info(self, instrid, typesrc):
        if typesrc == False:
            self.tracedestlist.append(instrid)
        else:
            self.tracesrclist.append(instrid)


    
    def add_instr_type(self, instrtype):
        self.instr_type = instrtype

        
    def add_wfc_info(self, waitfc, desched):
        if desched:
            self.wdfc.append(waitfc)
        else:
            self.wfc.append(waitfc)


    def add_set_fc(self, setfc, desched):
        if desched:
            self.setdfc.append(setfc)
        else:
            self.setwfc.append(setfc)
    
    def set_branch_range(self):
        self.in_br_range = self.in_br_range + 1

    def get_branch_range(self):
        return self.in_br_range


    def add_src_info(self, srctype, addr):
        self.src.append(addr)
        # if srctype == res_type.imm:
        #     self.src.append(addr)
        # else:
        #     self.src.append(int(addr))
        if srctype == res_type.intreg:
            self.has_internal = True
        elif srctype == res_type.ar or srctype == res_type.temp:
            self.has_us = True
        elif srctype == res_type.sh:
            self.has_shared = True
        elif srctype == res_type.cf:
            self.has_cfs = True

        self.src_type.append(int(srctype))
        self.src_vld.append(True)
        self.src_numb = self.src_numb + 1


    def add_dest_info(self, desttype, addr):
        self.dest.append(addr)
        #self.dest.append(int(addr))
        if desttype == res_type.ar or desttype == res_type.temp:
            self.has_us = True
        elif desttype == res_type.sh:
            self.has_shared = True
        self.dest_type.append(int(desttype))
        self.dest_vld.append(True)
        self.dest_numb = self.dest_numb + 1


    def set_instr_detail_info(self, detail):
        self.instr_detail = detail
    
    def check_instr_is_branch(self):
        return self.instr_type == instruction_type.CTRL and self.instr_detail == instruction_type.CTRL_BRANCH

    def check_instr_is_dma(self):
        return self.instr_type == instruction_type.DMA and self.instr_detail == instruction_type.DMA_LD
    def check_instr_is_dma_atm(self):
        return self.instr_type == instruction_type.DMA and self.instr_detail == instruction_type.DMA_ATOM
    def check_instr_is_dmabarrier(self):
        return self.instr_type == instruction_type.DMA and self.instr_detail == instruction_type.DMA_BARRIER

    def check_instr_is_smp(self):
        return self.instr_type == instruction_type.SMP

    def check_instr_is_psbrd(self):
        return self.instr_type == instruction_type.PSBRD

    def check_instr_is_itr(self):
        return self.instr_type == instruction_type.ITR
    
    def check_instr_long_letancy(self):
        return self.check_instr_is_dma() or self.check_instr_is_dma_atm() or self.check_instr_is_smp() #or self.check_instr_is_psbrd()

    def check_instr_is_uvb_write(self):
        return self.instr_type == instruction_type.EMI and self.instr_detail == instruction_type.EMI_UVB

    def get_instr_bytes(self):
        if not (self.bytes == 4 or self.bytes == 8 or self.bytes == 12 or self.bytes == 16):
            assert(0)
        return self.bytes

    def check_instr_has_internal(self):
        return self.has_internal

    def check_instr_has_us(self):
        return self.has_us 

    def check_instr_has_temp(self):
        if res_type.temp in self.src_type or res_type.temp in self.dest_type:
            return True
        return False
    
    def check_instr_has_sh(self):
        return self.has_shared

    def check_instr_has_cfs(self):
        return self.has_cfs

    def add_instr_str_info(self, instr_str):
        self.strinfo = self.strinfo + instr_str + "; "


    def check_occur_resource(self, srcaddr, srctype):
        srcaddrint = int(srcaddr)
        ucount = 0
        for item in self.dest:
            if srcaddrint == int(item) and self.dest_type[ucount] == srctype:
                return True
            ucount = ucount + 1
        return False



#instr parser
class str_info:
    def __init__(self ) -> None:
        self.instr_str = []
        self.instrid = -1
        self.instrbytes = 0
        self.resource = res_type()

    def set_instrid(self, instrid, instrbytes):
        self.instr_str = []
        self.instrid = instrid
        self.instrbytes = instrbytes
    
    def add_instr_info(self, strline):
        if "type " == strline[:5]:
            self.instr_str.insert(0, strline)
        else:
            self.instr_str.append(strline)
    
    def add_part_src_info(self, instr_trace_info, strline, field, burst):
        srcfield = strline[strline.find(field) + len(field) : ]
        if "," in srcfield:
            srcfield = srcfield[ :srcfield.find(",")]
        if "_" in srcfield or '' == srcfield:
            return 
        src_res_type_info, src_addr_info = self.resource.parse_res_type(srcfield)
        if self.resource.check_index_mode(srcfield):
            instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
        else:
            for idx in range(burst):
                instr_trace_info.add_src_info(src_res_type_info, int(src_addr_info) + idx)


    def parse_fc_info(self, instr_trace_info, line, afterop):
        do_type = []
        if "set = " in line:
            do_type.append("set = ")
        if "wait = " in line:
            do_type.append("wait = ")
        for itemtype in do_type:
            set_fc_type = True if "set = " in line else False
            linetype = line[line.find(itemtype) + len(itemtype) : ]
            fc_info_list = []
            if "," in linetype:
                fc_info_list = linetype.split(",")
            else:
                fc_info_list.append(linetype)
            for fc_item in fc_info_list:
                if "wfc" in fc_item:
                    desched = False
                    fc_item = fc_item.replace("wfc", "")
                    value = int(fc_item)
                    if set_fc_type:
                        instr_trace_info.add_set_fc(value, desched)
                    else:
                        instr_trace_info.add_wfc_info(value, desched)
                if "dfc" in fc_item:
                    desched = True
                    fc_item = fc_item.replace("dfc", "")
                    value = int(fc_item)
                    if set_fc_type:
                        instr_trace_info.add_set_fc(value, desched)
                    else:
                        instr_trace_info.add_wfc_info(value, desched)


    # detail instructions
    def parse_mov_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.MOV)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            elif "mov" in line:
                comp_bytes = 4
                if "conversion_format=" in line:
                    fmtfield = line[line.find("conversion_format=") + len("conversion_format=") : ]
                    fmtfield = fmtfield[ :fmtfield.find(",")]
                    if "16" in fmtfield:
                        comp_bytes = 2
                    elif "8" in fmtfield:
                        comp_bytes = 1
                
                burst = 1
                if "burst_length=" in line:
                    burstfield = line[line.find("burst_length=") + len("burst_length=") : ]
                    burstfield = burstfield[ :burstfield.find(",")]
                    # if comp_bytes != 4:
                    #     burst = int(int(burstfield) / comp_bytes)
                    # else:
                    burst = int(burstfield)

                if "dest_reg=" in line:
                    destfield = line[line.find("dest_reg=") + len("dest_reg=") : ]
                    destfield = destfield[ :destfield.find(",")]
                    res_type_info, addr_info = self.resource.parse_res_type(destfield)
                    if self.resource.check_index_mode(destfield):
                        instr_trace_info.add_dest_info(res_type_info, addr_info)
                    else:
                        for idx in range(burst):
                            instr_trace_info.add_dest_info(res_type_info, int(addr_info) + idx)
                if "source_reg=" in line:
                    srcfield = line[line.find("source_reg=") + len("source_reg=") : ]
                    srcfield = srcfield[ :srcfield.find(",")]
                    srcfield = srcfield.replace(".A", "").replace(".B", "").replace(".C", "")
                    src_res_type_info, src_addr_info = self.resource.parse_res_type(srcfield)
                    if self.resource.check_index_mode(srcfield):
                        instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
                    else:
                        for idx in range(burst):
                            instr_trace_info.add_src_info(src_res_type_info, int(src_addr_info) + idx)



    def parse_int_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.INT)
        int_op_list = ["byp", "madd", "mul", "add", "f16_to_u8", "u8_to_f16"]
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            if "type " in line or "= result" in line:
                continue
            if "sr[" in line and "]" in line:
                continue
            if ".flr" in line:
                line = line.replace(".flr","")
            for intopitem in int_op_list:
                if "madd" in line and intopitem == "add":
                    continue
                if intopitem in line:
                    srcdest_info = line[line.find(intopitem) + len(intopitem): ].split(",")
                    for item in srcdest_info:
                        item = item.replace(" ","")
                        if "result" in item:
                            continue

                        srcitem = []
                        if ".neg" in item:
                            item = item.replace(".neg","")
                        if ".u32" in item or ".i32" in item:
                            if ".u32" in item:
                                srcitem.append(item.replace(".u32",""))
                            elif ".i32" in item:
                                srcitem.append(item.replace(".i32",""))
                        elif ".u64" in item or ".i64" in item:
                            if ".u64" in item:
                                item = item.replace(".u64","")
                            elif ".i64" in item:
                                item = item.replace(".i64","")
                            if "<<32" in item:
                                item = item.replace("<<32","")
                            srcitem = item.split("|")

                        for srcidxinfo in srcitem:
                            if "(sc" in srcidxinfo:
                                srcidxinfo = srcidxinfo[:srcidxinfo.find("(sc")]
                            if "(" in srcidxinfo:
                                srcidxinfo = srcidxinfo.replace("(","")
                            if ")" in srcidxinfo:
                                srcidxinfo = srcidxinfo.replace(")","")
                            src_res_type_info, src_addr_info = self.resource.parse_res_type(srcidxinfo)
                            if self.resource.check_index_mode(srcidxinfo):
                                instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
                            else:
                                instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
            # if "tst =" in line:
            #     print("tst")
            if "tst ? " in line:
                srcdest_info = line[line.find("tst ? ") + len("tst ? "): ].split(":")
                for item in srcdest_info:
                    item = item.replace(" ","")
                    if "result" in item:
                        continue
                    srcidxinfo = item[ :item.find("(")]
                    src_res_type_info, src_addr_info = self.resource.parse_res_type(srcidxinfo)
                    if self.resource.check_index_mode(srcidxinfo):
                        instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
                    else:
                        instr_trace_info.add_src_info(src_res_type_info, src_addr_info)

            if "d0=" in line or "d1=" in line:
                destfield = line[line.find("=") + len("=") : ]
                destfield = destfield.replace(" ","")
                res_type_info, addr_info = self.resource.parse_res_type(destfield)
                if self.resource.check_index_mode(destfield):
                    instr_trace_info.add_dest_info(res_type_info, addr_info)
                else:
                    instr_trace_info.add_dest_info(res_type_info, addr_info)
        #print("instr src=",instr_trace_info.src, "instr src type=",instr_trace_info.src_type, "instr dst=",instr_trace_info.dest, "instr dst type=",instr_trace_info.dest_type)
    
    def parse_bit_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.BIT)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            elif "s0 = " in line or "s1 = " in line or "s2 = " in line or "s3 = " in line:
                if "(sc" in line:
                    srcfield = line[line.find("= ") + len("= ") :line.find("/")]
                else:
                    srcfield = line[line.find("= ") + len("= ") :]
                src_res_type_info, src_addr_info = self.resource.parse_res_type(srcfield)
                if self.resource.check_index_mode(srcfield):
                    instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
                else:
                    instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
            elif "= ROT" in line:
                destfield = line[ :line.find(" = ROT")]
                res_type_info, addr_info = self.resource.parse_res_type(destfield)
                if self.resource.check_index_mode(destfield):
                    instr_trace_info.add_dest_info(res_type_info, addr_info)
                else:
                    instr_trace_info.add_dest_info(res_type_info, addr_info)
        #print("instr src=",instr_trace_info.src, "instr src type=",instr_trace_info.src_type, "instr dst=",instr_trace_info.dest, "instr dst type=",instr_trace_info.dest_type)


        
    def parse_pap_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.FOP)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            elif " = " in line:
                #print("line info = ", line)
                line = line.replace(".neg", "").replace(".sat", "").replace(".abs", "").replace(".flr", "")
                dest_info = line[: line.find(" = ")]
                srcs_info = line[line.find(" = ") + len(" = ") :]
                dest_info = dest_info.replace(" ", "")
                if ".f16" in dest_info:
                    dest_info = dest_info[:dest_info.find(".f16")]
                if ".e" in dest_info:
                    dest_info = dest_info[:dest_info.find(".e")]
                if "res" not in dest_info:
                    dest_res_type_info, dest_addr_info = self.resource.parse_res_type(dest_info)
                    instr_trace_info.add_dest_info(dest_res_type_info, dest_addr_info)
                srcs_list = []
                if ">" in srcs_info or "<" in srcs_info or "=" in srcs_info:
                    srcs_info = srcs_info.replace(">", ",").replace("<", ",").replace("=", ",").replace("!", ",").replace("res", ",")
                    srcs_info = srcs_info.replace(" ", "").split(",")
                    srcs_list = srcs_info
                elif "res" not in srcs_info:
                    srcs_list.append(srcs_info)
                for singlesrc_info in srcs_list:
                    if ".f16" in singlesrc_info:
                            singlesrc_info = singlesrc_info[:singlesrc_info.find(".f16")]
                    if ".e" in singlesrc_info:
                        singlesrc_info = singlesrc_info[:singlesrc_info.find(".e")]
                    if "(sc" in singlesrc_info:
                        singlesrc_info = singlesrc_info[:singlesrc_info.find("(sc")]
                    if singlesrc_info == '' or singlesrc_info == 'NaN':
                        continue
                    src_res_type_info, src_addr_info = self.resource.parse_res_type(singlesrc_info)
                    instr_trace_info.add_src_info(src_res_type_info, src_addr_info)

            elif "fma" in line or "mul" in line or "add" in line or "cmp" in line or "frc" in line or "ds" in line or "sw" in line or "mov" in line:
                itemcount = 0
                srcdst_info = line
                if "cmp_movc" in line:
                    srcdst_info = line[9: ]
                    if ">" in srcdst_info or "<" in srcdst_info or "=" in srcdst_info:
                        srcdst_info = srcdst_info.replace(">", ",").replace("<", ",").replace("=", ",")
                    srcdst_info = srcdst_info.replace("max", ",").replace("min", ",").replace("?", ",").replace(":", ",")
                    srcdst_info = srcdst_info.split(",")
                elif "dsdx" in line or "dsdy" in line or "swpx" in line or "swpy" in line or "movc" in line:
                    srcdst_info = line[5: ].split(",")
                else:
                    srcdst_info = line[4: ].split(",")

                #print("src_dst info", srcdst_info)
                for item in srcdst_info:
                    if "" == item:
                        continue
                    item = item.replace(" ", "")
                    item = item.replace(".neg", "").replace(".sat", "").replace(".abs", "").replace(".flr", "")
                    if ".f16" in item:
                            item = item[:item.find(".f16")]
                    if ".e" in item:
                        item = item[:item.find(".e")]
                    if "(sc" in item:
                        item = item[item.find("/") + len("/") : item.find("(sc")]
                    if "res" in item:
                        itemcount = itemcount + 1
                        continue
                    if itemcount == 0:
                        res_type_info, addr_info = self.resource.parse_res_type(item)
                        instr_trace_info.add_dest_info(res_type_info, addr_info)
                    else:
                        
                        res_type_info, addr_info = self.resource.parse_res_type(item)
                        instr_trace_info.add_src_info(res_type_info, addr_info)
                    itemcount = itemcount + 1
                    
        #print("instr src=",instr_trace_info.src, "instr src type=",instr_trace_info.src_type, "instr dst=",instr_trace_info.dest, "instr dst type=",instr_trace_info.dest_type)

        

    def parse_dma_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.DMA)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            if "ld" in line:
                instr_trace_info.set_instr_detail_info(instruction_type.DMA_LD)
            if "barrier" in line:
                instr_trace_info.set_instr_detail_info(instruction_type.DMA_BARRIER)
            if "base_address=" in line:
                burst = 1
                if "texelst" not in line and "atom" not in line and "tilest" not in line and "idf" not in line and "barrier" not in line:
                    burstxstr = line[line.find("burst_length_x=") + len("burst_length_x=") :]
                    burst_x = int(burstxstr[ :burstxstr.find(",")])  #isdecimal
                    burstystr = line[line.find("burst_length_y=") + len("burst_length_y=") :]
                    burst_y = int(burstystr[ :burstystr.find(",")])
                    burst = burst_y * ((burst_x + 3) >> 2)
                if "ld " in line or "atom" in line:
                    instr_trace_info.set_instr_detail_info(instruction_type.DMA_LD)
                    if "dest_reg=" in line:
                        destfield = line[line.find("dest_reg=") + len("dest_reg="): ]
                        destfield = destfield[ :destfield.find(",")]
                        res_type_info, addr_info = self.resource.parse_res_type(destfield)
                        if self.resource.check_index_mode(destfield):
                            instr_trace_info.add_dest_info(res_type_info, addr_info)
                        else:
                            for idx in range(burst):
                                instr_trace_info.add_dest_info(res_type_info, int(addr_info) + idx)
                if "st " in line or "atom" in line:
                    if "data_reg=" in line:
                        datafield = line[line.find("data_reg=") + len("data_reg="): ]
                        datafield = datafield[ :datafield.find(",")]
                        res_type_info, addr_info = self.resource.parse_res_type(datafield)
                        if self.resource.check_index_mode(datafield):
                            instr_trace_info.add_src_info(res_type_info, addr_info)
                        else:
                            for idx in range(burst):
                                instr_trace_info.add_src_info(res_type_info, int(addr_info) + idx)
                if "base_address=" in line:
                    srcfield = line[line.find("base_address=") + len("base_address=") : ]
                    srcfield = srcfield[ :srcfield.find(",")]
                    src_res_type_info, src_addr_info = self.resource.parse_res_type(srcfield)
                    if self.resource.check_index_mode(srcfield):
                        instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
                    else:
                        for idx in range(2):
                            instr_trace_info.add_src_info(src_res_type_info, int(src_addr_info) + idx)
                if "address_offset=" in line:
                    srcfield = line[line.find("address_offset=") + len("address_offset=") : ]
                    srcfield = srcfield[ :srcfield.find(",")]
                    if "_" in srcfield:
                        continue
                    src_res_type_info, src_addr_info = self.resource.parse_res_type(srcfield)
                    if self.resource.check_index_mode(srcfield):
                        instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
                    else:
                        for idx in range(1):
                            instr_trace_info.add_src_info(src_res_type_info, int(src_addr_info) + idx)
        #print("instr src=",instr_trace_info.src, "instr src type=",instr_trace_info.src_type, "instr dst=",instr_trace_info.dest, "instr dst type=",instr_trace_info.dest_type)
              

    def parse_tap_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.TAP)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            if "texture_address " in line:
                line = line.replace("texture_address ", "")
                line = line[line.find("dest_reg=") + len("dest_reg="): ]
                destfield = line[ :line.find(",")]
                res_type_info, addr_info = self.resource.parse_res_type(destfield)
                for idx in range(2):
                    instr_trace_info.add_dest_info(res_type_info, int(addr_info) + idx)
                predsrc = line[line.find(", ") + 2: line.find(", state_base_image")]
                src_res_type_info, src_addr_info = self.resource.parse_res_type(predsrc)
                instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
                
                src_info_list = ["state_base_image=", "state_imageoffset=", "texture_coordinates="]
                for item in src_info_list:
                    self.add_part_src_info(instr_trace_info, line, item, 1)
                


    def parse_smp_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.SMP)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            if "dest_reg=" in line:
                instr_trace_info.set_instr_detail_info(instruction_type.SMP_LD)
                destchfield = line[line.find("chan_count=x") + len("chan_count=x"): ]
                destch = int(destchfield[ :destchfield.find(",")])

                destfield = line[line.find("dest_reg=") + len("dest_reg="): ]
                destfield = destfield[ :destfield.find(",")]
                if "[" in destfield and "]" in destfield and ":" in destfield:
                    destfield = destfield.replace("[", "").replace("]", "")
                destlist = destfield.split(":")
                res_type_info, addr_info = self.resource.parse_res_type(destlist[0])
                for idx in range(destch):
                    instr_trace_info.add_dest_info(res_type_info, int(addr_info) + idx)
            
            src_info_list = ["data_reg=", "image_state=", "sampler_state=", "lod_reg="]
            for item in src_info_list:
                if item == "data_reg=":
                    self.add_part_src_info(instr_trace_info, line, item, 2)
                else:
                    self.add_part_src_info(instr_trace_info, line, item, 1)
        #print("instr src=",instr_trace_info.src, "instr src type=",instr_trace_info.src_type, "instr dst=",instr_trace_info.dest, "instr dst type=",instr_trace_info.dest_type)

    def parse_af32_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.AF32)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            if ".f32" in line:
                line = line.replace(".f32", "")
            if ".f16" in line:
                line = line.replace(".f16", "")
            if "dest_reg=" in line:
                destfield = line[line.find("dest_reg=") + len("dest_reg=") : ]
                destfield = destfield[ :destfield.find(",")]
                res_type_info, addr_info = self.resource.parse_res_type(destfield)
                if self.resource.check_index_mode(destfield):
                    instr_trace_info.add_dest_info(res_type_info, addr_info)
                else:
                    instr_trace_info.add_dest_info(res_type_info, int(addr_info))
            if "source_reg=" in line:
                line = line.replace(".neg", "").replace(".sat", "").replace(".abs", "")
                srcfield = line[line.find("source_reg=") + len("source_reg=") : ]
                if ";" in line:
                    line = line[:line.find("line = line.")]
                src_res_type_info, src_addr_info = self.resource.parse_res_type(srcfield)
                if self.resource.check_index_mode(srcfield):
                    instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
                else:
                    instr_trace_info.add_src_info(src_res_type_info, int(src_addr_info))
        #print("instr src=",instr_trace_info.src, "instr src type=",instr_trace_info.src_type, "instr dst=",instr_trace_info.dest, "instr dst type=",instr_trace_info.dest_type)



    def parse_ap_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.AP)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            if "atom" in line:
                dest_info = line[line.find("partial_result=") + len("partial_result="): line.find(", atomic_address")]
                if dest_info != "_":
                    dest_res_type_info, dest_addr_info = self.resource.parse_res_type(dest_info)
                    instr_trace_info.add_dest_info(dest_res_type_info, dest_addr_info)
                src_info_list = ["atomic_address=", "atomic_index=", "source_reg="]
                for item in src_info_list:
                    self.add_part_src_info(instr_trace_info, line, item, 1)

    
    def parse_psbwr_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.PSBWR)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            if "write" in line:
                srcdest_str = line[line.find("write ") + len("write "): ]
                srcdest_str = srcdest_str.replace(" ", "")
                srcdest_info = srcdest_str.split(",")
                for item in srcdest_info:
                    if "." in item:
                        item = item[:item.find(".")]
                    if "o" in item:
                        res_type_info, addr_info = self.resource.parse_res_type(item)
                        instr_trace_info.add_dest_info(res_type_info, int(addr_info))
                    else:
                        src_res_type_info, src_addr_info = self.resource.parse_res_type(item)
                        instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
            elif "blend" in line:
                srcdest_str = line[line.find("blend ") + len("blend "): ]
                deststr = srcdest_str[:srcdest_str.find(" = ")]
                src_str = srcdest_str[srcdest_str.find("= ") + len("= ") : ]
                if "o" in deststr:
                    if "." in deststr:
                        deststr = deststr[:deststr.find(".")]
                    res_type_info, addr_info = self.resource.parse_res_type(deststr)
                    instr_trace_info.add_dest_info(res_type_info, int(addr_info))
                #src
                src_info = src_str.split(" ")
                compbytes = 2 if "u16" in src_info or "i16" in src_info or "f16" in src_info else 1
                for srcidxinfo in src_info:
                    if "*" in srcidxinfo:
                        if "*o" in srcidxinfo:
                            srcidxinfo = srcidxinfo[srcidxinfo.find("*o") + len("*o") :]
                            if "." in srcidxinfo:
                                srcidxinfo = srcidxinfo[:srcidxinfo.find(".")]
                            instr_trace_info.add_src_info(res_type.pout, int(srcidxinfo)) 
                        else:
                            dwburst = 1 if "e2" in srcidxinfo or "e3" in srcidxinfo else 0
                            if compbytes == 1:
                                dwburst = 0
                            srcidxinfo = srcidxinfo[srcidxinfo.find("*") + len("*") :]
                            if "." in srcidxinfo:
                                srcidxinfo = srcidxinfo[:srcidxinfo.find(".")]
                            src_res_type_info, src_addr_info = self.resource.parse_res_type(srcidxinfo)
                            instr_trace_info.add_src_info(src_res_type_info, int(src_addr_info) + dwburst) 
        #print("instr src=",instr_trace_info.src, "instr src type=",instr_trace_info.src_type, "instr dst=",instr_trace_info.dest, "instr dst type=",instr_trace_info.dest_type)

    
    def parse_psbrd_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.PSBRD)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            if "read" in line:
                srcdest_info = line[line.find("read ") + len("read "): ].split(",")
                for item in srcdest_info:
                    if "o" in item:
                        srcfield = item[item.find("o") + len("o") : ]
                        instr_trace_info.add_src_info(res_type.pout, int(srcfield))
                    else:
                        dest_res_type_info, dest_addr_info = self.resource.parse_res_type(item)
                        instr_trace_info.add_dest_info(dest_res_type_info, dest_addr_info)


    def parse_itr_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.ITR)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            if "dest_reg=" in line:
                destfield = line[line.find("dest_reg=") + len("dest_reg=") : ]
                destfield = destfield.replace(".clamp", "")
                destfield = destfield[ :destfield.find(",")]
                res_type_info, addr_info = self.resource.parse_res_type(destfield)
                instr_trace_info.add_dest_info(res_type_info, int(addr_info))
            if "varying=" in line:
                srcfield = line[line.find("varying=") + len("varying=") : ]
                srcfield = srcfield[ :srcfield.find(",")]
                if "_" == srcfield:
                    continue
                res_type_info, addr_info = self.resource.parse_res_type(srcfield)
                instr_trace_info.add_src_info(res_type_info, int(addr_info))

        #print("instr src=",instr_trace_info.src, "instr src type=",instr_trace_info.src_type, "instr dst=",instr_trace_info.dest, "instr dst type=",instr_trace_info.dest_type)

    
    def parse_emi_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.EMI)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            if "isp" in line:
                instr_trace_info.set_instr_detail_info(instruction_type.EMI_ISP)
                if ".neg" in line:
                    line = line.replace(".neg", "")
                if ("isp_dstst " in line or "isp_kill_cfb " in line) and "," not in line:
                    if "isp_dstst " in line:
                        src_str = line[line.find("isp_dstst ") + len("isp_dstst "): ]
                    elif "isp_kill_cfb " in line:
                        src_str = line[line.find("isp_kill_cfb ") + len("isp_kill_cfb "): ]
                    src_res_type_info, src_addr_info = self.resource.parse_res_type(src_str)
                    instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
                else:
                    if "depth=" in line:
                        line = line[line.find("depth="): ]
                    line = line.replace(" ", "")
                    depthfields = line[line.find("depth=") + len("depth="): ].replace("coverage_mask=","").split(",")
                    for item in depthfields:
                        item = item[item.find("=") + len("="):]
                    res_type_info, addr_info = self.resource.parse_res_type(depthfields[0])
                    instr_trace_info.add_dest_info(res_type_info, addr_info)
                    src_res_type_info, src_addr_info = self.resource.parse_res_type(depthfields[1])
                    instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
                    if len(depthfields) > 2:
                        src_res_type_info, src_addr_info = self.resource.parse_res_type(depthfields[2])
                        instr_trace_info.add_src_info(src_res_type_info, src_addr_info)
            elif "uvb" in line:
                if "uvb_write" in line:
                    instr_trace_info.set_instr_detail_info(instruction_type.EMI_UVB)
                src_info_list = ["address=", "data="] #"stream"
                for item in src_info_list:
                    if item not in line:
                        continue
                    self.add_part_src_info(instr_trace_info, line, item, 1)
            

    
    def parse_ctrl_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.CTRL)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            if "limm" in line:
                destfield = line[line.find("limm= ") + len("limm= ") : ]
                destfield = destfield[ :destfield.find(",")]
                res_type_info, addr_info = self.resource.parse_res_type(destfield)
                if self.resource.check_index_mode(destfield):
                    instr_trace_info.add_dest_info(res_type_info, addr_info)
                else:
                    instr_trace_info.add_dest_info(res_type_info, int(addr_info))
            elif "br" in line or "ba" in line:
                instr_trace_info.set_instr_detail_info(instruction_type.CTRL_BRANCH)
                if "imm" in line:
                    immfield = line[line.find("(") + len("(") : ]
                    immfield = immfield[ :immfield.find(")")]
                    positive = True
                    if "+" in immfield:
                        immfield = immfield.replace("+","")
                    elif "-" in immfield:
                        immfield = immfield.replace("-","")
                        positive = False
                    immvalue = int(immfield) if positive else (int(-1) * int(immfield))
                    instr_trace_info.add_dest_info(res_type.imm, immvalue)

                else:
                    brfield = line[line.find(" ") + len(" ") : ]
                    src_res_type_info, src_addr_info = self.resource.parse_res_type(brfield)
                    instr_trace_info.add_src_info(src_res_type_info, src_addr_info)


    def parse_cond_instr(self, instr_trace_info):
        instr_trace_info.add_instr_type(instruction_type.COND)
        for line in self.instr_str:
            instr_trace_info.add_instr_str_info(line)
            if "type " in line:
                continue
            if "set = " in line or "wait = " in line:
                self.parse_fc_info(instr_trace_info, line, False)
                continue
            if "conditional_test=" in line:
                destfield = line[line.find("conditional_test=") + len("conditional_test=") : ]
                destfield = destfield[ :destfield.find(",")]
                destfield = destfield.replace(" ", "")
                if "." in destfield:
                    destfield = destfield[ :destfield.find(".")]
                if "always" == destfield:
                    continue
                res_type_info, addr_info = self.resource.parse_res_type(destfield)
                instr_trace_info.add_dest_info(res_type_info, addr_info)
            


    def parse_instr_info(self):
        instr_trace_info = instr_trace()
        instr_trace_info.set_instrid_bytes(self.instrid, self.instrbytes)
        if self.instr_str:
            instr_type = self.instr_str[0].lower()
            instr_type = instr_type[instr_type.find("type ") + len("type ") : ]
            if instr_type == instruction_type.BIT:
                self.parse_bit_instr(instr_trace_info)
            elif instr_type == instruction_type.INT:
                self.parse_int_instr(instr_trace_info)
            elif instr_type == instruction_type.FOP:
                self.parse_pap_instr(instr_trace_info)
            elif instr_type == instruction_type.MOV:
                self.parse_mov_instr(instr_trace_info)
            elif instr_type == instruction_type.DMA:
                self.parse_dma_instr(instr_trace_info)
            elif instr_type == instruction_type.SMP:
                self.parse_smp_instr(instr_trace_info)
            elif instr_type == instruction_type.TAP:
                self.parse_tap_instr(instr_trace_info)
            elif instr_type == instruction_type.ITR:
                self.parse_itr_instr(instr_trace_info)
            elif instr_type == instruction_type.EMI:
                self.parse_emi_instr(instr_trace_info)
            elif instr_type == instruction_type.PSBWR:
                self.parse_psbwr_instr(instr_trace_info)
            elif instr_type == instruction_type.PSBRD:
                self.parse_psbrd_instr(instr_trace_info)
            elif instr_type == instruction_type.AF32:
                self.parse_af32_instr(instr_trace_info)
            elif instr_type == instruction_type.AP:
                self.parse_ap_instr(instr_trace_info)
            elif instr_type == instruction_type.CTRL:
                self.parse_ctrl_instr(instr_trace_info)
            elif instr_type == instruction_type.COND:
                self.parse_cond_instr(instr_trace_info)
            else:
                print("parsing kind not specified = ", instr_type)
                assert(0)
        add_instr_into_trace_list(instr_trace_info)
    

#file parser
def parser_file(fileinput, fileout, cmdargs):
    global instr_trace_list
    open(fileout, 'w', encoding='utf-8').close()
    asm_file = open(fileinput, 'r', encoding='utf-8')
    lines = asm_file.readlines()

    bnew_shader = True
    bnew_instr = False
    bprocessed = False
    count = 0
    instr_infoobj = str_info()
    for line in lines:
        if re.match("^SM:.*", line) or re.match(".*Const Calc.*", line) or re.match(".*Main Program.*", line):
            output_shader_info(fileout, line)

        if bnew_shader == False and bprocessed == False and bnew_instr == False:
            output_instr_into_trace_list(fileout, cmdargs)
            bnew_shader = True
            bnew_instr = False
            bprocessed = False
            instr_trace_list.clear()
            count = 0


        if "Instr : " == line[:8]:
            bnew_instr = True
            if " Relative Address Offset" in line:
                instrid = int(line[line.find("Instr : ") + len("Instr : ") : line.find(" Relative Address Offset")])
                instr_bytestr = line[line.find("Bytes ") + len("Bytes ") : ]
                instr_bytes = int(instr_bytestr[: instr_bytestr.find(" ")])
                instr_infoobj.set_instrid(instrid, instr_bytes)
        elif "{\n" == line:
            continue 
        elif "}" == line[:1]:
            bnew_instr = False
            bprocessed = True
        else:
            line = line.replace("\t", "").replace("\n", "")
            instr_infoobj.add_instr_info(line)
            if bnew_instr and "end" == line:
                bnew_shader = False

        if bprocessed:
            instr_infoobj.parse_instr_info()
            count = count + 1
            
            bprocessed = False
    


# main
if __name__ == "__main__":
    args = parse_cmd()
    cmdinfo = cmd_info()
    if cmdinfo.parse_cmd_detail_info(args):
        cmdinfo.process_files()
    

