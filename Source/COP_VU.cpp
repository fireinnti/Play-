#include <assert.h>
#include "COP_VU.h"
#include "VUShared.h"
#include "MIPS.h"
#include "offsetof_def.h"
#include "MemoryUtils.h"

using namespace std;

CCOP_VU::CCOP_VU(MIPS_REGSIZE nRegSize) :
CMIPSCoprocessor(nRegSize),
m_nFT(0),
m_nFS(0),
m_nFD(0),
m_nDest(0),
m_nFTF(0),
m_nFSF(0),
m_nBc(0)
{
	SetupReflectionTables();
}

void CCOP_VU::CompileInstruction(uint32 nAddress, CMipsJitter* codeGen, CMIPS* pCtx)
{
	SetupQuickVariables(nAddress, codeGen, pCtx);

	m_nDest			= (uint8)((m_nOpcode >> 21) & 0x0F);
	
	m_nFSF			= ((m_nDest >> 0) & 0x03);
	m_nFTF			= ((m_nDest >> 2) & 0x03);

	m_nFT			= (uint8)((m_nOpcode >> 16) & 0x1F);
	m_nFS			= (uint8)((m_nOpcode >> 11) & 0x1F);
	m_nFD			= (uint8)((m_nOpcode >>  6) & 0x1F);

	m_nBc			= (uint8)((m_nOpcode >>  0) & 0x03);

	switch((m_nOpcode >> 26) & 0x3F)
	{
	case 0x12:
		//COP2
		((this)->*(m_pOpCop2[(m_nOpcode >> 21) & 0x1F]))();
		break;
	case 0x36:
		//LQC2
		LQC2();
		break;
	case 0x3E:
		//SQC2
		SQC2();
		break;
	default:
		Illegal();
		break;
	}
}

//////////////////////////////////////////////////
//General Instructions
//////////////////////////////////////////////////

//36
void CCOP_VU::LQC2()
{
    ComputeMemAccessAddr();

    for(unsigned int i = 0; i < 4; i++)
    {
		m_codeGen->PushCtx();
	    m_codeGen->PushIdx(1);
	    m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::GetWordProxy), 2, true);

        m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[m_nFT].nV[i]));

        if(i != 3)
        {
            m_codeGen->PushCst(4);
            m_codeGen->Add();
        }
    }

    m_codeGen->PullTop();
}

//3E
void CCOP_VU::SQC2()
{
	ComputeMemAccessAddr();

	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushCtx();
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[m_nFT].nV[i]));
		m_codeGen->PushIdx(2);
		m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::SetWordProxy), 3, false);

		if(i != 3)
		{
			m_codeGen->PushCst(4);
			m_codeGen->Add();
		}
	}

	m_codeGen->PullTop();
}

//////////////////////////////////////////////////
//COP2 Instructions
//////////////////////////////////////////////////

//01
void CCOP_VU::QMFC2()
{
    for(unsigned int i = 0; i < 4; i++)
    {
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[m_nFS].nV[i]));
        m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[i]));
    }
}

//02
void CCOP_VU::CFC2()
{
    if(m_nFS < 16)
    {
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nFS]));
        m_codeGen->PushCst(0xFFFF);
        m_codeGen->And();
    }
    else
    {
	    switch(m_nFS)
	    {
	    case 18:	//Clipping flag
            m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2CF));
            break;
	    case 16:	//STATUS
	    case 20:	//R
            assert(0);
		    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[0].nV[0]));
            break;
	    case 17:	//MAC flag
#ifdef _DEBUG
            printf("Warning: Reading contents of MAC flag through CFC2.\r\n");
#endif
	    case 26:	//TPC
	    case 28:	//FBRST
	    case 29:	//VPU-STAT
		    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[0].nV[0]));
		    break;
	    case 21:
		    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2I));
		    break;
	    case 22:
		    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2Q));
		    break;
	    default:
		    assert(0);
		    break;
	    }
    }

	m_codeGen->SignExt();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[0]));
}

