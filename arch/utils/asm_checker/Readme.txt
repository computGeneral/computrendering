ASM Checker guidence

1. ASM Checker used to parse instruction string info compiled from cpmpiler. 
If you want to parse the specify instruction file, please add command args:--specify-case-name=file_path/file_name.txt. The file can include lots of shaders. And the checker will check one by one.
At the same time, if the output path specified, the log will output to the same path of current input file with _log.txt suffix, like file_name_log.txt. 
If you want to parse batch instruction files, you can add the command args:--case-path=file_path
--instr-kind use to specify the instruction type shoule be traced. One of the kind is --instr-kind=longlatency. the long latency includes sample/dma_load and dma_atomic instructions.
For example:
python ASM_Checker.py --specify-case-name=C:\Users\lei.chen\Desktop\docs\research\PerformanceAnalysis\tool_test\test\shader_ps_25_crc_3147008375.txt
python ASM_Checker.py --case-path=C:\Users\lei.chen\Desktop\docs\research\PerformanceAnalysis\PixelShader --instr-kind=longlatency


2. The original instruction shows with vertical layout, some each instruction has lots fo lines.
But after parsing, it will be presented in one line. 
For example:
Instr : 7 Relative Address Offset : 0x0000000044 Bytes 12 : 0x0fe07cc0 0x07932248 0x840c0ad8 
{
	type int
	set = wfc0
	byp result.i32, sr[SAVMSK_ICM] (sr49).i32
	tst = result != sh2.i32
	d0.i32 = tst ? 0xFFFFFFFF (sc36) (s2) : 0x0 (sc0) (s3)
	d0=r18
}
the instruction will be converted as:
 instr  7 , bytes  12 , instr info: type int; set = wfc0; byp result.i32, sr[SAVMSK_ICM] (sr49).i32; tst = result != sh2.i32; d0.i32 = tst ? 0xFFFFFFFF (sc36) (s2) : 0x0 (sc0) (s3); d0=r18; 



3. long latency will be labeled with "*" at the head of the line. 
The source dependency will be presented by the command argument "--depend-depth". The info is showing behind as "--->" after current instruction.
The destination register will also be presented. The info is behind as "===>"
For example:
* instr  24 , bytes  12 , instr info: type smp; wait = wfc1; set = dfc0; smp2d.fcnorm dest_reg=r[6:8], data_reg=r9, image_state=sh4, sampler_state=sh1, chan_count=x3, swap=false, coord_format=f32, lod_reg=_, lod_mode=NORMAL, sb_mode=none, cache_coh=ccmcu, slc_cache_policy=NEW_ALLOC, cache_pri=0, swap=false, coord=f32; 
			--->  src trace...
			instr  15 , bytes  8 , instr info: type fop; movc r9, p0, 0x0/0f (sc0), i0; 
			instr  23 , bytes  12 , instr info: type fop; set = wfc1; movc r10, p0, 0x0/0f (sc0), i1; wait = dfc2; 
			===>  dest trace...
			instr  33 , bytes  12 , instr info: type fop; add r16, sh17, r6; i1 = sh17; 
			instr  35 , bytes  4 , instr info: type fop; add i0, i1, r7; 
			instr  36 , bytes  4 , instr info: type fop; add i1, i1, r8; 


4. After parsing the whole shader, the detail log info will be presented at the header of each shader in log file.
The log info includes max shared info, attr/temp usage info, uvb size info, active slots info.
For example:
-------------------------------------------------- 
Start to process shader...
max shared reg size is 18
coeffs supports waves number is < 30
attr size = 0, temp size = 20
active wave number is about 30
warning: long latency can't be hidded...24
total instructions number is 124,	long latency number is 7
 input temp reg:  r0,  r1,  r2,  r3,  r4,  r5,  r6, 
   output temp reg:  r0,  r1,  r2,  r3,  r4,  r5,  r6,  r7,  r8,  r9,  r10,  r11,  r12,  r13,  r14,  r15,  r16,  r17,  r18,  r19, 
   max temp position is 34
   needed max temp reg number is: 19
************************************************** 


5. The destination tracing info and active slots info can be used for tracing long latency.
for example:
* instr  24 , bytes  12 , instr info: type smp; wait = wfc1; set = dfc0; smp2d.fcnorm dest_reg=r[6:8], data_reg=r9, image_state=sh4, sampler_state=sh1, chan_count=x3, swap=false, coord_format=f32, lod_reg=_, lod_mode=NORMAL, sb_mode=none, cache_coh=ccmcu, slc_cache_policy=NEW_ALLOC, cache_pri=0, swap=false, coord=f32; 
			===>  dest trace...
			instr  33 , bytes  12 , instr info: type fop; add r16, sh17, r6; i1 = sh17; 
			instr  35 , bytes  4 , instr info: type fop; add i0, i1, r7; 
			instr  36 , bytes  4 , instr info: type fop; add i1, i1, r8;

