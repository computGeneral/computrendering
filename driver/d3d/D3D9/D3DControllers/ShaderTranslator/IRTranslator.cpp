
	#include "IRTranslator.h"

	using namespace std;
	using namespace arch;

	// Macros
	#ifdef D3D_DEBUG_ON
	    #define D3D_DEBUG(expr) { expr }
	#else
	    #define D3D_DEBUG(expr) { }
	#endif

	#undef IN
	#undef OUT

	// NOTE 1: For pixel shader versions < 0x0200 value of r0 is the color output according to specs.
	//         A MOV instruction is generated to put the value of the CG1 register that r0 is mapped to
	//         in the CG1 register that d3d color output is mapped to.


	//  NOTE 2: Extra code will be added to handle alpha test, this way:
	//            - Initially, references to color output will be mapped to a TEMP gpu register.
	//            - A MOV instruction is genererated if needed, see NOTE 1.
	//            - At the end of translation the color output will be unmapped from the TEMP and
	//              declared as usual, mapped to an OUTPUT gpu register.
	//            - Extra code will take gpu TEMP register as input and store result in gpu OUTPUT register.


	//  Constructor.
	IRTranslator::IRTranslator()
	{
	    
	    //  Create the map that translates the D3D9 write mask mode to the CG1 write mask mode.
	    DWORD W0 = D3DSP_WRITEMASK_0;
	    DWORD W1 = D3DSP_WRITEMASK_1;
	    DWORD W2 = D3DSP_WRITEMASK_2;
	    DWORD W3 = D3DSP_WRITEMASK_3;

	    map<DWORD, MaskMode> &mmm = maskModeMap;

	    mmm[0  | 0  | 0  | 0 ] = NNNN;
	    mmm[0  | 0  | 0  | W3] = NNNW;
	    mmm[0  | 0  | W2 | 0 ] = NNZN;
	    mmm[0  |  0 | W2 | W3] = NNZW;
	    mmm[0  | W1 | 0  | 0 ] = NYNN;
	    mmm[0  | W1 | 0  | W3] = NYNW;
	    mmm[0  | W1 | W2 | 0 ] = NYZN;
	    mmm[0  | W1 | W2 | W3] = NYZW;

	    mmm[W0 | 0  | 0  | 0 ] = XNNN;
	    mmm[W0 | 0  | 0  | W3] = XNNW;
	    mmm[W0 | 0  | W2 | 0 ] = XNZN;
	    mmm[W0 | 0  | W2 | W3] = XNZW;
	    mmm[W0 | W1 | 0  | 0 ] = XYNN;
	    mmm[W0 | W1 | 0  | W3] = XYNW;
	    mmm[W0 | W1 | W2 | 0 ] = XYZN;
	    mmm[W0 | W1 | W2 | W3] = mXYZW;

	    //  Create the table that translates dhe D3D9 swizzle mode to the CG1 swizzle mode.
	    DWORD xx = D3DVS_X_X;
	    DWORD xy = D3DVS_X_Y;
	    DWORD xz = D3DVS_X_Z;
	    DWORD xw = D3DVS_X_W;
	    DWORD yx = D3DVS_Y_X;
	    DWORD yy = D3DVS_Y_Y;
	    DWORD yz = D3DVS_Y_Z;
	    DWORD yw = D3DVS_Y_W;
	    DWORD zx = D3DVS_Z_X;
	    DWORD zy = D3DVS_Z_Y;
	    DWORD zz = D3DVS_Z_Z;
	    DWORD zw = D3DVS_Z_W;
	    DWORD wx = D3DVS_W_X;
	    DWORD wy = D3DVS_W_Y;
	    DWORD wz = D3DVS_W_Z;
	    DWORD ww = D3DVS_W_W;

	    map< DWORD, SwizzleMode > &mm = swizzleModeMap;

	    mm[xx|yx|zx|wx] = XXXX; mm[xx|yx|zx|wy] = XXXY;
	    mm[xx|yx|zx|wz] = XXXZ; mm[xx|yx|zx|ww] = XXXW;
	    mm[xx|yx|zy|wx] = XXYX; mm[xx|yx|zy|wy] = XXYY;
	    mm[xx|yx|zy|wz] = XXYZ; mm[xx|yx|zy|ww] = XXYW;
	    mm[xx|yx|zz|wx] = XXZX; mm[xx|yx|zz|wy] = XXZY;
	    mm[xx|yx|zz|wz] = XXZZ; mm[xx|yx|zz|ww] = XXZW;
	    mm[xx|yx|zw|wx] = XXWX; mm[xx|yx|zw|wy] = XXWY;
	    mm[xx|yx|zw|wz] = XXWZ; mm[xx|yx|zw|ww] = XXWW;

	    mm[xx|yy|zx|wx] = XYXX; mm[xx|yy|zx|wy] = XYXY;
	    mm[xx|yy|zx|wz] = XYXZ; mm[xx|yy|zx|ww] = XYXW;
	    mm[xx|yy|zy|wx] = XYYX; mm[xx|yy|zy|wy] = XYYY;
	    mm[xx|yy|zy|wz] = XYYZ; mm[xx|yy|zy|ww] = XYYW;
	    mm[xx|yy|zz|wx] = XYZX; mm[xx|yy|zz|wy] = XYZY;
	    mm[xx|yy|zz|wz] = XYZZ; mm[xx|yy|zz|ww] = XYZW;
	    mm[xx|yy|zw|wx] = XYWX; mm[xx|yy|zw|wy] = XYWY;
	    mm[xx|yy|zw|wz] = XYWZ; mm[xx|yy|zw|ww] = XYWW;

	    mm[xx|yz|zx|wx] = XZXX; mm[xx|yz|zx|wy] = XZXY;
	    mm[xx|yz|zx|wz] = XZXZ; mm[xx|yz|zx|ww] = XZXW;
	    mm[xx|yz|zy|wx] = XZYX; mm[xx|yz|zy|wy] = XZYY;
	    mm[xx|yz|zy|wz] = XZYZ; mm[xx|yz|zy|ww] = XZYW;
	    mm[xx|yz|zz|wx] = XZZX; mm[xx|yz|zz|wy] = XZZY;
	    mm[xx|yz|zz|wz] = XZZZ; mm[xx|yz|zz|ww] = XZZW;
	    mm[xx|yz|zw|wx] = XZWX; mm[xx|yz|zw|wy] = XZWY;
	    mm[xx|yz|zw|wz] = XZWZ; mm[xx|yz|zw|ww] = XZWW;

	    mm[xx|yw|zx|wx] = XWXX; mm[xx|yw|zx|wy] = XWXY;
	    mm[xx|yw|zx|wz] = XWXZ; mm[xx|yw|zx|ww] = XWXW;
	    mm[xx|yw|zy|wx] = XWYX; mm[xx|yw|zy|wy] = XWYY;
	    mm[xx|yw|zy|wz] = XWYZ; mm[xx|yw|zy|ww] = XWYW;
	    mm[xx|yw|zz|wx] = XWZX; mm[xx|yw|zz|wy] = XWZY;
	    mm[xx|yw|zz|wz] = XWZZ; mm[xx|yw|zz|ww] = XWZW;
	    mm[xx|yw|zw|wx] = XWWX; mm[xx|yw|zw|wy] = XWWY;
	    mm[xx|yw|zw|wz] = XWWZ; mm[xx|yw|zw|ww] = XWWW;

	//////////////////////////////////

	    mm[xy|yx|zx|wx] = YXXX; mm[xy|yx|zx|wy] = YXXY;
	    mm[xy|yx|zx|wz] = YXXZ; mm[xy|yx|zx|ww] = YXXW;
	    mm[xy|yx|zy|wx] = YXYX; mm[xy|yx|zy|wy] = YXYY;
	    mm[xy|yx|zy|wz] = YXYZ; mm[xy|yx|zy|ww] = YXYW;
	    mm[xy|yx|zz|wx] = YXZX; mm[xy|yx|zz|wy] = YXZY;
	    mm[xy|yx|zz|wz] = YXZZ; mm[xy|yx|zz|ww] = YXZW;
	    mm[xy|yx|zw|wx] = YXWX; mm[xy|yx|zw|wy] = YXWY;
	    mm[xy|yx|zw|wz] = YXWZ; mm[xy|yx|zw|ww] = YXWW;

	    mm[xy|yy|zx|wx] = YYXX; mm[xy|yy|zx|wy] = YYXY;
	    mm[xy|yy|zx|wz] = YYXZ; mm[xy|yy|zx|ww] = YYXW;
	    mm[xy|yy|zy|wx] = YYYX; mm[xy|yy|zy|wy] = YYYY;
	    mm[xy|yy|zy|wz] = YYYZ; mm[xy|yy|zy|ww] = YYYW;
	    mm[xy|yy|zz|wx] = YYZX; mm[xy|yy|zz|wy] = YYZY;
	    mm[xy|yy|zz|wz] = YYZZ; mm[xy|yy|zz|ww] = YYZW;
	    mm[xy|yy|zw|wx] = YYWX; mm[xy|yy|zw|wy] = YYWY;
	    mm[xy|yy|zw|wz] = YYWZ; mm[xy|yy|zw|ww] = YYWW;

	    mm[xy|yz|zx|wx] = YZXX; mm[xy|yz|zx|wy] = YZXY;
	    mm[xy|yz|zx|wz] = YZXZ; mm[xy|yz|zx|ww] = YZXW;
	    mm[xy|yz|zy|wx] = YZYX; mm[xy|yz|zy|wy] = YZYY;
	    mm[xy|yz|zy|wz] = YZYZ; mm[xy|yz|zy|ww] = YZYW;
	    mm[xy|yz|zz|wx] = YZZX; mm[xy|yz|zz|wy] = YZZY;
	    mm[xy|yz|zz|wz] = YZZZ; mm[xy|yz|zz|ww] = YZZW;
	    mm[xy|yz|zw|wx] = YZWX; mm[xy|yz|zw|wy] = YZWY;
	    mm[xy|yz|zw|wz] = YZWZ; mm[xy|yz|zw|ww] = YZWW;

	    mm[xy|yw|zx|wx] = YWXX; mm[xy|yw|zx|wy] = YWXY;
	    mm[xy|yw|zx|wz] = YWXZ; mm[xy|yw|zx|ww] = YWXW;
	    mm[xy|yw|zy|wx] = YWYX; mm[xy|yw|zy|wy] = YWYY;
	    mm[xy|yw|zy|wz] = YWYZ; mm[xy|yw|zy|ww] = YWYW;
	    mm[xy|yw|zz|wx] = YWZX; mm[xy|yw|zz|wy] = YWZY;
	    mm[xy|yw|zz|wz] = YWZZ; mm[xy|yw|zz|ww] = YWZW;
	    mm[xy|yw|zw|wx] = YWWX; mm[xy|yw|zw|wy] = YWWY;
	    mm[xy|yw|zw|wz] = YWWZ; mm[xy|yw|zw|ww] = YWWW;

	///////////////////////

	    mm[xz|yx|zx|wx] = ZXXX; mm[xz|yx|zx|wy] = ZXXY;
	    mm[xz|yx|zx|wz] = ZXXZ; mm[xz|yx|zx|ww] = ZXXW;
	    mm[xz|yx|zy|wx] = ZXYX; mm[xz|yx|zy|wy] = ZXYY;
	    mm[xz|yx|zy|wz] = ZXYZ; mm[xz|yx|zy|ww] = ZXYW;
	    mm[xz|yx|zz|wx] = ZXZX; mm[xz|yx|zz|wy] = ZXZY;
	    mm[xz|yx|zz|wz] = ZXZZ; mm[xz|yx|zz|ww] = ZXZW;
	    mm[xz|yx|zw|wx] = ZXWX; mm[xz|yx|zw|wy] = ZXWY;
	    mm[xz|yx|zw|wz] = ZXWZ; mm[xz|yx|zw|ww] = ZXWW;

	    mm[xz|yy|zx|wx] = ZYXX; mm[xz|yy|zx|wy] = ZYXY;
	    mm[xz|yy|zx|wz] = ZYXZ; mm[xz|yy|zx|ww] = ZYXW;
	    mm[xz|yy|zy|wx] = ZYYX; mm[xz|yy|zy|wy] = ZYYY;
	    mm[xz|yy|zy|wz] = ZYYZ; mm[xz|yy|zy|ww] = ZYYW;
	    mm[xz|yy|zz|wx] = ZYZX; mm[xz|yy|zz|wy] = ZYZY;
	    mm[xz|yy|zz|wz] = ZYZZ; mm[xz|yy|zz|ww] = ZYZW;
	    mm[xz|yy|zw|wx] = ZYWX; mm[xz|yy|zw|wy] = ZYWY;
	    mm[xz|yy|zw|wz] = ZYWZ; mm[xz|yy|zw|ww] = ZYWW;

	    mm[xz|yz|zx|wx] = ZZXX; mm[xz|yz|zx|wy] = ZZXY;
	    mm[xz|yz|zx|wz] = ZZXZ; mm[xz|yz|zx|ww] = ZZXW;
	    mm[xz|yz|zy|wx] = ZZYX; mm[xz|yz|zy|wy] = ZZYY;
	    mm[xz|yz|zy|wz] = ZZYZ; mm[xz|yz|zy|ww] = ZZYW;
	    mm[xz|yz|zz|wx] = ZZZX; mm[xz|yz|zz|wy] = ZZZY;
	    mm[xz|yz|zz|wz] = ZZZZ; mm[xz|yz|zz|ww] = ZZZW;
	    mm[xz|yz|zw|wx] = ZZWX; mm[xz|yz|zw|wy] = ZZWY;
	    mm[xz|yz|zw|wz] = ZZWZ; mm[xz|yz|zw|ww] = ZZWW;


	    mm[xz|yw|zx|wx] = ZWXX; mm[xz|yw|zx|wy] = ZWXY;
	    mm[xz|yw|zx|wz] = ZWXZ; mm[xz|yw|zx|ww] = ZWXW;
	    mm[xz|yw|zy|wx] = ZWYX; mm[xz|yw|zy|wy] = ZWYY;
	    mm[xz|yw|zy|wz] = ZWYZ; mm[xz|yw|zy|ww] = ZWYW;
	    mm[xz|yw|zz|wx] = ZWZX; mm[xz|yw|zz|wy] = ZWZY;
	    mm[xz|yw|zz|wz] = ZWZZ; mm[xz|yw|zz|ww] = ZWZW;
	    mm[xz|yw|zw|wx] = ZWWX; mm[xz|yw|zw|wy] = ZWWY;
	    mm[xz|yw|zw|wz] = ZWWZ; mm[xz|yw|zw|ww] = ZWWW;

	///////////////////////////////

	    mm[xw|yx|zx|wx] = WXXX; mm[xw|yx|zx|wy] = WXXY;
	    mm[xw|yx|zx|wz] = WXXZ; mm[xw|yx|zx|ww] = WXXW;
	    mm[xw|yx|zy|wx] = WXYX; mm[xw|yx|zy|wy] = WXYY;
	    mm[xw|yx|zy|wz] = WXYZ; mm[xw|yx|zy|ww] = WXYW;
	    mm[xw|yx|zz|wx] = WXZX; mm[xw|yx|zz|wy] = WXZY;
	    mm[xw|yx|zz|wz] = WXZZ; mm[xw|yx|zz|ww] = WXZW;
	    mm[xw|yx|zw|wx] = WXWX; mm[xw|yx|zw|wy] = WXWY;
	    mm[xw|yx|zw|wz] = WXWZ; mm[xw|yx|zw|ww] = WXWW;

	    mm[xw|yy|zx|wx] = WYXX; mm[xw|yy|zx|wy] = WYXY;
	    mm[xw|yy|zx|wz] = WYXZ; mm[xw|yy|zx|ww] = WYXW;
	    mm[xw|yy|zy|wx] = WYYX; mm[xw|yy|zy|wy] = WYYY;
	    mm[xw|yy|zy|wz] = WYYZ; mm[xw|yy|zy|ww] = WYYW;
	    mm[xw|yy|zz|wx] = WYZX; mm[xw|yy|zz|wy] = WYZY;
	    mm[xw|yy|zz|wz] = WYZZ; mm[xw|yy|zz|ww] = WYZW;
	    mm[xw|yy|zw|wx] = WYWX; mm[xw|yy|zw|wy] = WYWY;
	    mm[xw|yy|zw|wz] = WYWZ; mm[xw|yy|zw|ww] = WYWW;

	    mm[xw|yz|zx|wx] = WZXX; mm[xw|yz|zx|wy] = WZXY;
	    mm[xw|yz|zx|wz] = WZXZ; mm[xw|yz|zx|ww] = WZXW;
	    mm[xw|yz|zy|wx] = WZYX; mm[xw|yz|zy|wy] = WZYY;
	    mm[xw|yz|zy|wz] = WZYZ; mm[xw|yz|zy|ww] = WZYW;
	    mm[xw|yz|zz|wx] = WZZX; mm[xw|yz|zz|wy] = WZZY;
	    mm[xw|yz|zz|wz] = WZZZ; mm[xw|yz|zz|ww] = WZZW;
	    mm[xw|yz|zw|wx] = WZWX; mm[xw|yz|zw|wy] = WZWY;
	    mm[xw|yz|zw|wz] = WZWZ; mm[xw|yz|zw|ww] = WZWW;


	    mm[xw|yw|zx|wx] = WWXX; mm[xw|yw|zx|wy] = WWXY;
	    mm[xw|yw|zx|wz] = WWXZ; mm[xw|yw|zx|ww] = WWXW;
	    mm[xw|yw|zy|wx] = WWYX; mm[xw|yw|zy|wy] = WWYY;
	    mm[xw|yw|zy|wz] = WWYZ; mm[xw|yw|zy|ww] = WWYW;
	    mm[xw|yw|zz|wx] = WWZX; mm[xw|yw|zz|wy] = WWZY;
	    mm[xw|yw|zz|wz] = WWZZ; mm[xw|yw|zz|ww] = WWZW;
	    mm[xw|yw|zw|wx] = WWWX; mm[xw|yw|zw|wy] = WWWY;
	    mm[xw|yw|zw|wz] = WWWZ; mm[xw|yw|zw|ww] = WWWW;

	    //  Translator state inicializations.
	    alpha_func = D3DCMP_ALWAYS;
	}

	//  Definition node visitor.
	void IRTranslator::visit(DefinitionIRNode *n)
	{
	    //  Get the destionation register of the definition.
	    DestinationParameterIRNode *dest = n->getDestination();
	    
	    //  Create the D3D9 register identifier for the destination register.
	    D3DRegisterId d3dreg(dest->getNRegister(), dest->getRegisterType());
	    
	    //  Check the type of destination register.
	    switch(dest->getRegisterType())
	    {
		case D3DSPR_CONST:
		    {        
			//  Read the 4 tokens with the values defined for the destination register.
			float ftokens[4];
			for(U32 i = 0; i < 4; i++)
			{
			    FloatIRNode *f = static_cast<FloatIRNode *>(n->getChilds()[1 + i]);
			    ftokens[i] = f->getValue();
			}
			
			//  Create a constant value from the values recieved 
			ConstValue value(ftokens);
			
			//  Declare and set the value of the D3D9 constant register and map to an CG1 constant register.
			declareMapAndReserveConst(d3dreg, value);
		    }
				
		    break;
		    
		case D3DSPR_CONST2:
		case D3DSPR_CONST3:
		    CG_ASSERT("Extra float point constant register spaces not implemented.");
		    break;

		case D3DSPR_CONSTBOOL:
		    {        
			//  Read 1 token with the value defined for the destination register.
			bool btoken;
			BoolIRNode *b = static_cast<BoolIRNode *>(n->getChilds()[1]);
			btoken = b->getValue();
			
			//  Create a constant value from the values recieved 
			ConstValue value(btoken);
			
			//  Declare and set the value of the D3D9 constant register and map to an CG1 constant register.
			declareMapAndReserveConst(d3dreg, value);
		    }
		    break;

		case D3DSPR_CONSTINT:
		    {        
			//  Read the 4 tokens with the values defined for the destination register.
			S32 intTokens[4];
			for(U32 i = 0; i < 4; i++)
			{
			    IntegerIRNode *t = static_cast<IntegerIRNode *>(n->getChilds()[1 + i]);
			    intTokens[i] = t->getValue();
			}
			
			//  Create a constant value from the values recieved 
			ConstValue value(intTokens);
			
			//  Declare and set the value of the D3D9 constant register and map to an CG1 constant register.
			declareMapAndReserveConst(d3dreg, value);
		    }
		    break;
			 
		default:
		    CG_ASSERT("Register type doesn't support value definition.");
		    break;
	    }            
	}

	//  Visitor for end node.
	void IRTranslator::visit(EndIRNode *n)
	{

	    //  Check for pixel shader 1.x.
	    if((type == PIXEL_SHADER) && (version < 0x0200))
	    {
		//  The D3D9 r0 register (temporary register) is aliased to the color output register.
		//  Generate a MOV instruction from the temporary register to the actual color output register.
		generateMov(mappedRegister(D3DRegisterId(0,D3DSPR_COLOROUT)), mappedRegister(D3DRegisterId(0, D3DSPR_TEMP)));
	    }
	    
	    //  Check for pixel shader and alpha test emulation.
	    if((type == PIXEL_SHADER) && (alpha_func != D3DCMP_ALWAYS))
	    {        
		//  When alpha test emulation is required the D3D9 color output register from the pixel shader is mapped
		//  to a temporary register.  Before generating the instructions for alpha test emulation
		//  the D3D9 color output register is unmapped from the temporary register and mapped to the
		//  actual CG1 color output register.
		
		//  Unmap color output from temp register
		GPURegisterId color_temp = unmapRegister(D3DRegisterId(0, D3DSPR_COLOROUT));
		releaseTemp(color_temp);
		
		//  Reserve temp register again to forbid extra code generation to use it as temp.
		reserveTemp(color_temp);
		
		//  Declare color output as usual
		GPURegisterId color_out = declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_COLOR, 0), D3DRegisterId(0,D3DSPR_COLOROUT));
		
		//  Generate the extra code to emulate the alpha test.
		generate_extra_code(color_temp, color_out);
	    }

	    //ShaderInstructionBuilder builder;
	    //builder.setOpcode(END);
	    //instructions.push_back(builder.buildInstruction());

	    //  Set end flag in the last shader instruction.
	    instructions.back()->setEndFlag(true);

	    // NOTE: All D3D9 constant registers must be declared because with VS relative
	    //       addressing not only constants that appear in the code are used.
	    // TODO: Is this really required?  Use a flag checking for relative addressing mode.
	    //       Take care with integration with alpha test/fog code because they also use some constants.
	    //       Pixel shader modes that support relative addressing.    
	    /*if(type == VERTEX_SHADER)
	    {
		for (U32 c = 0; c < 256; c ++)
		{
		    if (!isMapped(D3DRegisterId(c, D3DSPR_CONST)))
		    {
			declareMapAndReserveConst(D3DRegisterId(c, D3DSPR_CONST));
		    }
		}
	    }*/
	}

	//  Generate a MOV shader instruction that copies the value from a source register to a destination register.
	void IRTranslator::generateMov(GPURegisterId dest, GPURegisterId src)
	{
	    ShaderInstructionBuilder builder;
	    builder.setOpcode(MOV);
	    Operand op;
	    op.registerId = src;
	    builder.setOperand(0, op);
	    Result res;
	    res.registerId = dest;
	    builder.setResult(res);
	    instructions.push_back(builder.buildInstruction());
	}

	//  Release an CG1 temporary register.
	void IRTranslator::releaseTemp(GPURegisterId tmp)
	{
	    //  Check that the register being released is a temporary register.
	    if(tmp.bank != TEMP)
		CG_ASSERT("Trying to release a non temporary register");
		
	    //  Add to the list of available temporary registers.
	    availableTemp.insert(tmp);
	}

	//  Reserves an CG1 temporary register and maps the D3D9 register to the CG1 temporary
	//  register.
	GPURegisterId IRTranslator::reserveAndMapTemp(D3DRegisterId d3dreg)
	{
	    //  Reserve a temporary register.
	    GPURegisterId reserved = reserveTemp();
	    
	    //  Map the D3D9 register to the CG1 temporary register.
	    mapRegister(d3dreg, reserved);
	    return reserved;
	}

	//  Reserves an CG1 temporary register.
	GPURegisterId IRTranslator::reserveTemp(GPURegisterId gpu_temp)
	{
	    //  Check that the input register is a temporary register.
	    if (gpu_temp.bank != arch::TEMP)
		CG_ASSERT("Trying to declare a non temporary register as temporary register.");
	    
	    //  Check if the temporary register requested is available.
	    set<GPURegisterId>::iterator itT = availableTemp.find(gpu_temp);
	    if (itT == availableTemp.end())
		CG_ASSERT("Trying to declare a reserved temp register.");
		
	    //  Remove the temporary register from the list of available temporary registers.
	    availableTemp.erase(itT);
	    
	    return gpu_temp;
	}


	//  Reserves an CG1 temporary register.
	GPURegisterId IRTranslator::reserveTemp()
	{
	    //  Check if there are available temporary registers.
	    if (availableTemp.begin() == availableTemp.end())    
		CG_ASSERT("Run out of temporary registers");
		
	    //  Get the next available temporary register.
	    set<GPURegisterId>::iterator itN = availableTemp.begin();
	    GPURegisterId declared = *itN;
	    
	    //  Remove the temporary register from the list of available temporary registers.
	    availableTemp.erase(itN);

	    return declared;
	}

	//  Releases a predicate register.
	void IRTranslator::releasePredicate(GPURegisterId pred)
	{
	    //  Check the type of the requested register.
	    if(pred.bank != arch::PRED)
		CG_ASSERT("Trying to undeclare a non temp register");
	    
	    //  Add to the list of available predicate registers.
	    availablePredicates.insert(pred);
	}

	//  Reserv and map a predicate register.
	GPURegisterId IRTranslator::reserveAndMapPredicate(D3DRegisterId d3dreg)
	{
	    //  Reserve a predicate register.
	    GPURegisterId reserved = reservePredicate();
	    
	    //  Map the D3D9 register to the CG1 predicate register.
	    mapRegister(d3dreg, reserved);
	    
	    return reserved;
	}

	//  Reserve a predicate register.
	GPURegisterId IRTranslator::reservePredicate(GPURegisterId pred)
	{
	    //  Check the type of the requested register.
	    if(pred.bank != arch::PRED)
		CG_ASSERT("Trying to declare a non temp register as temp");
	    
	    //  Search for the register in the list of available predicate registers.
	    set<GPURegisterId>::iterator itT = availablePredicates.find(pred);
	    
	    //  Check if the predicate register is no longer available.
	    if(itT == availablePredicates.end())
		CG_ASSERT("Trying to declare a reserved temp register");
		
	    //  Remove the predicate register from the list of available predicate registers.
	    availablePredicates.erase(itT);
	    
	    return pred;
	}


	//  Reserve a predicate register.
	GPURegisterId IRTranslator::reservePredicate()
	{
	    //  Check if there are no available predicate registers.
	    if (availablePredicates.begin() == availablePredicates.end())
		CG_ASSERT("Run out of predicate registers");
		
	    //  Get the next available predicate register.
	    set<GPURegisterId>::iterator itN = availablePredicates.begin();
	    GPURegisterId declared = *itN;
	    
	    //  Remove the predicate register from the list of available predicate registers.    
	    availablePredicates.erase(itN);

	    return declared;
	}

	//  Declares a D3D9 sampler register, reserves an CG1 texture register and maps the D3D9 sampler
	//  register to the CG1 texture register.
	GPURegisterId IRTranslator::declareMapAndReserveSampler(D3DSAMPLER_TEXTURE_TYPE type, D3DRegisterId d3d_register)
	{
	    //  Direct mapping from D3D9 samplers to CG1 texture units.
	    
	    //  Check the range of the D3D9 sampler.
	    if (d3d_register.num >= MAX_TEXTURES)
		CG_ASSERT("Sampler identifier out of range.");
	    
	    //  Create the corresponding CG1 texture register.
	    GPURegisterId declared;
	    declared.bank = arch::TEXT;
	    declared.num = d3d_register.num;
	    
	    //  Declare the D3D9 sampler register.
	    samplerDeclaration.push_back(SamplerDeclaration(d3d_register.num, declared.num, type));

	    //  Map the D3D9 sampler register to the CG1 texture register.
	    mapRegister(d3d_register, declared);

	    return declared;
	}

	//  Declares a D3D9 register as output register with a given output usage, reserves an CG1 register
	//  and maps the D3D9 register to the CG1 register.
	GPURegisterId IRTranslator::declareMapAndReserveOutput(D3DUsageId usage, D3DRegisterId d3d_register)
	{
	    //  Mapping of output registers to CG1 register is fixed and depends on the output usage.
	    
	    //  Get the CG1 register associated with the given output usage.
	    GPURegisterId reserved;
	    reserved = outputUsage[usage];

	    //  Check if there is an CG1 output register reserved for the requested output semantic usage.
	    if (reserved.bank == arch::INVALID)
		CG_ASSERT("Output semantic usage is not mapped to an CG1 output register.");
	    
	    //  Declare the output.
	    outputDeclaration.push_back(OutputRegisterDeclaration(usage.usage, usage.index, reserved.num));

	    //  Map the D3D9 register to the CG1 register.
	    mapRegister(d3d_register, reserved);

	    return reserved;
	}

	//  Declares a D3D9 register as input register with a given input ussage, reserves an CG1 register
	//  and maps the D3D9 register to the CG1 register.
	GPURegisterId IRTranslator::declareMapAndReserveInput(D3DUsageId usage, D3DRegisterId d3d_register)
	{
	    GPURegisterId reserved;

	    //  Check shader type.
	    if(type == VERTEX_SHADER)
	    {
		// Get the next available CG1 input register
		set<GPURegisterId>::iterator it = availableInput.begin();        
		reserved = *it;
		
		//  Remove the CG1 input register from the list of available input registers.
		availableInput.erase(it);
	    }
	    else
	    { 
		// PIXEL_SHADER
		
		//  Check shader version.        
		if(version < 0x0300)
		{
		    //  For PS 1.x and 2.x usage is defined by register type and number.
		    switch (d3d_register.type)
		    {
			case D3DSPR_INPUT:
			    usage.index = d3d_register.num;
			    usage.usage = D3DDECLUSAGE_COLOR;
			    break;
			case D3DSPR_TEXTURE:
			    usage.index = d3d_register.num;
			    usage.usage = D3DDECLUSAGE_TEXCOORD;
			    break;
			default:
			    CG_ASSERT("Undefined input register type for pixel shader 1.x and 2.x");
			    break;
		    }
		}
		
		//  For pixel shaders the mapping to CG1 input registers is fixed.
		
		//  Get the CG1 input register associated with the input usage.
		reserved = inputUsage[usage];

		//  Check if there is an CG1 output register reserved for the requested output semantic usage.
		if (reserved.bank == arch::INVALID)
		    CG_ASSERT("Input semantic usage is not mapped to an CG1 input register.");

		//  Remove the CG1 input register from the list of available CG1 input registers.
		availableInput.erase(reserved);
	    }

	    //  Declare the input.
	    inputDeclaration.push_back(InputRegisterDeclaration(usage.usage, usage.index, reserved.num));
	    
	    //  Map the D3D9 input register to the CG1 register.
	    mapRegister(d3d_register, reserved);

	    return reserved;
	}

	//  Checks if a D3D9 register is mapped to an CG1 register.
	bool IRTranslator::isMapped(D3DRegisterId d3dreg)
	{
	    return registerMap.find(d3dreg) != registerMap.end();
	}

	//  Reserves an CG1 register and map the D3D9 register to the reserved CG1 register.
	GPURegisterId IRTranslator::mapAndReserveRegister(D3DRegisterId d3dreg)
	{
	    // Check if the D3D9 register is already mapped to an CG1 register.
	    if (registerMap.find(d3dreg) != registerMap.end())
		CG_ASSERT("Register already declared.");

	    GPURegisterId reserved;
    
    //  Check the D3D9 register type.
    switch(d3dreg.type)
    {
        case D3DSPR_CONST:
        
            //  Declare, reserve and map a constnat register.
            reserved = declareMapAndReserveConst(d3dreg);
            break;
            
        case D3DSPR_TEMP:
        
            //  Reserve and map a temporary register.
            reserved = reserveAndMapTemp(d3dreg);
            break;
            
        default:
            
            D3D_DEBUG( cout << "IRTranslator: WARNING: D3D register type not supported" << endl; )
            ///@note The intention is to avoid halting simulation
            reserved = GPURegisterId(0, arch::INVALID);
            error = true;
            break;
    }
    
    return reserved;
}