//05
void CCOP_VU::QMTC2()
{
    for(unsigned int i = 0; i < 4; i++)
    {
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[i]));
        m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[m_nFS].nV[i]));
    }
}

//06
void CCOP_VU::CTC2()
{
    if(m_nFS < 16)
    {
        throw runtime_error("Not implemented.");
    }
    else
    {
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[0]));

        switch(m_nFS)
        {
        case 16:
            //STATUS - Not implemented
            m_codeGen->PullTop();
            break;
	    case 21:
		    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2I));
		    break;
        case 28:
            //FBRST - Don't care
            m_codeGen->PullTop();
            break;
        default:
            throw runtime_error("Not implemented.");
            break;
        }
    }
}

//10-1F
void CCOP_VU::V()
{
	((this)->*(m_pOpVector[(m_nOpcode & 0x3F)]))();
}

//////////////////////////////////////////////////
//Vector Instructions
//////////////////////////////////////////////////

//00
//01
//02
//03
void CCOP_VU::VADDbc()
{
	VUShared::ADDbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//04
//05
//06
//07
void CCOP_VU::VSUBbc()
{
    VUShared::SUBbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//08
//09
//0A
//0B
void CCOP_VU::VMADDbc()
{
    VUShared::MADDbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//0C
//0D
//0E
//0F
void CCOP_VU::VMSUBbc()
{
    VUShared::MSUBbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//10
//11
void CCOP_VU::VMAXbc()
{
    VUShared::MAXbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//14
//15
//17
void CCOP_VU::VMINIbc()
{
    VUShared::MINIbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//18
//19
//1A
//1B
void CCOP_VU::VMULbc()
{
	VUShared::MULbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//1C
void CCOP_VU::VMULq()
{
	VUShared::MULq(m_codeGen, m_nDest, m_nFD, m_nFS, m_nAddress);
}

//1F
void CCOP_VU::VMINIi()
{
    VUShared::MINIi(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//20
void CCOP_VU::VADDq()
{
	VUShared::ADDq(m_codeGen, m_nDest, m_nFD, m_nFS, m_nAddress);
}

//28
void CCOP_VU::VADD()
{
	VUShared::ADD(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2A
void CCOP_VU::VMUL()
{
	VUShared::MUL(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2B
void CCOP_VU::VMAX()
{
	VUShared::MAX(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2C
void CCOP_VU::VSUB()
{
	VUShared::SUB(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2E
void CCOP_VU::VOPMSUB()
{
	VUShared::OPMSUB(m_codeGen, m_nFD, m_nFS, m_nFT);
}

//2F
void CCOP_VU::VMINI()
{
	VUShared::MINI(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//3C
void CCOP_VU::VX0()
{
	((this)->*(m_pOpVx0[(m_nOpcode >> 6) & 0x1F]))();
}

//3D
void CCOP_VU::VX1()
{
	((this)->*(m_pOpVx1[(m_nOpcode >> 6) & 0x1F]))();
}

//3E
void CCOP_VU::VX2()
{
	((this)->*(m_pOpVx2[(m_nOpcode >> 6) & 0x1F]))();
}

//3F
void CCOP_VU::VX3()
{
	((this)->*(m_pOpVx3[(m_nOpcode >> 6) & 0x1F]))();
}

//////////////////////////////////////////////////
//Vx Common Instructions
//////////////////////////////////////////////////

//
void CCOP_VU::VADDAbc()
{
    VUShared::ADDAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc);
}

//
void CCOP_VU::VSUBAbc()
{
    VUShared::SUBAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc);
}

//
void CCOP_VU::VMADDAbc()
{
	VUShared::MADDAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc);
}

//
void CCOP_VU::VMSUBAbc()
{
    VUShared::MSUBAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc);
}

//
void CCOP_VU::VMULAbc()
{
	VUShared::MULAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc);
}

//////////////////////////////////////////////////
//V0 Instructions
//////////////////////////////////////////////////

//04
void CCOP_VU::VITOF0()
{
	VUShared::ITOF0(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//05
void CCOP_VU::VFTOI0()
{
	VUShared::FTOI0(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//0A
void CCOP_VU::VADDA()
{
    VUShared::ADDA(m_codeGen, m_nDest, m_nFS, m_nFT);
}

//0C
void CCOP_VU::VMOVE()
{
    VUShared::MOVE(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//0E
void CCOP_VU::VDIV()
{
	VUShared::DIV(m_codeGen, m_nFS, m_nFSF, m_nFT, m_nFTF, m_nAddress, 1);
}

//10
void CCOP_VU::VRNEXT()
{
    VUShared::RNEXT(m_codeGen, m_nDest, m_nFT);
}

//////////////////////////////////////////////////
//V1 Instructions
//////////////////////////////////////////////////

//04
void CCOP_VU::VITOF4()
{
    VUShared::ITOF4(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//05
void CCOP_VU::VFTOI4()
{
	VUShared::FTOI4(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//07
void CCOP_VU::VABS()
{
    VUShared::ABS(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//0C
void CCOP_VU::VMR32()
{
	VUShared::MR32(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//0E
void CCOP_VU::VSQRT()
{
	VUShared::SQRT(m_codeGen, m_nFT, m_nFTF, m_nAddress, 1);
}

//////////////////////////////////////////////////
//V2 Instructions
//////////////////////////////////////////////////

//04
void CCOP_VU::VITOF12()
{
	VUShared::ITOF12(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//05
void CCOP_VU::VFTOI12()
{
	VUShared::FTOI12(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//0B
void CCOP_VU::VOPMULA()
{
    VUShared::OPMULA(m_codeGen, m_nFS, m_nFT);
}

//0E
void CCOP_VU::VRSQRT()
{
	VUShared::RSQRT(m_codeGen, m_nFS, m_nFSF, m_nFT, m_nFTF, m_nAddress, 1);
}

//10
void CCOP_VU::VRINIT()
{
    VUShared::RINIT(m_codeGen, m_nFS, m_nFSF);
}

//////////////////////////////////////////////////
//V3 Instructions
//////////////////////////////////////////////////

//04
void CCOP_VU::VITOF15()
{
    VUShared::ITOF15(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//07
void CCOP_VU::VCLIP()
{
    VUShared::CLIP(m_codeGen, m_nFS, m_nFT);
}

//0B
void CCOP_VU::VNOP()
{
	//Nothing to do
}

//0E
void CCOP_VU::VWAITQ()
{
    VUShared::WAITQ(m_codeGen);
}

//10
void CCOP_VU::VRXOR()
{
    VUShared::RXOR(m_codeGen, m_nFS, m_nFSF);
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

CCOP_VU::InstructionFuncConstant CCOP_VU::m_pOpCop2[0x20] =
{
	//0x00
	&CCOP_VU::Illegal,			&CCOP_VU::QMFC2,			&CCOP_VU::CFC2,			&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::QMTC2,			&CCOP_VU::CTC2,			&CCOP_VU::Illegal,
	//0x08
	&CCOP_VU::Illegal,			&CCOP_VU::Illegal,			&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,			&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
	//0x10
	&CCOP_VU::V,				&CCOP_VU::V,				&CCOP_VU::V,			&CCOP_VU::V,			&CCOP_VU::V,			&CCOP_VU::V,				&CCOP_VU::V,			&CCOP_VU::V,
	//0x18
	&CCOP_VU::V,				&CCOP_VU::V,				&CCOP_VU::V,			&CCOP_VU::V,			&CCOP_VU::V,			&CCOP_VU::V,				&CCOP_VU::V,			&CCOP_VU::V,
};

CCOP_VU::InstructionFuncConstant CCOP_VU::m_pOpVector[0x40] =
{
	//0x00
	&CCOP_VU::VADDbc,		&CCOP_VU::VADDbc,		&CCOP_VU::VADDbc,		&CCOP_VU::VADDbc,		&CCOP_VU::VSUBbc,		&CCOP_VU::VSUBbc,		&CCOP_VU::VSUBbc,		&CCOP_VU::VSUBbc,
	//0x08
	&CCOP_VU::VMADDbc,		&CCOP_VU::VMADDbc,		&CCOP_VU::VMADDbc,		&CCOP_VU::VMADDbc,		&CCOP_VU::VMSUBbc,		&CCOP_VU::VMSUBbc,		&CCOP_VU::VMSUBbc,		&CCOP_VU::VMSUBbc,
	//0x10
	&CCOP_VU::VMAXbc,		&CCOP_VU::VMAXbc,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VMINIbc,		&CCOP_VU::VMINIbc,		&CCOP_VU::Illegal,		&CCOP_VU::VMINIbc,
	//0x18
	&CCOP_VU::VMULbc,		&CCOP_VU::VMULbc,		&CCOP_VU::VMULbc,		&CCOP_VU::VMULbc,		&CCOP_VU::VMULq,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VMINIi,
	//0x20
	&CCOP_VU::VADDq,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
	//0x28
	&CCOP_VU::VADD,			&CCOP_VU::Illegal,		&CCOP_VU::VMUL,			&CCOP_VU::VMAX,			&CCOP_VU::VSUB,			&CCOP_VU::Illegal,		&CCOP_VU::VOPMSUB,		&CCOP_VU::VMINI,
	//0x30
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
	//0x38
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VX0,			&CCOP_VU::VX1,			&CCOP_VU::VX2,			&CCOP_VU::VX3,
};

CCOP_VU::InstructionFuncConstant CCOP_VU::m_pOpVx0[0x20] =
{
	//0x00
	&CCOP_VU::VADDAbc,		&CCOP_VU::VSUBAbc,		&CCOP_VU::VMADDAbc,		&CCOP_VU::VMSUBAbc,		&CCOP_VU::VITOF0,		&CCOP_VU::VFTOI0,		&CCOP_VU::VMULAbc,		&CCOP_VU::Illegal,
	//0x08
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VADDA,	    &CCOP_VU::Illegal,		&CCOP_VU::VMOVE,		&CCOP_VU::Illegal,		&CCOP_VU::VDIV,			&CCOP_VU::Illegal,
	//0x10
	&CCOP_VU::VRNEXT,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
	//0x18
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
};

CCOP_VU::InstructionFuncConstant CCOP_VU::m_pOpVx1[0x20] =
{
	//0x00
	&CCOP_VU::VADDAbc,		&CCOP_VU::VSUBAbc,		&CCOP_VU::VMADDAbc,		&CCOP_VU::VMSUBAbc,		&CCOP_VU::VITOF4,	    &CCOP_VU::VFTOI4,		&CCOP_VU::VMULAbc,		&CCOP_VU::VABS,
	//0x08
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VMR32,		&CCOP_VU::Illegal,		&CCOP_VU::VSQRT,		&CCOP_VU::Illegal,
	//0x10
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
	//0x18
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
};

CCOP_VU::InstructionFuncConstant CCOP_VU::m_pOpVx2[0x20] =
{
	//0x00
	&CCOP_VU::VADDAbc,		&CCOP_VU::VSUBAbc,		&CCOP_VU::VMADDAbc,		&CCOP_VU::VMSUBAbc,		&CCOP_VU::VITOF12,		&CCOP_VU::VFTOI12,		&CCOP_VU::VMULAbc,		&CCOP_VU::Illegal,
	//0x08
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VOPMULA,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VRSQRT,		&CCOP_VU::Illegal,
	//0x10
	&CCOP_VU::VRINIT,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
	//0x18
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
};

CCOP_VU::InstructionFuncConstant CCOP_VU::m_pOpVx3[0x20] =
{
	//0x00
	&CCOP_VU::VADDAbc,		&CCOP_VU::VSUBAbc,		&CCOP_VU::VMADDAbc,		&CCOP_VU::VMSUBAbc,		&CCOP_VU::VITOF15,		&CCOP_VU::Illegal,		&CCOP_VU::VMULAbc,		&CCOP_VU::VCLIP,
	//0x08
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VNOP,			&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VWAITQ,		&CCOP_VU::Illegal,
	//0x10
	&CCOP_VU::VRXOR,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
	//0x18
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
};
