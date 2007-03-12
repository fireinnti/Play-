#include <stdexcept>
#include "MipsTestEngine.h"
#include "StdStream.h"
#include "lexical_cast_ex.h"
#include "xml/Node.h"
#include "xml/Parser.h"
#include "xml/FilteringNodeIterator.h"
#include "xml/Utils.h"

using namespace Framework;
using namespace std;

CMipsTestEngine::CMipsTestEngine(const char* sPath)
{
    Xml::CNode* pRootNode;
    pRootNode = Xml::CParser::ParseDocument(&CStdStream(fopen(sPath, "rb")));

    if(pRootNode == NULL)
    {
        throw exception();
    }

    LoadInputs(pRootNode->Select("Test/Inputs"));
    LoadOutputs(pRootNode->Select("Test/Outputs"));
    LoadInstances(pRootNode->Select("Test/Instances"));

    delete pRootNode;
}

CMipsTestEngine::~CMipsTestEngine()
{

}

CMipsTestEngine::OutputsType::iterator CMipsTestEngine::GetOutputsBegin()
{
    return m_Outputs.begin();
}

CMipsTestEngine::OutputsType::iterator CMipsTestEngine::GetOutputsEnd()
{
    return m_Outputs.end();
}

CMipsTestEngine::CValueSet* CMipsTestEngine::GetInput(unsigned int nId)
{
    InputsByIdMapType::iterator itInput;
    itInput = m_InputsById.find(nId);
    return (itInput != m_InputsById.end()) ? (itInput->second) : (NULL);
}

void CMipsTestEngine::LoadInputs(Xml::CNode* pInputsNode)
{
    if(pInputsNode == NULL)
    {
        throw exception();
    }

    for(Xml::CFilteringNodeIterator itNode(pInputsNode, "ValueSet"); !itNode.IsEnd(); itNode++)
    {
        CValueSet* pInput(new CValueSet(*itNode));
        m_Inputs.push_back(pInput);
        m_InputsById[pInput->GetInputId()] = pInput;
    }
}

void CMipsTestEngine::LoadOutputs(Xml::CNode* pOutputsNode)
{
    if(pOutputsNode == NULL)
    {
        throw exception();
    }

    for(Xml::CFilteringNodeIterator itNode(pOutputsNode, "ValueSet"); !itNode.IsEnd(); itNode++)
    {
        m_Outputs.push_back(new CValueSet(*itNode));
    }
}

void CMipsTestEngine::LoadInstances(Xml::CNode* pInstancesNode)
{
    if(pInstancesNode == NULL)
    {
        throw exception();
    }

    for(Xml::CFilteringNodeIterator itNode(pInstancesNode, "Instance"); !itNode.IsEnd(); itNode++)
    {
        m_Instances.push_back(new CInstance(*itNode));
    }
}

////////////////////////////////////////////////////
// CValueSet implementation
////////////////////////////////////////////////////

CMipsTestEngine::CValueSet::CValueSet(Xml::CNode* pValueSetNode)
{
    m_nInputId = 0;
    m_nInstanceId = 0;

    Xml::GetAttributeIntValue(pValueSetNode, "InputId", reinterpret_cast<int*>(&m_nInputId));
    Xml::GetAttributeIntValue(pValueSetNode, "InstanceId", reinterpret_cast<int*>(&m_nInstanceId));

    for(Xml::CNode::NodeIterator itNode(pValueSetNode->GetChildrenBegin());
        itNode != pValueSetNode->GetChildrenEnd(); itNode++)
    {
        Xml::CNode* pNode(*itNode);
        if(!pNode->IsTag()) continue;
        
        //Check the value type
        if(!strcmp(pNode->GetText(), "Register"))
        {
            m_Values.push_back(new CRegisterValue(pNode));
        }
        else
        {
            throw runtime_error(string("Unknown value type '") + pNode->GetText() + string("' encountered."));
        }
    }
}

CMipsTestEngine::CValueSet::~CValueSet()
{

}

unsigned int CMipsTestEngine::CValueSet::GetInputId() const
{
    return m_nInputId;
}

unsigned int CMipsTestEngine::CValueSet::GetInstanceId() const
{
    return m_nInstanceId;
}

////////////////////////////////////////////////////
// CInstance implementation
////////////////////////////////////////////////////

CMipsTestEngine::CInstance::CInstance(Xml::CNode* pInstanceNode)
{
    if(!Xml::GetAttributeIntValue(pInstanceNode, "Id", reinterpret_cast<int*>(&m_nId)))
    {
        throw runtime_error("No Id declared for instance.");
    }

    m_sSource = pInstanceNode->GetInnerText();
}

CMipsTestEngine::CInstance::~CInstance()
{

}

////////////////////////////////////////////////////
// CValue implementation
////////////////////////////////////////////////////

CMipsTestEngine::CValue::~CValue()
{

}

////////////////////////////////////////////////////
// CRegisterValue implementation
////////////////////////////////////////////////////

CMipsTestEngine::CRegisterValue::CRegisterValue(Xml::CNode* pNode)
{
    const char* sName;
    const char* sValue;
    if(!Xml::GetAttributeStringValue(pNode, "Name", &sName))
    {
        throw runtime_error("RegisterValue: Couldn't find attribute 'Name'.");
    }

    m_nRegister = -1;
    for(unsigned int i = 0; i < 32; i++)
    {
        if(!strcmp(CMIPS::m_sGPRName[i], sName))
        {
            m_nRegister = i;
            break;
        }
    }

    if(m_nRegister == -1)
    {
        throw runtime_error("RegisterValue: Invalid register name.");
    }

    m_nValue0 = 0;
    m_nValue1 = 0;

    if(Xml::GetAttributeStringValue(pNode, "Value0", &sValue))
    {
        m_nValue0 = lexical_cast_hex<string>(sValue);
    }

    if(Xml::GetAttributeStringValue(pNode, "Value1", &sValue))
    {
        m_nValue1 = lexical_cast_hex<string>(sValue);
    }
}

CMipsTestEngine::CRegisterValue::~CRegisterValue()
{

}

void CMipsTestEngine::CRegisterValue::AssembleLoad()
{

}

void CMipsTestEngine::CRegisterValue::Verify()
{

}