//  Maps a D3D9 register to an CG1 register.
void IRTranslator::mapRegister(D3DRegisterId d3dreg, GPURegisterId gpureg)
{
    // Check if the D3D9 register is already mapped to an CG1 register.
    if (registerMap.find(d3dreg) != registerMap.end())
        CG_ASSERT("Register already mapped.");

    //  Map the D3D9 register to the CG1 register.
    registerMap[d3dreg] = gpureg;
}

//  Remove the mapping of D3D9 register to an CG1 register.
GPURegisterId IRTranslator::unmapRegister(D3DRegisterId d3dreg)
{
    //  Find the mapping for the D3D9 register.
    map<D3DRegisterId, GPURegisterId>::iterator it;
    it = registerMap.find(d3dreg);
    
    // Check if the D3D9 register was not mapped to an CG1 register.
    if (it == registerMap.end())
        CG_ASSERT("Register not mapped.");

    //  Get CG1 register to which the D3D9 register is mapped.
    GPURegisterId result = (*it).second;
    
    //  Remove the mapping.
    registerMap.erase(it);
    
    return result;
}

//  Declare a D3D9 constant register, reserve an CG1 constant register and map the D3D9 register to the
//  CG1 constant register.
GPURegisterId IRTranslator::declareMapAndReserveConst(D3DRegisterId d3dreg)
{
    // NOTE: D3D constants register is assigned to a gpu register keeping the same number.
    //       this is important because constants are sometimes accesed in indexed mode,
    //       so the order has to be the same.

    GPURegisterId reserved;
    reserved.bank = arch::PARAM;
    
    //  Check the type of constant register.
    switch(d3dreg.type)
    {
        case D3DSPR_CONST:
        
            //  Get the CG1 constant register to which the D3D9 constant register is mapped (direct mapping).
            reserved.num = d3dreg.num;
            break;

        case D3DSPR_CONST2:
        case D3DSPR_CONST3:
            CG_ASSERT("Extra float point constant space not implemented.");
            break;
            
        case D3DSPR_CONSTINT:

            //  Get the CG1 constant register to which the D3D9 constant register is mapped (direct mapping).
            reserved.num = d3dreg.num + INTEGER_CONSTANT_START;
            break;

        case D3DSPR_CONSTBOOL:

            //  Get the CG1 constant register to which the D3D9 constant register is mapped (direct mapping).
            reserved.num = d3dreg.num + BOOLEAN_CONSTANT_START;
            break;
    }

    //  Check if the CG1 constant register is available.    
    set<GPURegisterId>::iterator it = availableConst.find(reserved);
    if(it == availableConst.end())
    {
        char buffer[128];
        sprintf(buffer, "Constant %d not available.", d3dreg.num);
        CG_ASSERT(buffer);
    }
    
    //  Remove the CG1 constant register from the list of available CG1 constant registers.
    availableConst.erase(it);

    //  Map the D3D9 constant register to the CG1 constant register.
    mapRegister(d3dreg, reserved);

    //  Declare the constant with default value.
    ConstValue value;
    constDeclaration.push_back(ConstRegisterDeclaration(reserved.num, d3dreg.num, false, value));
    
    return reserved;
}

