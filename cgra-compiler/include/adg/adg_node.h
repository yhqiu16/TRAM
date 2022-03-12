#ifndef __ADG_NODE_H__
#define __ADG_NODE_H__

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <assert.h>

// configuration data location
struct CfgDataLoc{
    int low;  // lowest bit index 
    int high; // highest bit index
};


class ADG;

class ADGNode
{
private:
    int _id;
    std::string _type;
    int _bitWidth;
    int _numInputs;
    int _numOutputs;
    int _cfgBlkIdx;
    int _x;
    int _y;
    // int _cfgBitLen;
    std::vector<uint64_t> _cfgBits;
    std::map<int, std::pair<int, int>> _inputs; // <input-index, <node-id, node-port-idx>>
    std::map<int, std::set<std::pair<int, int>>> _outputs; // <output-index, set<node-id, node-port-idx>>
    // inputs connected to each output, <output-index, set<input-index>>
    std::map<int, std::set<int>> _out2ins;
    // outputs connected to each input, <input-index, set<output-index>>
    std::map<int, std::set<int>> _in2outs; 
    // configuration info, <ID, CfgDataFormat>
    std::map<int, CfgDataLoc> _configInfo;
    // sub-ADG consisting of sub-modules
    ADG* _subADG; 
protected:
    void printData();
public:
    ADGNode(){}
    ADGNode(int id){ _id = id; }
    ~ADGNode(){} // cannot delete _subADG here because of pre-declare class; 
    int id(){ return _id; }
    std::string type(){ return _type; }
    int bitWidth(){ return _bitWidth; }
    int numInputs(){ return _numInputs; }
    int numOutputs(){ return _numOutputs; }
    int cfgBlkIdx(){ return _cfgBlkIdx; }
    int x(){ return _x; }
    int y(){ return _y; }
    // int cfgBitLen(){ return _cfgBitLen; }
    void setId(int id){ _id = id; }
    void setType(std::string type){ _type = type; }
    void setBitWidth(int bitWidth){ _bitWidth = bitWidth; }
    void setNumInputs(int numInputs){ _numInputs = numInputs; }
    void setNumOutputs(int numOutputs){ _numOutputs = numOutputs; }
    void setCfgBlkIdx(int cfgBlkIdx){ _cfgBlkIdx = cfgBlkIdx; }
    void setX(int x){ _x = x; }
    void setY(int y){ _y = y; }
    // void setCfgBitLen(int cfgBitLen){ _cfgBitLen = cfgBitLen; }
    ADG* subADG(){ return _subADG; }
    void setSubADG(ADG* subADG){ _subADG = subADG; }
    const std::map<int, std::pair<int, int>>& inputs(){ return _inputs; }
    const std::map<int, std::set<std::pair<int, int>>>& outputs(){ return _outputs; }
    std::pair<int, int> input(int index); // return <node-id, node-port-idx>
    std::set<std::pair<int, int>> output(int index); // return set<node-id, node-port-idx>
    void addInput(int index, std::pair<int, int> node); // add input, _input_used
    void addOutput(int index, std::pair<int, int> node); // add output, _output_used
    // inputs connected to each output
    std::set<int> out2ins(int outPort);
    // outputs connected to each input
    std::set<int> in2outs(int inPort);
    // add input connected to the output
    void addOut2ins(int outPort, int inPort);
    // add output connected to the input
    void addIn2outs(int inPort, int outPort);
    // check if the input and output port are connected
    bool isInOutConnected(int inPort, int outPort); 
    // configuration info, <ID, CfgDataFormat>
    const std::map<int, CfgDataLoc>& configInfo(){ return _configInfo;}
    // get config info for sub-module
    const CfgDataLoc& configInfo(int id);
    // add config info for sub-module
    void addConfigInfo(int id, CfgDataLoc subModuleCfg);
    // virtual void verify();
    // // analyze the connections among the internal sub-modules 
    // virtual void analyzeIntraConnect(){}
    virtual void print();
};


class GPENode : public ADGNode
{
private:
    int _numRfReg; // number of registers in RegFile 
    int _maxDelay; // max delay cycles of the internal DelayPipe 
    int _numOperands; // number of operands
    std::set<std::string> _operations; // supported operations
    std::vector<std::set<int>> _operandInputs; // indexes of input ports connected to each operand
public:
    using ADGNode::ADGNode; // C++11, inherit parent constructors
    int numRfReg(){ return _numRfReg; } 
    int maxDelay(){ return _maxDelay; }
    int numOperands(){ return _numOperands; }
    void setNumRfReg(int numRfReg){ _numRfReg = numRfReg; }
    void setMaxDelay(int maxDelay){ _maxDelay = maxDelay; }
    void setNumOperands(int numOperands); // set numOperands and resize _operandInputs 
    const std::set<std::string>& operations(){ return _operations; }
    void addOperation(std::string op); // add supported operation
    void delOperation(std::string op); // delete supported operation
    bool opCapable(std::string op); // check if the operation is supported
    const std::set<int>& operandInputs(int opeIdx); // get input ports connected to this operand
    void addOperandInputs(int opeIdx, int inputIdx); // add input port connected to this operand
    void delOperandInputs(int opeIdx, int inputIdx);
    void addOperandInputs(int opeIdx, std::set<int> inputIdxs); // add input ports connected to this operand
    int getOperandIdx(int inputIdx); // get which operand this input is connected
    virtual void print();
};


class GIBNode : public ADGNode
{
private:
    bool _trackReged; // if there are registers in the track output ports
    std::map<int, bool> _outReged; // if there are registers in the output port 
public:
    using ADGNode::ADGNode; // C++11, inherit parent constructors
    bool trackReged(){ return _trackReged; }
    void setTrackReged(bool trackReged){ _trackReged = trackReged; }
    bool outReged(int idx);
    void setOutReged(int idx, bool reged);
    virtual void print();
};


class IOBNode : public ADGNode
{
private:
    /* data */
public:
    using ADGNode::ADGNode; // C++11, inherit parent constructors
};






#endif