//  Declare a D3D9 constant register with the defined value, reserve the CG1 constant register
//  and map the D3D9 register to the CG1 constant register.
GPURegisterId IRTranslator::declareMapAndReserveConst(D3DRegisterId d3dreg, ConstValue value)
{
    //  NOTE: D3D constants register is assigned to a gpu register keeping the same number.
    //        this is important because constants are sometimes accesed in indexed mode,
    //        so the order has to be the same.

    GPURegisterId reserved;
    reserved.bank = arch::PARAM;
    
    //  Check the type of constant register.
    switch(d3dreg.type)
    {
        case D3DSPR_CONST:
        
            //  Get the CG1 constant register to which the D3D9 constant register is mapped (direct mapping).
            reserved.num = d3dreg.num;
            break;

        case D3DSPR_CONST2:
        case D3DSPR_CONST3:
            CG_ASSERT("Extra float point constant space not implemented.");
            break;
            
        case D3DSPR_CONSTINT:

            //  Get the CG1 constant register to which the D3D9 constant register is mapped (direct mapping).
            reserved.num = d3dreg.num + INTEGER_CONSTANT_START;
            break;

        case D3DSPR_CONSTBOOL:

            //  Get the CG1 constant register to which the D3D9 constant register is mapped (direct mapping).
            reserved.num = d3dreg.num + BOOLEAN_CONSTANT_START;
            break;
    }

    //  Check if the CG1 constant register is available.    
    set<GPURegisterId>::iterator it = availableConst.find(reserved);
    if (it == availableConst.end())
        CG_ASSERT("Constant not available");
    
    //  Remove the CG1 constant register from the list of available CG1 constant registers.
    availableConst.erase(it);

    //  Map the D3D9 constant register to the CG1 constant register.
    mapRegister(d3dreg, reserved);

    //  Declare the constant with the defined value.
    constDeclaration.push_back(ConstRegisterDeclaration(reserved.num, d3dreg.num, true, value));
    
    return reserved;
}

//  Initialize the translation of the D3D9 shader program.
void IRTranslator::begin()
{
    //  Clear declarations.
    inputDeclaration.clear();
    outputDeclaration.clear();
    constDeclaration.clear();
    samplerDeclaration.clear();
    
    //  Set default fixed function state.
    alpha_test_declaration.comparison = alpha_func;
    alpha_test_declaration.alpha_const_minus_one = 0;
    alpha_test_declaration.alpha_const_ref = 0;
    fog_declaration.fog_enabled = fog_enabled;
    
    //  Set information about the current translation.
    untranslatedControlFlow = false;
    untranslatedInstructions = 0;
    insideIFBlock = false;
    insideELSEBlock = false;
    currentPredication.predicated = false;
    currentPredication.negatePredicate = false;
    currentPredication.predicateRegister = 0;
    insideREPBlock = false;
    
    //  Delete the shader instructions from previous translations.
    delete_instructions();
}

//  Destructor.
IRTranslator::~IRTranslator()
{
    //  Delete the shader instructions from previous translations.
    delete_instructions();
}

//  Delete the shader instructions from previous translations.
void IRTranslator::delete_instructions()
{
    list<ShaderInstruction *>::iterator it_i;
    for(it_i = instructions.begin(); it_i != instructions.end(); it_i++)
        delete(*it_i);
    instructions.clear();
}

//  Creates a NativeShader object with the shader translation and the associated information.
NativeShader *IRTranslator::build_native_shader()
{
    //  Compute the size of the CG1 shader program in bytes.
    U32 lenght = 16 * instructions.size();
    
    //  Allocate memory for the CG1 shader program.
    U08 *bytecode = new U08[lenght];
    
    list<ShaderInstruction *>::iterator it;
    U32 offset = 0;

    //  Get the bytecode for the CG1 shader program.
    for(it = instructions.begin(); it != instructions.end(); it++)
    {
        //  Get the bytecode for the current instruction.
        (*it)->getCode(&bytecode[offset]);
        offset += 16;
    }

    //  Create declaration information for the shader program.
    NativeShaderDeclaration declaration(constDeclaration, inputDeclaration,
                                        outputDeclaration, samplerDeclaration,
                                        alpha_test_declaration, fog_declaration);
                                        
    return new NativeShader(declaration, bytecode, lenght);
}

//  Return a reference to the list of CG1 shader instructions for the translated program.
std::list<ShaderInstruction *> &IRTranslator::get_instructions()
{
    return instructions;
}

//  Translate the D3D9 write mask mode to the corresponding CG1 write mask mode.
MaskMode IRTranslator::nativeMaskMode(DWORD d3dmask)
{
    //  Search for the corresponding CG1 write mask mode.
    map<DWORD, MaskMode>::iterator itM = maskModeMap.find(d3dmask);
    
    //  Check if a translation was found.    
    if (itM == maskModeMap.end())
        CG_ASSERT("Unknown d3d mask");
    
    return (*itM).second;
}

//  Translate the D3D9 swizzle mode to the corresponding CG1 swizzle mode.
SwizzleMode IRTranslator::nativeSwizzleMode(DWORD d3dswizz)
{
    //  Search for the corresponding CG1 swizzle mode.
    map<DWORD, SwizzleMode>::iterator itS = swizzleModeMap.find(d3dswizz);
    
    //  Check if a translation was found.
    if (itS != swizzleModeMap.end())
        return (*itS).second;
    else
        return XYZW;
}

//  Initialize the shader registers based on the shader type and version.
void IRTranslator::initializeRegisters()
{
    // NOTE: If alpha test is enabled d3d two constant registers are not mapped to any gpu param register
    //       because they are used to store alpha reference and (-1, -1, -1, -1)    

    //  Allocate constant registers.
    availableConst.clear();
    for(U32 c = 0; c < UNIFIED_CONSTANT_NUM_REGS; c++)
        availableConst.insert(GPURegisterId(c, arch::PARAM));

    //  Allocate temporary registers.
    availableTemp.clear();
    for(U32 r = 0; r < UNIFIED_TEMPORARY_NUM_REGS; r++)
        availableTemp.insert(GPURegisterId(r, arch::TEMP));
    
    //  Allocate input registers.
    availableInput.clear();
    for(U32 r = 0; r < UNIFIED_INPUT_NUM_REGS; r++)
        availableInput.insert(GPURegisterId(r, arch::IN));
    
    //  Allocate output registers.
    availableOutput.clear();
    for(U32 r = 0; r < UNIFIED_OUTPUT_NUM_REGS; r++)
        availableOutput.insert(GPURegisterId(r, arch::OUT));

    //  Allocate predicate registers.
    availablePredicates.clear();
    for(U32 p = 0; p < UNIFIED_PREDICATE_NUM_REGS; p++)
        availablePredicates.insert(GPURegisterId(p, arch::PRED));

    //  Allocate texture units/samplers.
    availableSamplers.clear();
    for(U32 t = 0; t < MAX_TEXTURES; t++)
        availableSamplers.insert(GPURegisterId(t, arch::TEXT));
    
    //  Clear the mapping of D3D9 registers to CG1 regsiters.
    registerMap.clear();
    
    //  Clear the D3D9 input semantic usages.
    inputUsage.clear();

    //  Clear the D3D9 output semantic usages.
    outputUsage.clear();

    //  Check the shader type.
    if(type == VERTEX_SHADER)
    {
        //  Restrict CG1 output register 0 for the position output semantic usage.
        outputUsage[D3DUsageId(D3DDECLUSAGE_POSITION, 0)] = GPURegisterId(0, arch::OUT);

        //  Restrict CG1 output registers 1 and 2 for the two color output semantic usages.
        for(U32 r = 0; r < 2; r++)
            outputUsage[D3DUsageId(D3DDECLUSAGE_COLOR, r)] = GPURegisterId(1 + r, arch::OUT);

        //  Restrict CG1 output registers 3 to 12 for texture coordinates.
        for(U32 r = 0; r < 10; r++)
            outputUsage[D3DUsageId(D3DDECLUSAGE_TEXCOORD, r)] = GPURegisterId(3 + r, arch::OUT);

        //  Restrict CG1 output register 14 for the fog output semantic usage.
        outputUsage[D3DUsageId(D3DDECLUSAGE_FOG, 0)] = GPURegisterId(14, arch::OUT);
        
        //  Restrict CG1 output register 15 for point size output semantic usage.
        outputUsage[D3DUsageId(D3DDECLUSAGE_PSIZE, 0)] = GPURegisterId(15, arch::OUT);

        //  Check the shader version.
        switch(version)
        {
            case 0x0101:
                // v#
                // Input registers will be mapped when a declaration is found.
                
                // c#
                // Constants will be declared when found
                
                // r#
                // Temps will be declared when found
                
                // a0 (address register)
                mapRegister(D3DRegisterId(0, D3DSPR_ADDR), GPURegisterId(0, arch::ADDR));
                
                // oPos (position output register)
                declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_POSITION, 0),  D3DRegisterId(D3DSRO_POSITION, D3DSPR_RASTOUT));
                
                // oFog (fog output register)
                declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_FOG, 0),  D3DRegisterId(D3DSRO_FOG, D3DSPR_RASTOUT));
                
                // oPts (point size output register)
                declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_PSIZE, 0),  D3DRegisterId(D3DSRO_POINT_SIZE, D3DSPR_RASTOUT));
                
                // oD# (two color output registers)
                for(U32 r = 0; r < 2; r++)
                    declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_COLOR, r), D3DRegisterId(r, D3DSPR_ATTROUT));
                
                // oT# (eight texture output registers)
                for(U32 r = 0; r < 8; r++)
                    declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_TEXCOORD, r), D3DRegisterId(r, D3DSPR_OUTPUT));
                
                break;

            case 0x0200:
            case 0x0201:
            case 0x02FF:

                // v#
                // Input registers will be mapped when a declaration is found.
                
                // c#
                // Constants will be declared when found
                
                // r#
                // Temps will be declared when found
                
                // a0 (address register)
                mapRegister(D3DRegisterId(0,D3DSPR_ADDR), GPURegisterId(0, arch::ADDR));
                
                // b#
                // Not supported
                
                // i#
                // Not supported
                
                // aL
                // Not supported
                
                // oPos (position output register)
                declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_POSITION, 0),  D3DRegisterId(D3DSRO_POSITION, D3DSPR_RASTOUT));
                
                // oFog (fog output register)
                declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_FOG, 0),  D3DRegisterId(D3DSRO_FOG, D3DSPR_RASTOUT));
                
                // oPts (point size output register)
                declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_PSIZE, 0),  D3DRegisterId(D3DSRO_POINT_SIZE, D3DSPR_RASTOUT));
                
                // oD# (two color output registers)
                for(U32 r = 0; r < 2; r++)
                    declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_COLOR, r), D3DRegisterId(r, D3DSPR_ATTROUT));
                
                // oT# (eight texture output registers)
                for(DWORD r = 0; r < 8; r++)
                    declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_TEXCOORD, r), D3DRegisterId(r, D3DSPR_OUTPUT));
                   
                break;

            case 0x0300:
                // v#
                // Input registers will be mapped when a declaration is found.
                
                // c#
                // Constants will be declared when found
                
                // r#
                // Temps will be declared when found
                
                // a0 (address register)
                mapRegister(D3DRegisterId(0, D3DSPR_ADDR), GPURegisterId(0, arch::ADDR));
                
                // i#                
                // Not supported
                
                // b#
                // Not supported
                
                // p0
                // Not supported
                
                // s#
                // Samplers will be mapped when a declaration is found.
                
                // o#
                // Output registers will be mapped when a declaration is found.
                
                break;

            default:
                {
                    char buff[1024];
                    sprintf(buff, "Vertex shader version %04x not supported\n", version);
                    CG_ASSERT(buff);
                }
                break;                
        }
    }
    else
    {
    
        // IR_PIXEL_SHADER

        //  Check the alpha test function.            
        if (alpha_func != D3DCMP_ALWAYS)
        {
            //  Reserve the two constant registers required for alpha test emulation.
            declareMapAndReserveConst(D3DRegisterId(ALPHA_TEST_CONSTANT_MINUS_ONE, D3DSPR_CONST));
            declareMapAndReserveConst(D3DRegisterId(ALPHA_TEST_CONSTANT_REF, D3DSPR_CONST));
            
            alpha_test_declaration.alpha_const_minus_one = ALPHA_TEST_CONSTANT_MINUS_ONE;
            alpha_test_declaration.alpha_const_ref = ALPHA_TEST_CONSTANT_REF;
        }       

        //  Restrict CG1 input register 0 for position input semantic usage.
        inputUsage[D3DUsageId(D3DDECLUSAGE_POSITION, 0)] = GPURegisterId(0, arch::IN);

        //  Restrict CG1 input registers 1 and 2 for color input semantic usage.
        for(U32 r = 0; r < 2; r++) 
            inputUsage[D3DUsageId(D3DDECLUSAGE_COLOR, r)] = GPURegisterId(1 + r, arch::IN);
        
        //  Reserve CG1 input registers 3 to 12 for texture coordinate input semantic usage.
        for(U32 r = 0; r < 10; r ++)
            inputUsage[D3DUsageId(D3DDECLUSAGE_TEXCOORD, r)] = GPURegisterId(3 + r, arch::IN);

        //  Reserve CG1 input register 15 for face input semantic usage.
        inputUsage[D3DUsageId(D3DDECLUSAGE_POSITION, 1)] = GPURegisterId(15, arch::IN);
        
        //  Reserve CG1 output registers 1 to 4 for color output.
        for(U32 r = 0; r < 4; r++)
            outputUsage[D3DUsageId(D3DDECLUSAGE_COLOR, r)] = GPURegisterId(1 + r, arch::OUT);
        
        //  Reserver CG1 output register 0 for depth output.
        outputUsage[D3DUsageId(D3DDECLUSAGE_POSITION, 0)] = GPURegisterId(0, arch::OUT);

        GPURegisterId input;
        GPURegisterId textureTemp;
        switch(version)
        {
            case 0x0101:
            case 0x0102:
            case 0x0103:
                // c#
                // Constants will be declared when found
                
                // r#
                // Temps will be declared when found
                
                // oC0 (color output register)
                // If alpha test emulation is required map color output register to a temporary register.
                if (alpha_func == D3DCMP_ALWAYS)
                    declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_COLOR, 0), D3DRegisterId(0, D3DSPR_COLOROUT));
                else
                    reserveAndMapTemp(D3DRegisterId(0, D3DSPR_COLOROUT));

                // t# (4 texture input registers)
                for(U32 r = 0; r < 4; r++)
                {
                    //  Declare and map texture input registers to CG1 registers.
                    declareMapAndReserveInput(D3DUsageId(D3DDECLUSAGE_TEXCOORD, r), D3DRegisterId(r, D3DSPR_TEXTURE));
                    
                    //  For pixel shader models 1.x the texture coordinate input registers are aliased to temporary registers.
                    
                    //  Unmap texture coordinate register from the CG1 input register and map to a temporary register.
                    input = unmapRegister(D3DRegisterId(r, D3DSPR_TEXTURE));
                    textureTemp = reserveAndMapTemp(D3DRegisterId(r, D3DSPR_TEXTURE));
                    
                    //  Copy from the input CG1 register to the temporary register.
                    generateMov(textureTemp, input);
                }
                
                // v# (color input registers)
                for(U32 r = 0; r < 2; r++)
                    declareMapAndReserveInput(D3DUsageId(D3DDECLUSAGE_COLOR, r), D3DRegisterId(r, D3DSPR_INPUT));
                
                break;

            case 0x0104:
                // c#
                // Constants will be declared when found

                // r#
                // Temps will be declared when found

                // oC0 (color output register)
                // If alpha test emulation is required map color output semantic usage to a temporary register.
                if (alpha_func == D3DCMP_ALWAYS)
                    declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_COLOR, 0), D3DRegisterId(0, D3DSPR_COLOROUT));
                else
                    reserveAndMapTemp(D3DRegisterId(0, D3DSPR_COLOROUT));
                 
                // t# (texture input registers)
                for(U32 r = 0; r < 6; r ++)
                    declareMapAndReserveInput(D3DUsageId(D3DDECLUSAGE_TEXCOORD, r), D3DRegisterId(r, D3DSPR_TEXTURE));
                
                // v# (color input registers)
                for(U32 r = 0; r < 2; r++)
                    declareMapAndReserveInput(D3DUsageId(D3DDECLUSAGE_COLOR, r), D3DRegisterId(r, D3DSPR_INPUT));
                
                break;

            case 0x0200:
            case 0x0201:
            case 0x02FF:
                // v#
                // Input registers are mapped when a declaration is found

                // c#
                // Constants will be declared when found

                // r#
                // Temps will be declared when found

                // i#
                // Not supported

                // b#
                // Not supported

                // p0

                // Not supported

                // s#
                // Samplers are mapped when a declaration is found

                // t# (texture input registers)
                // Will be mapped when a declaration is found.
                
                // oC# (four color output registers)
                // oC0 (color output register)
                // If alpha test emulation is required map color output semantic usage to a temporary register.
                if(alpha_func == D3DCMP_ALWAYS)
                    declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_COLOR, 0), D3DRegisterId(0, D3DSPR_COLOROUT));
                else
                    reserveAndMapTemp(D3DRegisterId(0,D3DSPR_COLOROUT));

                for(U32 r = 1; r < 4; r++)
                    declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_COLOR, r), D3DRegisterId(r, D3DSPR_COLOROUT));
                    
                // oDepth0 (depth output register)
                declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_POSITION, 0), D3DRegisterId(0, D3DSPR_DEPTHOUT));
                
                break;
                
            case 0x0300:

                // v#
                // Input registers are mapped when a declaration is found               
                
                // c#
                // Constants will be declared when found
                
                // r#
                // Temps will be declared when found
                
                // i#
                // Not supported
                
                // b#
                // Not supported
                
                // p0
                // Not supported
                
                // s#
                // Samplers are mapped when a declaration is found
                
                // vFace
                // Will be mapped when a declaration is found.
                
                // vPos
                // Will be mapped when a declaration is found.
                
                // aL
                // Not supported
                
                // oC# (four color output registers)
                
                // oC0 (color output registers)
                // If alpha test emulation is required map color output semantic usage to a temporary register.
                if(alpha_func == D3DCMP_ALWAYS)
                    declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_COLOR, 0), D3DRegisterId(0, D3DSPR_COLOROUT));
                else
                    reserveAndMapTemp(D3DRegisterId(0, D3DSPR_COLOROUT));
                
                for(U32 r = 1; r < 4; r++)
                    declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_COLOR, r), D3DRegisterId(r, D3DSPR_COLOROUT));

                // oDepth0 (depth output register)
                declareMapAndReserveOutput(D3DUsageId(D3DDECLUSAGE_POSITION, 0), D3DRegisterId(0, D3DSPR_DEPTHOUT));
                
                break;
                
            default:
                {
                    char buff[1024];
                    sprintf(buff, "Pixel shader version %04x not supported\n", version);
                    CG_ASSERT(buff);
                }
                break;                
        }
    }
}

//  Initialize the mapping of D3D9 opcodes to CG1 opcodes.
void IRTranslator::initializeOpcodes()
{
    opcodeMap.clear();
    
    //  Create the mapping table based on the shader type and version.
    if(type == VERTEX_SHADER)
    {
        opcodeMap[D3DSIO_ADD]  = ADD;
        opcodeMap[D3DSIO_DP3]  = DP3;
        opcodeMap[D3DSIO_DP4]  = DP4;
        opcodeMap[D3DSIO_DST]  = DST;
        opcodeMap[D3DSIO_EXP]  = EX2;
        if (version == 0x0101)
            opcodeMap[D3DSIO_EXPP] = EXP;
        else
            opcodeMap[D3DSIO_EXPP] = EX2;
        opcodeMap[D3DSIO_FRC]  = FRC;
        opcodeMap[D3DSIO_LIT]  = LIT;
        opcodeMap[D3DSIO_LOG]  = LG2;
        opcodeMap[D3DSIO_LOGP] = LOG;
        opcodeMap[D3DSIO_MAD]  = MAD;
        opcodeMap[D3DSIO_MAX]  = MAX;
        opcodeMap[D3DSIO_MIN]  = MIN;
        opcodeMap[D3DSIO_MOV]  = MOV;
        opcodeMap[D3DSIO_MOVA] = ARL;
        opcodeMap[D3DSIO_MUL]  = MUL;
        opcodeMap[D3DSIO_NOP]  = NOP;
        opcodeMap[D3DSIO_RCP]  = RCP;
        opcodeMap[D3DSIO_RSQ]  = RSQ;
        opcodeMap[D3DSIO_SGE]  = SGE;
        opcodeMap[D3DSIO_SLT]  = SLT;
    }
    else
    {
        //IR_PIXEL_SHADER
        opcodeMap[D3DSIO_ADD]     = ADD;
        opcodeMap[D3DSIO_DP3]     = DP3;
        opcodeMap[D3DSIO_DP4]     = DP4;
        opcodeMap[D3DSIO_EXP]     = EX2;
        opcodeMap[D3DSIO_EXPP]    = EXP;
        opcodeMap[D3DSIO_FRC]     = FRC;
        opcodeMap[D3DSIO_LOG]     = LG2;
        opcodeMap[D3DSIO_MAD]     = MAD;
        opcodeMap[D3DSIO_MAX]     = MAX;
        opcodeMap[D3DSIO_MIN]     = MIN;
        opcodeMap[D3DSIO_MOV]     = MOV;
        opcodeMap[D3DSIO_MUL]     = MUL;
        opcodeMap[D3DSIO_NOP]     = NOP;
        opcodeMap[D3DSIO_RCP]     = RCP;
        opcodeMap[D3DSIO_RSQ]     = RSQ;
        opcodeMap[D3DSIO_TEXKILL] = KIL;
        if(version >= 0x0200)
        {
            opcodeMap[D3DSIO_TEX]    = TEX;
            opcodeMap[D3DSIO_TEXLDL] = TXL;
        }
	}
}

//  Visitor for version node.
void IRTranslator::visit(VersionIRNode *n)
{
    //  Get the shader program type and version.
    type = n->getType();
    version = n->getVersion();

    //  Initialize the translator state based on the shader type and version.
    initializeRegisters();
    initializeOpcodes();
}

//  Visitor for declaration node.
void IRTranslator::visit(DeclarationIRNode *n)
{
    // TODO check if there's exceptions, this castings get me nervous
    DestinationParameterIRNode *destination = static_cast<DestinationParameterIRNode*>(n->getChilds()[1]);
    D3DRegisterId d3dreg(destination->getNRegister(), destination->getRegisterType());

    SemanticIRNode *semantic = 0;
    SamplerInfoIRNode *info = 0;
    D3DUsageId usage;
    
    //  Check the register type.
    switch (destination->getRegisterType())
    {
        case D3DSPR_INPUT:
        
            //  Input register.  Get semantic usage and declare/map/reserve as input register.
            semantic = static_cast<SemanticIRNode*>(n->getChilds()[0]);
            usage.usage = semantic->getUsage();
            usage.index = semantic->getUsageIndex();
            declareMapAndReserveInput(usage, d3dreg);
            break;

        case D3DSPR_MISCTYPE:
        
            //  Special input registers in PS3.0.  Get semantic usage and declare/map/reserve as input register.
            switch(destination->getNRegister())
            {
                case D3DSMO_POSITION:
                    semantic = static_cast<SemanticIRNode*>(n->getChilds()[0]);
                    usage.usage = semantic->getUsage();
                    usage.index = semantic->getUsageIndex();
                    declareMapAndReserveInput(usage, d3dreg);
                    break;
                case D3DSMO_FACE:
                    semantic = static_cast<SemanticIRNode*>(n->getChilds()[0]);
                    usage.usage = semantic->getUsage();
                    usage.index = semantic->getUsageIndex();
                    declareMapAndReserveInput(usage, d3dreg);
                    break;
            }
            break;
            
        case D3DSPR_TEXTURE:
        
            //  Texture input registers (PS2.0).  Declare/map/reserve as input registers.
            semantic = static_cast<SemanticIRNode*>(n->getChilds()[0]);
            usage.usage = semantic->getUsage();
            usage.index = semantic->getUsageIndex();
            declareMapAndReserveInput(usage, d3dreg);
            break;

        case D3DSPR_SAMPLER:

            //  Sampler register.  Declare, reserve and map.        
            info = static_cast<SamplerInfoIRNode*>(n->getChilds()[0]);
            declareMapAndReserveSampler(info->getTextureType(), d3dreg);
            break;

        case D3DSPR_TEXCRDOUT:

            //  Output registers (VS3.0).  Declare/map/reserve as output registers.
            semantic = static_cast<SemanticIRNode*>(n->getChilds()[0]);
            usage.usage = semantic->getUsage();
            usage.index = semantic->getUsageIndex();
            declareMapAndReserveOutput(usage, d3dreg);
            break;
        
        default:
            {
                char buffer[128];
                sprintf(buffer, "Unexpected destination type %d for declaration at offset %x.", destination->getRegisterType(), n->getOffset());
                CG_ASSERT(buffer);
            }
            break;
    }
}

//  Process an instruction source operand (source parameter nodes associated with an instruction node).
void IRTranslator::visitInstructionSource(SourceParameterIRNode *n)
{
    //  Get the D3D9 register used as instruction source.
    D3DRegisterId d3dreg(n->getNRegister(), n->getRegisterType());
    
    //  Check if the D3D9 register is already mapped to an CG1 register.
    if (!isMapped(d3dreg))
    {
        //  Map the D3D9 register to an CG1 register.
        mapAndReserveRegister(d3dreg);
    }

    Operand op;    

    //  Get the CG1 register to which the D3D9 register is mapped to.
    op.registerId = mappedRegister(d3dreg);    
    
    //  Translate the operand swizzling mode.
    op.swizzle = nativeSwizzleMode(n->getSwizzle(0) | n->getSwizzle(1) | n->getSwizzle(2) | n->getSwizzle(3));
    
    //  Set the negate operand modifier.
    op.negate = (n->getModifier() == D3DSPSM_NEG);
    
    //  Check if the source parameter uses relative addressing.
    if((n->getAddressMode() == D3DSHADER_ADDRMODE_RELATIVE))
    {
        //  Set the relative adressing parameters.
        op.addressing = true;
        relative.enabled = true;
        
        //  Check the shader version.
        if(version < 0x0200)
        {
            // Relative addressing uses a0 implicitely for version < 2.0
            relative.component = 0;
            relative.registerN = 0;
        }
        else
        {
            // There's a relative addressing token for versions >= 2.0
            RelativeAddressingIRNode *n_rel = n->getRelativeAddressing();
            relative.registerN = n_rel->getNRegister();
            relative.component = n_rel->getComponentIndex();
        }
        
        // NOTE: In simulator: cN[a0.x + offset]
        //       In Direct3D:  cN[a0.x]
        //       So offset is 0
        relative.offset = 0;
    }

    //  Add the instruction operand to the vector of operands for the current instruction
    //  being translated.
    operands.push_back(op);
}

//  Process instruction results (destination parameter node associated with an instruction node).
void IRTranslator::visitInstructionDestination(DestinationParameterIRNode *n)
{
    //  Get the D3D9 register used as the instruction result.
    D3DRegisterId d3dreg(n->getNRegister(), n->getRegisterType());
    
    //  Check if the D3D9 register is already mapped to an CG1 register.
    if (!isMapped(d3dreg))
    {
        //  Map the D3D9 register to an CG1 register.
        mapAndReserveRegister(d3dreg);
    }
    
    //  Set the instruction result information.
    result.registerId = mappedRegister(d3dreg);
    result.maskMode = nativeMaskMode(n->getWriteMask());
    result.saturate = ((n->getModifier() & D3DSPDM_SATURATE) != 0);
}

//  Gets the CG1 register to which a D3D9 register is mapped to.
GPURegisterId IRTranslator::mappedRegister(D3DRegisterId d3dreg)
{
    //  Search the D3D9 register in the mapping table.
    map<D3DRegisterId, GPURegisterId>::iterator itR;
    itR = registerMap.find(d3dreg);
    
    //  Check if the D3D9 register is mapped to an CG1 register.
    if(itR != registerMap.end())
        return (*itR).second;
    else
    {
        D3D_DEBUG( cout << "IRTranslator: WARNING: Unmapped d3d register" << endl; )
        ///@note The intention is to avoid halting simulation
        error = true;
        return GPURegisterId(0, arch::INVALID);
    }
}

//  Translate the M4X4 shader instruction to CG1 shader instructions.
void IRTranslator::emulateM4X4(InstructionIRNode *n)
{
    // M4x4 references implicitly the three constants following source1,
    // so is necessary to declare them
    SourceParameterIRNode *source = n->getSources()[1];
    D3DRegisterId d3dconst(source->getNRegister(), source->getRegisterType());
    for(U32 i = 0; i < 3; i ++)
    {
        d3dconst.num++;
        if (!isMapped(d3dconst))
            mapAndReserveRegister(d3dconst);
    }


    ShaderInstructionBuilder builder;
    builder.setOpcode(DP4);
    builder.setOperand(0, operands[0]);
    Operand auxOp = operands[1];
    Result auxRes = result;

    auxRes.maskMode = nativeMaskMode(D3DSP_WRITEMASK_0);
    builder.setResult(auxRes);
    builder.setOperand(1, auxOp);
    instructions.push_back(builder.buildInstruction());

    auxRes.maskMode = nativeMaskMode(D3DSP_WRITEMASK_1);
    builder.setResult(auxRes);
    auxOp.registerId.num ++;
    builder.setOperand(1, auxOp);
    instructions.push_back(builder.buildInstruction());

    auxRes.maskMode = nativeMaskMode(D3DSP_WRITEMASK_2);
    builder.setResult(auxRes);
    auxOp.registerId.num ++;
    builder.setOperand(1, auxOp);
    instructions.push_back(builder.buildInstruction());

    auxRes.maskMode = nativeMaskMode(D3DSP_WRITEMASK_3);
    builder.setResult(auxRes);
    auxOp.registerId.num ++;
    builder.setOperand(1, auxOp);
    instructions.push_back(builder.buildInstruction());

}

//  Translate the SUB instruction to CG1 shader instructions.
void IRTranslator::emulateSUB()
{
    //  SUB dst, src0, src1
    //  -------------------------
    //  ADD dst, src0, -src1
    
    ShaderInstructionBuilder builder;
    builder.setOpcode(ADD);
    builder.setOperand(0, operands[0]);
    Operand auxOp = operands[1];
    auxOp.negate = !auxOp.negate;
    builder.setOperand(1, auxOp);
    builder.setResult(result);
    builder.setRelativeMode(relative);
    instructions.push_back(builder.buildInstruction());
}

//  Translate the TEXLD1314 instruction to CG1 shader instructions.
void IRTranslator::emulateTEXLD1314(InstructionIRNode *n)
{
    DestinationParameterIRNode *destN;
    D3DRegisterId resultD3DRegister;
    D3DRegisterId sourceD3DRegister;
    ShaderInstructionBuilder builder;
    GPURegisterId resultRegister;
    Operand operand;
    
    //  Check the shader version.
    switch(version)
    {
        case 0x0101:
        case 0x0102:
        case 0x0103:
        
            destN = n->getDestination();
            resultD3DRegister = D3DRegisterId(destN->getNRegister(), destN->getRegisterType());
            builder.setOpcode(TEX);
            builder.setResult(result);
            resultRegister = result.registerId;
            operand.registerId = result.registerId;
            builder.setOperand(0, operand);
            operand.registerId = GPURegisterId(resultD3DRegister.num, arch::TEXT);
            builder.setOperand(1, operand);
            instructions.push_back(builder.buildInstruction());
            break;
            
        case 0x0104:
        
            destN = n->getDestination();
            resultD3DRegister = D3DRegisterId(destN->getNRegister(), destN->getRegisterType());
            builder.setResult(result);
            builder.setOperand(0, operands[0]);
            operand.registerId = GPURegisterId(resultD3DRegister.num, arch::TEXT);
            builder.setOperand(1, operand);
            instructions.push_back(builder.buildInstruction());
            break;
            
        default:
            {
                char buff[1024];
                sprintf(buff, "Pixel shader version %04x not supported\n", version);
                CG_ASSERT(buff);
            }
            break;                
    }
}

//  Translate the LRP instruction to CG1 shader instructions.
void IRTranslator::emulateLRP()
{

    //  LRP dst, src0, src1, src2
    //  -------------------------
    //  ADD t0, src1, -src2
    //  MAD dst, src0, t0, src2
        
    ShaderInstructionBuilder builder;
    GPURegisterId t0 = reserveTemp();

    // ADD t0, src1, -src2
    builder.resetParameters();
    builder.setOpcode(ADD);
    builder.setOperand(0, operands[1]);
    Operand op2neg = operands[2];
    op2neg.negate = !op2neg.negate;
    builder.setOperand(1, op2neg);
    Result res_t0;
    res_t0.registerId = t0;
    builder.setResult(res_t0);
    builder.setPredication(currentPredication);
    instructions.push_back(builder.buildInstruction());

    //    MAD dst, src0, r0, src2
    builder.resetParameters();
    builder.setOpcode(MAD);
    builder.setOperand(0, operands[0]);
    Operand op_t0;
    op_t0.registerId = t0;
    builder.setOperand(1, op_t0);
    builder.setOperand(2, operands[2]);
    builder.setResult(result);
    builder.setPredication(currentPredication);
    instructions.push_back(builder.buildInstruction());

    releaseTemp(t0);
}

//  Translate the POW instruction to CG1 shader instructions.
void IRTranslator::emulatePOW()
{

    // POW(ABS(src0),src1) == EXP(src1 * LOG(ABS(src0)))
    // ---------------------------------------------------------------
    // POW dst, src0, src1
    // -------------------
    // LG2 t0, |src0|
    // MUL t0, t0, src1
    // EX2 dst, t0

    ShaderInstructionBuilder builder;
    GPURegisterId t0 = reserveTemp();

    //  Build => LG2 t0, |src0|
    builder.resetParameters();
    builder.setOpcode(arch::LG2);
    Operand opTemp = operands[0];
    opTemp.absolute = true;
    builder.setOperand(0, opTemp);
    Result resTemp;
    resTemp.registerId = t0;
    resTemp.maskMode = arch::XNNN;   
    builder.setResult(resTemp);
    builder.setPredication(currentPredication);
    instructions.push_back(builder.buildInstruction());

    //  Build => MUL t0, t0, src1
    builder.resetParameters();
    builder.setOpcode(arch::MUL);
    opTemp.registerId = t0;
    opTemp.absolute = opTemp.negate = false;
    opTemp.swizzle = arch::XXXX;    
    builder.setOperand(0, opTemp);
    builder.setOperand(1, operands[1]);
    builder.setResult(resTemp);
    builder.setPredication(currentPredication);
    instructions.push_back(builder.buildInstruction());
    
    //  Build => EX2 dst, dst
    builder.resetParameters();
    builder.setOpcode(arch::EX2);
    builder.setOperand(0, opTemp);
    builder.setResult(result);
    builder.setPredication(currentPredication);
    instructions.push_back(builder.buildInstruction());    

    releaseTemp(t0);
}

//  Translate the NRM instruction to CG1 shader instructions.
void IRTranslator::emulateNRM()
{
    // NRM dest, src0
    // ----------------------------
    // DP3 t0, src0, src0
    // RSQ t0, t0
    // MUL dest, src0, t0
    
    ShaderInstructionBuilder builder;
    GPURegisterId t0 = reserveTemp();

    //  Build => DP3 t0, src0, src0
    builder.resetParameters();
    builder.setOpcode(arch::DP3);
    builder.setOperand(0, operands[0]);
    builder.setOperand(1, operands[0]);
    Result t0_res;
    t0_res.registerId = t0;
    t0_res.maskMode = arch::XNNN;
    builder.setResult(t0_res);
    builder.setPredication(currentPredication);
    instructions.push_back(builder.buildInstruction());
   
    //  Build => RSQ t0, t0
    builder.resetParameters();
    builder.setOpcode(arch::RSQ);
    Operand t0_tempOp;
    t0_tempOp.registerId = t0;
    t0_tempOp.swizzle = arch::XXXX;
    builder.setOperand(0, t0_tempOp);
    builder.setResult(t0_res);
    builder.setPredication(currentPredication);
    instructions.push_back(builder.buildInstruction());
    
    //  Build => MUL dest, src0, t0
    builder.resetParameters();
    builder.setOpcode(arch::MUL);
    builder.setOperand(0, operands[0]);
    builder.setOperand(1, t0_tempOp);
    builder.setResult(result);
    builder.setPredication(currentPredication);
    instructions.push_back(builder.buildInstruction());

    releaseTemp(t0);
}

//  Translate the CMP instruction to CG1 shader instructions.
void IRTranslator::emulateCMP()
{
    //  Using the CMP instruction already implemented in the simulator:
    //
    //  CMP dst, src0, src1, src2
    //
    //     dst = (src < 0) ? src1 : src2
    // 
    //  D3D9 CMP dst, src0, src1, src2
    //
    //     dst = (src >= 0) ? src1 : src2
    //
    //  So the implementation has only to swap src1 with src2.
    //

    ShaderInstructionBuilder builder;

    builder.resetParameters();
    builder.setOpcode(CMP);
    builder.setOperand(0, operands[0]);
    builder.setOperand(1, operands[2]);
    builder.setOperand(2, operands[1]);
    builder.setResult(result);
    builder.setPredication(currentPredication);
    instructions.push_back(builder.buildInstruction());   
}

//  Translate the DP2ADD instruction to CG1 shader instructions.
void IRTranslator::emulateDP2ADD()
{

    // DP2ADD dest, src0, src1, src2
    // ----------------------------
    //
    //  result is scalar (broadcast)
    //  src2 is scalar (broadcast)
    //
    //   dest = src0.x * src1.x + src0.y * src1.y + src2.x
    //
    //   MAD t0, src0.x, src1.x, src2
    //   MAD dest, src0.y, src1.y, t0
    //
    
    ShaderInstructionBuilder builder;
    GPURegisterId t0 = reserveTemp();

    Operand mad_src0;
    Operand mad_src1;
    Operand mad_src2;
    Result mad_res;
    
    //   MAD t0, src0.x, src1.x, src2
    builder.resetParameters();
    builder.setOpcode(MAD);        
    mad_src0 = operands[0];
    selectSwizzledComponent(0, operands[0], mad_src0);
    mad_src1 = operands[1];
    selectSwizzledComponent(0, operands[1], mad_src1);        
    builder.setOperand(0, mad_src0);
    builder.setOperand(1, mad_src1);
    builder.setOperand(2, operands[2]);    
    mad_res.registerId = t0;
    mad_res.maskMode = arch::XNNN;
    builder.setResult(mad_res);
    builder.setPredication(currentPredication);
    instructions.push_back(builder.buildInstruction());

    //   MAD dest, src0.y, src1.y, t0
    builder.resetParameters();
    builder.setOpcode(MAD);    
    selectSwizzledComponent(1, operands[0], mad_src0);
    selectSwizzledComponent(1, operands[1], mad_src1);
    mad_src2.registerId = t0;
    mad_src2.swizzle = arch::XXXX;
    builder.setOperand(0, mad_src0);
    builder.setOperand(1, mad_src1);
    builder.setOperand(2, mad_src2);
    builder.setResult(result);
    builder.setPredication(currentPredication);
    instructions.push_back(builder.buildInstruction());

    releaseTemp(t0);
}    

//  Translate the ABS shader instruction to CG1 shader instructions.
void IRTranslator::emulateABS()
{
    // ABS dest, src
    // ----------------------------
    // MOV dest, |src|


    ShaderInstructionBuilder builder;
    Operand absOp0 = operands[0];
    absOp0.absolute = true;
    builder.resetParameters();
    builder.setOpcode(MOV);
    builder.setOperand(0, absOp0);        
    builder.setResult(result);
    builder.setPredication(currentPredication);
    instructions.push_back(builder.buildInstruction());
}

//  Translate the SINCOS shader instruction to CG1 shader instructions.
void IRTranslator::emulateSINCOS()
{

    // SINCOS dest, src
    // ----------------------------
    //
    //  if dest.x
    //
    //      COS dest.x, src
    //
    //  if dest.y
    //
    //      SIN dest.y, src
    //
    //  if dest.xy
    //
    //      COS dest.x, src
    //      SIN dest.y, src
    //
    
    ShaderInstructionBuilder builder;

    Result cos_res;
    Result sin_res;

    //  Check if the cosinus has to be computed.    
    if ((result.maskMode == arch::XNNN) || (result.maskMode == arch::XYNN))
    {
        //   COS dest.x, src
        builder.resetParameters();
        builder.setOpcode(COS);
        builder.setOperand(0, operands[0]);
        cos_res = result;
        cos_res.maskMode = arch::XNNN;
        builder.setResult(cos_res);
        builder.setPredication(currentPredication);
        instructions.push_back(builder.buildInstruction());
    }

    //  Check if the sinus has to be computed.    
    if ((result.maskMode == arch::NYNN) || (result.maskMode == arch::XYNN))
    {
        //   SIN dest.y, src
        builder.resetParameters();
        builder.setOpcode(SIN);
        builder.setOperand(0, operands[0]);
        sin_res = result;
        sin_res.maskMode = arch::NYNN;
        builder.setResult(sin_res);
        builder.setPredication(currentPredication);
        instructions.push_back(builder.buildInstruction());
    }
}    

//  Generates the instructions required to compute the predicate for the condition in an if instruction.
void IRTranslator::computePredicate(D3DSHADER_COMPARISON compareOp)
{
    //  Compute the predicate register based on an IF instruction condition.
    ShaderInstructionBuilder builder;
    
    //  Select a predicate register.
    Result predResult;
    GPURegisterId predReg = reservePredicate();
    
    //  Set the predicate register as result register.
    predResult.registerId = predReg;

    //  Set the current instruction predication state.
    currentPredication.predicated = true;
    currentPredication.predicateRegister = predReg.num;

    //  Set the set predicate instruction operands and result registers.    
    builder.resetParameters();
    builder.setOperand(0, operands[0]);
    builder.setOperand(1, operands[1]);    
    builder.setResult(predResult);
    
    //  Check the if comparison function.
    switch(compareOp)
    {
        case D3DSPC_GT:
            currentPredication.negatePredicate = false;
            builder.setOpcode(SETPGT);
            break;
            
        case D3DSPC_EQ:
            currentPredication.negatePredicate = false;
            builder.setOpcode(SETPEQ);
            break;
            
        case D3DSPC_LT:
            currentPredication.negatePredicate = false;
            builder.setOpcode(SETPLT);
            break;

        case D3DSPC_GE:        
            currentPredication.negatePredicate = true;
            builder.setOpcode(SETPLT);
            break;
        case D3DSPC_NE:
            currentPredication.negatePredicate = true;
            builder.setOpcode(SETPEQ);
            break;

        case D3DSPC_LE:
            currentPredication.negatePredicate = true;
            builder.setOpcode(SETPGT);
            break;
        
        case D3DSPC_RESERVED0:
        case D3DSPC_RESERVED1:
            CG_ASSERT("Reserved comparison mode.");
            break;
        default:
            CG_ASSERT("Undefined comparison mode.");
            break;
    }    

    instructions.push_back(builder.buildInstruction());        
}


//  Generates an CG1 NOP instruction
void IRTranslator::generateNOP()
{
    ShaderInstructionBuilder builder;
    builder.resetParameters();
    builder.setOpcode(NOP);
    instructions.push_back(builder.buildInstruction());
}

//  Selects a written component from an instruction result register.
void IRTranslator::selectComponentFromResult(Result result, Operand &opTemp)
{
    //  Use one of the components of the result register
    //  to hold the temporal values.
    opTemp.registerId = result.registerId;
    
    //  Clear modifier and flags (copied from src0).
    opTemp.absolute = false;
    opTemp.addressing = false;    
    opTemp.negate = false;
    
    //  Select one of the components written by the
    //  result to hold the temporal value.
    switch(result.maskMode)
    {
        case arch::NNNW:
            opTemp.swizzle = arch::WWWW;
            break;
        case arch::NNZN:
        case arch::NNZW:
            opTemp.swizzle = arch::ZZZZ;
            break;
        case arch::NYNN:
        case arch::NYNW:
        case arch::NYZN:
        case arch::NYZW:
            opTemp.swizzle = arch::YYYY;
            break;        
        case arch::XNNN:
        case arch::XNNW:
        case arch::XNZN:
        case arch::XNZW:
        case arch::XYNN:
        case arch::XYNW:
        case arch::XYZN:
        case arch::mXYZW:
            opTemp.swizzle = arch::XXXX;
            break;
        default:
            CG_ASSERT("Undefined write mask mode.");
            break;
    }
}

//  Selects the specified swizzle component from an instruction operand setting a broadcast from that component in output operand.
void IRTranslator::selectSwizzledComponent(U32 component, Operand in, Operand &out)
{
    //  Get the selected swizzled component.
    U32 swizzledComponent = ((in.swizzle >> ((3 - component) * 2)) & 0x03);
    
    //  Set the swizzled component in the output operand.
    switch(swizzledComponent)
    {
        case 0:
            out.swizzle = arch::XXXX;
            break;
        case 1:
            out.swizzle = arch::YYYY;
            break;
        case 2:
            out.swizzle = arch::ZZZZ;
            break;
        case 3:
            out.swizzle = arch::WWWW;
            break;
        default:
            CG_ASSERT("Undefined component identifier.");
            break;
    }    
}

//  Visitor for instruciton nodes.
void IRTranslator::visit(InstructionIRNode *n)
{
    // Clear error
    error = false;

    // Clear the instruction operand and result information.
    operands.clear();   
    result = Result();    
    relative = RelativeMode();
    
    // Check if the instruction has result register.
    if(n->getDestination() != 0)
    {
        //  Process the instruction destination parameter token to obtain the result information.
        visitInstructionDestination(n->getDestination());
    }   

    //  Process the instruction source parameter tokens to obtain the operands information.
    for(U32 o = 0; o < n->getSources().size(); o++)
        visitInstructionSource(n->getSources()[o]);

    //  Check for errors when gathering information about the instruction operands and result.
    if(error)
    {
        //  Generate a NOP when the instruction can not be translated.
        generateNOP();
        
        //  Update the counter of untranslated instructions.
        untranslatedInstructions++;
        
        return;
    }

    // Try to find a direct translation
    map<D3DSHADER_INSTRUCTION_OPCODE_TYPE, ShOpcode>::const_iterator it_opc;
    it_opc = opcodeMap.find(n->getOpcode());
    if(it_opc != opcodeMap.end())
    {
        ShaderInstructionBuilder builder;

        // texkill seems doesn't admit swizzling and the swizzle calculated from the bytecode seems incorrect.
        // this will be a temporal solution.
        if (n->getOpcode() == D3DSIO_TEXKILL)
        {
            operands[0].swizzle = XYZW;
            builder.setOperand(0, operands[0]);
        }
        else
        {
            //  Set the instruction operands in the CG1 instruction builder.
            for(U32 i = 0; i < operands.size(); i ++)
                builder.setOperand(i, operands[i]);
        }
        
        //  Set the relative addressing mode in the CG1 instruction builder.
        builder.setRelativeMode(relative);
        
        //  Set the instruction result in the CG1 instruction builder.
        builder.setResult(result);
        
        ShOpcode opcode;
        
        //  texldp and texldb are encoded using the specific control field.        
        if (n->getOpcode() == D3DSIO_TEX)
        {
            if (n->getSpecificControls() == D3DSI_TEXLD_PROJECT)
                opcode = arch::TXP;
            else if (n->getSpecificControls() == D3DSI_TEXLD_BIAS)
                opcode = arch::TXB;
            else
                opcode = arch::TEX;
        }
        else
            opcode = (*it_opc).second;

        // NOTE: In D3D, address register can be written by a MOV instruction.
        //       CG1 handles address register writes with ARL instruction.
        //       So when a D3D MOV instruction is found, it's translated to either
        //       CG1 ARL or CG1 MOV depending on if instruction destination
        //       register is the address register or not
        if((type == VERTEX_SHADER) && (result.registerId.bank == ADDR) && (opcode == MOV))
            opcode = ARL;

        //  Set instruction predication in the CG1 instruction builder.
        builder.setPredication(currentPredication);
        
        //  Set the opcode in the CG1 instruction builder.
        builder.setOpcode(opcode);
        
        //  Build and add the CG1 instruction to the translated instruction list.
        instructions.push_back(builder.buildInstruction());
    }
    else
    {
        // Special instructions
        switch(n->getOpcode())
        {
            //  Control flow instructions.
            case D3DSIO_IFC:
            
                //  Check if already inside an IF block.
                if (insideIFBlock)
                {
                    error = true;
                }
                else
                {
                    insideIFBlock = true;
                    computePredicate(n->getComparison());
                }
                
                break;
            
            case D3DSIO_ELSE:
            
                //  Check if inside an IF block.
                if (insideIFBlock)
                {
                    insideIFBlock = false;
                    insideELSEBlock = true;
                    
                    //  Change the negation predicate for the other side of the condition.
                    currentPredication.negatePredicate = !currentPredication.negatePredicate;
                }
                else
                {
                    //  Unexpected ELSE.
                    error = true;
                }
                
                break;
                
            case D3DSIO_ENDIF:
            
                //  Check if inside an IF block
                if (insideIFBlock || insideELSEBlock)
                {
                    insideIFBlock = false;
                    insideELSEBlock = false;
                    currentPredication.predicated = false;
                    
                    //  Release the current predication register.
                    GPURegisterId predReg(currentPredication.predicateRegister, arch::PRED);
                    releasePredicate(predReg);
                }
                else
                {
                    //  Unexpected ENDIF.
                    error = true;
                }
                
                break;
                
            //  Emulated instructions.            
            case D3DSIO_M4x4:
                emulateM4X4(n);
                break;
            case D3DSIO_SUB:
                emulateSUB();
                break;
            case D3DSIO_TEX:
                emulateTEXLD1314(n);
                break;
            case D3DSIO_LRP:
                emulateLRP();
                break;
            case D3DSIO_POW:
                emulatePOW();
                break;
            case D3DSIO_NRM:
                emulateNRM();
                break;
            case D3DSIO_CMP:
                emulateCMP();
                break;
            case D3DSIO_DP2ADD:
                emulateDP2ADD();
                break;
            case D3DSIO_ABS:
                emulateABS();
                break;
            case D3DSIO_SINCOS:
                emulateSINCOS();
                break;
            
            default:
            
                ///@note The intention is to avoid halting simulation, so a NOP is generated.
                char message[40];
                sprintf(message, "Cannot translate Opcode %d", n->getOpcode());
                D3D_DEBUG( cout << "IRTranslator: WARNING: " << message << endl; )
                error = true;

                //  Check for control flow instruction not yet implemented.
                if (n->isJmp())
                    untranslatedControlFlow = true;

                break;
        }
    }

    //  Check for an error in the instruction translation.
    if (error)
    {
        //  Generate a NOP when the instruction can not be translated.
        generateNOP();
        
        //  Update the counter of untranslated instructions.
        untranslatedInstructions++;
        
        return;
    }
}

//  Set the alpha test function for emulation in the pixel shader.
void IRTranslator::setAlphaTest(D3DCMPFUNC _alpha_func)
{
    alpha_func = _alpha_func;
}

//  Set the fog enable for emulation in the pixel shader.
void IRTranslator::setFogEnabled(bool _fog_enabled)
{
    fog_enabled = _fog_enabled;
}

//  Returns if control flow instruction that could not be translated was found while translating the shader program.
bool IRTranslator::getUntranslatedControlFlow()
{
    return untranslatedControlFlow;
}

U32 IRTranslator::getUntranslatedInstructions()
{
    return untranslatedInstructions;
}

//  Implements fixed function emulation in the pixel shader (alpha test, fog ...)
void IRTranslator::generate_extra_code(GPURegisterId color_temp, GPURegisterId color_out)
{
    ShaderInstructionBuilder builder;
    Operand op0, op1;
    Result res;

    //  Check the selected alpha test function.
    switch(alpha_func)
    {
        case D3DCMP_NEVER:
            {
                //
                //  Kill all fragments.
                //
                //  c = {-1, -1, -1, -1}
                //
                //  KIL c
                //

                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();
                
                //  KIL c
                builder.setOpcode(KIL);
                op0.registerId = GPURegisterId(alpha_test_declaration.alpha_const_minus_one, arch::PARAM);
                builder.setOperand(0, op0);
                
                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());
            }
            break;

        case D3DCMP_LESS:
            {
                //
                //  SGE temp, color_temp.w, alpha_ref.w
                //  KIL -temp
                //
                
                //  Reserve a temporary register.
                GPURegisterId temp = reserveTemp();

                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();

                // SGE temp, color_temp.w, alpha_ref.w
                builder.setOpcode(SGE);
                op0.registerId = color_temp;
                op0.swizzle = arch::WWWW;
                op1.registerId = GPURegisterId(alpha_test_declaration.alpha_const_ref, arch::PARAM);
                op1.swizzle = arch::WWWW;
                res.registerId = temp;
                builder.setOperand(0, op0);
                builder.setOperand(1, op1);
                builder.setResult(res);
                
                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());
                
                //  Initialize the CG1 instruction builder.
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();

                // KIL -temp
                builder.setOpcode(KIL);
                op0.registerId = temp;
                op0.negate = true;
                builder.setOperand(0, op0);

                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());

                //  Release temporary register.
                releaseTemp(temp);
            }
            break;
            
        case D3DCMP_LESSEQUAL:
            {
                //
                //  ADD temp, alpha_ref.w, -color_temp.v
                //  KIL temp
                //
                
                //  Reserve a temporary register.
                GPURegisterId temp = reserveTemp();
                
                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();
                
                // ADD temp, alpha_ref.w, -color_temp.w
                builder.setOpcode(ADD);
                op0.registerId = GPURegisterId(alpha_test_declaration.alpha_const_ref, arch::PARAM);
                op0.swizzle = arch::WWWW;
                op1.registerId = color_temp;
                op1.swizzle = arch::WWWW;
                op1.negate = true;
                res.registerId = temp;
                builder.setOperand(0, op0);
                builder.setOperand(1, op1);
                builder.setResult(res);

                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());
                
                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();
                
                // KIL temp
                builder.setOpcode(KIL);
                op0.registerId = temp;
                builder.setOperand(0, op0);

                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());
                
                //  Release temporary register.
                releaseTemp(temp);
            }
            break;
            
        case D3DCMP_EQUAL:
            {
                //
                //  ADD temp, alpha_ref.w, -color_temp.w
                //  KIL -|temp|
                //
                
                //  Reserve a temporary register.
                GPURegisterId temp = reserveTemp();
                
                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();

                // ADD temp, alpha_ref.w, -color_temp.w
                builder.setOpcode(ADD);
                op0.registerId = GPURegisterId(alpha_test_declaration.alpha_const_ref, arch::PARAM);
                op0.swizzle = arch::WWWW;
                op1.registerId = color_temp;
                op1.swizzle = arch::WWWW;
                op1.negate = true;
                res.registerId = temp;
                builder.setOperand(0, op0);
                builder.setOperand(1, op1);
                builder.setResult(res);

                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());
                
                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();

                // KIL -|temp|
                builder.setOpcode(KIL);
                op0.registerId = temp;
                op0.negate = true;
                op0.absolute = true;
                builder.setOperand(0, op0);

                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());

                //  Release temporary register.
                releaseTemp(temp);
            }
            break;
            
        case D3DCMP_GREATEREQUAL:
            {
                //
                //  ADD temp, color_temp.w, -alpha_ref.w
                //  KIL temp
                //
                
                //  Reserve a temporary register.
                GPURegisterId temp = reserveTemp();
               
                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();

                // ADD temp, color_temp.w, -alpha_ref.w
                builder.setOpcode(ADD);
                op0.registerId = color_temp;
                op0.swizzle = arch::WWWW;
                op1.registerId = GPURegisterId(alpha_test_declaration.alpha_const_ref, arch::PARAM);
                op1.swizzle = arch::WWWW;
                op1.negate = true;
                res.registerId = temp;
                builder.setOperand(0, op0);
                builder.setOperand(1, op1);
                builder.setResult(res);

                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());

                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();

                // KIL temp
                builder.setOpcode(KIL);
                op0.registerId = temp;
                builder.setOperand(0, op0);

                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());
                
                //  Release temporary register.
                releaseTemp(temp);
            }
            break;
            
        case D3DCMP_GREATER:
        {
                //
                //  SGE temp, -color_temp.w, -alpha_ref.w
                //  KIL -temp
                //
                
                //  Reserve a temporary register.
                GPURegisterId temp = reserveTemp();

                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();

                // SGE temp, -color_temp.w, -alpha_ref.w
                builder.setOpcode(SGE);
                op0.registerId = color_temp;
                op0.swizzle = arch::WWWW;
                op0.negate = true;
                op1.registerId = GPURegisterId(alpha_test_declaration.alpha_const_ref, arch::PARAM);
                op1.swizzle = arch::WWWW;
                op1.negate = true;
                res.registerId = temp;
                builder.setOperand(0, op0);
                builder.setOperand(1, op1);
                builder.setResult(res);

                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());

                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();

                // KIL -temp
                builder.setOpcode(KIL);
                op0.registerId = temp;
                op0.negate = true;
                builder.setOperand(0, op0);

                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());

                //  Release temporary register.
                releaseTemp(temp);
            }
            break;
            
        case D3DCMP_NOTEQUAL:
        {
                //  SGE temp.x, color_temp.w, alpha_ref.w
                //  SGE temp.y, -color_temp.w, -alpha_ref.w
                //  MUL temp, temp.x, temp.y
                //  KIL -temp
                //
                
                //  Reserve a temporary register.
                GPURegisterId temp = reserveTemp();

                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();

                // SGE temp.x, color_temp.w, alpha_ref.w
                builder.setOpcode(SGE);
                op0.registerId = color_temp;
                op0.swizzle = arch::WWWW;
                op1.registerId = GPURegisterId(alpha_test_declaration.alpha_const_ref, arch::PARAM);
                op1.swizzle = arch::WWWW;
                res.registerId = temp;
                res.maskMode = XNNN;
                builder.setOperand(0, op0);
                builder.setOperand(1, op1);
                builder.setResult(res);

                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());

                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();

                // SGE temp.y, -color_temp.w, -alpha_ref.w
                builder.setOpcode(SGE);
                op0.registerId = color_temp;
                op0.swizzle = arch::WWWW;
                op0.negate = true;
                op1.registerId = GPURegisterId(alpha_test_declaration.alpha_const_ref, arch::PARAM);
                op1.swizzle = arch::WWWW;
                op1.negate = true;
                res.registerId = temp;
                res.maskMode = NYNN;
                builder.setOperand(0, op0);
                builder.setOperand(1, op1);
                builder.setResult(res);

                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());

                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();

                // MUL temp temp.x temp.y
                builder.setOpcode(MUL);
                op0.registerId = temp;
                op0.swizzle = XXXX;
                op1.registerId = temp;
                op1.swizzle = YYYY;
                res.registerId = temp;
                builder.setOperand(0, op0);
                builder.setOperand(1, op1);
                builder.setResult(res);

                //  Build instruction and add the the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());

                //  Initialize the CG1 instruction builder.                
                builder.resetParameters();
                op0 = op1 = Operand();
                res = Result();

                // KIL -temp
                builder.setOpcode(KIL);
                op0.registerId = temp;
                op0.negate = true;
                builder.setOperand(0, op0);

                //  Build instruction and add to the list of translated CG1 shader instructions.
                instructions.push_back(builder.buildInstruction());
                
                //  Release temporary register.
                releaseTemp(temp);
            }
            
            break;
            
        default:
            CG_ASSERT("Undefined alpha test comparison function.");
            break;
    }
    
    //  Copy the result color to the output color register.
    generateMov(color_out, color_temp);
}

void IRTranslator::visit(CommentIRNode *n)
{
}

void IRTranslator::visit(CommentDataIRNode *n)
{
}

void IRTranslator::visit(ParameterIRNode *n)
{
}

void IRTranslator::visit(SourceParameterIRNode *n)
{
}

void IRTranslator::visit(DestinationParameterIRNode *n)
{
}

void IRTranslator::visit(BoolIRNode *n)
{
}

void IRTranslator::visit(FloatIRNode *n)
{
}

void IRTranslator::visit(IntegerIRNode *n)
{
}

void IRTranslator::visit(SamplerInfoIRNode *n)
{
}

//  Visitor for semantic node.
void IRTranslator::visit(SemanticIRNode *n)
{
}

void IRTranslator::visit(RelativeAddressingIRNode *n)
{
}

void IRTranslator::visit(IRNode *n)
{
}

void IRTranslator::end()
{
}

